/* SPDX-FileCopyrightText: 2009-2010 NVIDIA Corporation
 * SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Adapted code from NVIDIA Corporation. */

#include "bvh/sort.h"

#include "bvh/params.h"
#include "bvh/unaligned.h"

#include "util/algorithm.h"
#include "util/task.h"

namespace ccl {

static const int BVH_SORT_THRESHOLD = 4096;

struct BVHReferenceCompare {
 public:
  int dim;
  const BVHUnaligned *unaligned_heuristic;
  const Transform *aligned_space;

  BVHReferenceCompare(const int dim,
                      const BVHUnaligned *unaligned_heuristic,
                      const Transform *aligned_space)
      : dim(dim), unaligned_heuristic(unaligned_heuristic), aligned_space(aligned_space)
  {
  }

  __forceinline BoundBox get_prim_bounds(const BVHReference &prim) const
  {
    return (aligned_space != nullptr) ?
               unaligned_heuristic->compute_aligned_prim_boundbox(prim, *aligned_space) :
               prim.bounds();
  }

  /* Compare two references.
   *
   * Returns value is similar to return value of `strcmp()`.
   */
  __forceinline int compare(const BVHReference &ra, const BVHReference &rb) const
  {
    BoundBox ra_bounds = get_prim_bounds(ra);
    BoundBox rb_bounds = get_prim_bounds(rb);
    const float ca = ra_bounds.min[dim] + ra_bounds.max[dim];
    const float cb = rb_bounds.min[dim] + rb_bounds.max[dim];

    if (ca < cb) {
      return -1;
    }
    if (ca > cb) {
      return 1;
    }
    if (ra.prim_object() < rb.prim_object()) {
      return -1;
    }
    if (ra.prim_object() > rb.prim_object()) {
      return 1;
    }
    if (ra.prim_index() < rb.prim_index()) {
      return -1;
    }
    if (ra.prim_index() > rb.prim_index()) {
      return 1;
    }
    if (ra.prim_type() < rb.prim_type()) {
      return -1;
    }
    if (ra.prim_type() > rb.prim_type()) {
      return 1;
    }

    return 0;
  }

  bool operator()(const BVHReference &ra, const BVHReference &rb)
  {
    return (compare(ra, rb) < 0);
  }
};

static void bvh_reference_sort_threaded(TaskPool *task_pool,
                                        BVHReference *data,
                                        const int job_start,
                                        const int job_end,
                                        const BVHReferenceCompare &compare);

/* Multi-threaded reference sort. */
static void bvh_reference_sort_threaded(TaskPool *task_pool,
                                        BVHReference *data,
                                        const int job_start,
                                        const int job_end,
                                        const BVHReferenceCompare &compare)
{
  int start = job_start;
  int end = job_end;
  bool have_work = (start < end);
  while (have_work) {
    const int count = job_end - job_start;
    if (count < BVH_SORT_THRESHOLD) {
      /* Number of reference low enough, faster to finish the job
       * in one thread rather than to spawn more threads.
       */
      sort(data + job_start, data + job_end + 1, compare);
      break;
    }
    /* Single QSort step.
     * Use median-of-three method for the pivot point.
     */
    int left = start;
    int right = end;
    const int center = (left + right) >> 1;
    if (compare.compare(data[left], data[center]) > 0) {
      swap(data[left], data[center]);
    }
    if (compare.compare(data[left], data[right]) > 0) {
      swap(data[left], data[right]);
    }
    if (compare.compare(data[center], data[right]) > 0) {
      swap(data[center], data[right]);
    }
    swap(data[center], data[right - 1]);
    const BVHReference median = data[right - 1];
    do {
      while (compare.compare(data[left], median) < 0) {
        ++left;
      }
      while (compare.compare(data[right], median) > 0) {
        --right;
      }
      if (left <= right) {
        swap(data[left], data[right]);
        ++left;
        --right;
      }
    } while (left <= right);
    /* We only create one new task here to reduce downside effects of
     * latency in TaskScheduler.
     * So generally current thread keeps working on the left part of the
     * array, and we create new task for the right side.
     * However, if there's nothing to be done in the left side of the array
     * we don't create any tasks and make it so current thread works on the
     * right side.
     */
    have_work = false;
    if (left < end) {
      if (start < right) {
        task_pool->push([task_pool, data, left, end, compare] {
          bvh_reference_sort_threaded(task_pool, data, left, end, compare);
        });
      }
      else {
        start = left;
        have_work = true;
      }
    }
    if (start < right) {
      end = right;
      have_work = true;
    }
  }
}

void bvh_reference_sort(const int start,
                        const int end,
                        BVHReference *data,
                        const int dim,
                        const BVHUnaligned *unaligned_heuristic,
                        const Transform *aligned_space)
{
  const int count = end - start;
  const BVHReferenceCompare compare(dim, unaligned_heuristic, aligned_space);
  if (count < BVH_SORT_THRESHOLD) {
    /* It is important to not use any mutex if array is small enough,
     * otherwise we end up in situation when we're going to sleep far
     * too often.
     */
    sort(data + start, data + end, compare);
  }
  else {
    TaskPool task_pool;
    bvh_reference_sort_threaded(&task_pool, data, start, end - 1, compare);
    task_pool.wait_work();
  }
}

}
