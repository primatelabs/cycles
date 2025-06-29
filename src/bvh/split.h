/* SPDX-FileCopyrightText: 2009-2010 NVIDIA Corporation
 * SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Adapted code from NVIDIA Corporation. */

#pragma once

#include "bvh/build.h"
#include "bvh/params.h"

namespace ccl {

class BVHBuild;
class Hair;
class Mesh;
class PointCloud;
struct Transform;

/* Object Split */

class BVHObjectSplit {
 public:
  float sah;
  int dim;
  int num_left;
  BoundBox left_bounds;
  BoundBox right_bounds;

  BVHObjectSplit() = default;
  BVHObjectSplit(BVHBuild *builder,
                 BVHSpatialStorage *storage,
                 const BVHRange &range,
                 vector<BVHReference> &references,
                 const float nodeSAH,
                 const BVHUnaligned *unaligned_heuristic = nullptr,
                 const Transform *aligned_space = nullptr);

  void split(BVHRange &left, BVHRange &right, const BVHRange &range);

 protected:
  BVHSpatialStorage *storage_;
  vector<BVHReference> *references_;
  const BVHUnaligned *unaligned_heuristic_;
  const Transform *aligned_space_;

  __forceinline BoundBox get_prim_bounds(const BVHReference &prim) const
  {
    if (aligned_space_ == nullptr) {
      return prim.bounds();
    }
    return unaligned_heuristic_->compute_aligned_prim_boundbox(prim, *aligned_space_);
  }
};

/* Spatial Split */

class BVHSpatialSplit {
 public:
  float sah;
  int dim;
  float pos;

  BVHSpatialSplit() : sah(FLT_MAX), dim(0), pos(0.0f), storage_(nullptr), references_(nullptr) {}
  BVHSpatialSplit(const BVHBuild &builder,
                  BVHSpatialStorage *storage,
                  const BVHRange &range,
                  vector<BVHReference> &references,
                  const float nodeSAH,
                  const BVHUnaligned *unaligned_heuristic = nullptr,
                  const Transform *aligned_space = nullptr);

  void split(BVHBuild *builder, BVHRange &left, BVHRange &right, const BVHRange &range);

  void split_reference(const BVHBuild &builder,
                       BVHReference &left,
                       BVHReference &right,
                       const BVHReference &ref,
                       const int dim,
                       float pos);

 protected:
  BVHSpatialStorage *storage_;
  vector<BVHReference> *references_;
  const BVHUnaligned *unaligned_heuristic_;
  const Transform *aligned_space_;

  /* Lower-level functions which calculates boundaries of left and right nodes
   * needed for spatial split.
   *
   * Operates directly with primitive specified by its index, reused by higher
   * level splitting functions.
   */
  void split_triangle_primitive(const Mesh *mesh,
                                const Transform *tfm,
                                const int prim_index,
                                const int dim,
                                const float pos,
                                BoundBox &left_bounds,
                                BoundBox &right_bounds);
  void split_curve_primitive(const Hair *hair,
                             const Transform *tfm,
                             const int prim_index,
                             const int segment_index,
                             const int dim,
                             const float pos,
                             BoundBox &left_bounds,
                             BoundBox &right_bounds);
  void split_point_primitive(const PointCloud *pointcloud,
                             const Transform *tfm,
                             const int prim_index,
                             const int dim,
                             const float pos,
                             BoundBox &left_bounds,
                             BoundBox &right_bounds);

  /* Lower-level functions which calculates boundaries of left and right nodes
   * needed for spatial split.
   *
   * Operates with BVHReference, internally uses lower level API functions.
   */
  void split_triangle_reference(const BVHReference &ref,
                                const Mesh *mesh,
                                const int dim,
                                const float pos,
                                BoundBox &left_bounds,
                                BoundBox &right_bounds);
  void split_curve_reference(const BVHReference &ref,
                             const Hair *hair,
                             const int dim,
                             const float pos,
                             BoundBox &left_bounds,
                             BoundBox &right_bounds);
  void split_point_reference(const BVHReference &ref,
                             const PointCloud *pointcloud,
                             const int dim,
                             const float pos,
                             BoundBox &left_bounds,
                             BoundBox &right_bounds);
  void split_object_reference(const Object *object,
                              const int dim,
                              const float pos,
                              BoundBox &left_bounds,
                              BoundBox &right_bounds);

  __forceinline BoundBox get_prim_bounds(const BVHReference &prim) const
  {
    if (aligned_space_ == nullptr) {
      return prim.bounds();
    }
    return unaligned_heuristic_->compute_aligned_prim_boundbox(prim, *aligned_space_);
  }

  __forceinline float3 get_unaligned_point(const float3 &point) const
  {
    if (aligned_space_ == nullptr) {
      return point;
    }
    return transform_point(aligned_space_, point);
  }
};

/* Mixed Object-Spatial Split */

class BVHMixedSplit {
 public:
  BVHObjectSplit object;
  BVHSpatialSplit spatial;

  float leafSAH;
  float nodeSAH;
  float minSAH;

  bool no_split;

  BoundBox bounds;

  BVHMixedSplit() = default;

  __forceinline BVHMixedSplit(BVHBuild *builder,
                              BVHSpatialStorage *storage,
                              const BVHRange &range,
                              vector<BVHReference> &references,
                              const int level,
                              const BVHUnaligned *unaligned_heuristic = nullptr,
                              const Transform *aligned_space = nullptr)
  {
    if (aligned_space == nullptr) {
      bounds = range.bounds();
    }
    else {
      bounds = unaligned_heuristic->compute_aligned_boundbox(
          range, &references.at(0), *aligned_space);
    }
    /* find split candidates. */
    const float area = bounds.safe_area();

    leafSAH = area * builder->params.primitive_cost(range.size());
    nodeSAH = area * builder->params.node_cost(2);

    object = BVHObjectSplit(
        builder, storage, range, references, nodeSAH, unaligned_heuristic, aligned_space);

    if (builder->params.use_spatial_split && level < BVHParams::MAX_SPATIAL_DEPTH) {
      BoundBox overlap = object.left_bounds;
      overlap.intersect(object.right_bounds);

      if (overlap.safe_area() >= builder->spatial_min_overlap) {
        spatial = BVHSpatialSplit(
            *builder, storage, range, references, nodeSAH, unaligned_heuristic, aligned_space);
      }
    }

    /* leaf SAH is the lowest => create leaf. */
    minSAH = min(min(leafSAH, object.sah), spatial.sah);
    no_split = (minSAH == leafSAH && builder->range_within_max_leaf_size(range, references));
  }

  __forceinline void split(BVHBuild *builder,
                           BVHRange &left,
                           BVHRange &right,
                           const BVHRange &range)
  {
    if (builder->params.use_spatial_split && minSAH == spatial.sah) {
      spatial.split(builder, left, right, range);
    }
    if (!left.size() || !right.size()) {
      object.split(left, right, range);
    }
  }
};

}
