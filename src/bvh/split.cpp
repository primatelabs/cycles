/* SPDX-FileCopyrightText: 2009-2010 NVIDIA Corporation
 * SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Adapted code from NVIDIA Corporation. */

#include "bvh/split.h"

#include "bvh/build.h"
#include "bvh/sort.h"

#include "scene/hair.h"
#include "scene/mesh.h"
#include "scene/object.h"
#include "scene/pointcloud.h"

#include "util/algorithm.h"

namespace ccl {

/* Object Split */

BVHObjectSplit::BVHObjectSplit(BVHBuild *builder,
                               BVHSpatialStorage *storage,
                               const BVHRange &range,
                               vector<BVHReference> &references,
                               const float nodeSAH,
                               const BVHUnaligned *unaligned_heuristic,
                               const Transform *aligned_space)
    : sah(FLT_MAX),
      dim(0),
      num_left(0),
      left_bounds(BoundBox::empty),
      right_bounds(BoundBox::empty),
      storage_(storage),
      references_(&references),
      unaligned_heuristic_(unaligned_heuristic),
      aligned_space_(aligned_space)
{
  const BVHReference *ref_ptr = &references_->at(range.start());
  float min_sah = FLT_MAX;

  for (int dim = 0; dim < 3; dim++) {
    /* Sort references. */
    bvh_reference_sort(range.start(),
                       range.end(),
                       &references_->at(0),
                       dim,
                       unaligned_heuristic_,
                       aligned_space_);

    // Resize must be called after every bvh_reference_sort, as sorting may use a task pool for
    // large ranges. This may cause another BVHObjectSplit to use and resize the storage on the
    // same thread.
    storage_->right_bounds.resize(range.size());

    /* sweep right to left and determine bounds. */
    BoundBox right_bounds = BoundBox::empty;
    for (int i = range.size() - 1; i > 0; i--) {
      const BoundBox prim_bounds = get_prim_bounds(ref_ptr[i]);
      right_bounds.grow(prim_bounds);
      storage_->right_bounds[i - 1] = right_bounds;
    }

    /* sweep left to right and select lowest SAH. */
    BoundBox left_bounds = BoundBox::empty;

    for (int i = 1; i < range.size(); i++) {
      const BoundBox prim_bounds = get_prim_bounds(ref_ptr[i - 1]);
      left_bounds.grow(prim_bounds);
      right_bounds = storage_->right_bounds[i - 1];

      const float sah = nodeSAH + left_bounds.safe_area() * builder->params.primitive_cost(i) +
                        right_bounds.safe_area() *
                            builder->params.primitive_cost(range.size() - i);

      if (sah < min_sah) {
        min_sah = sah;

        this->sah = sah;
        this->dim = dim;
        this->num_left = i;
        this->left_bounds = left_bounds;
        this->right_bounds = right_bounds;
      }
    }
  }
}

void BVHObjectSplit::split(BVHRange &left, BVHRange &right, const BVHRange &range)
{
  assert(!references_->empty());
  /* sort references according to split */
  bvh_reference_sort(range.start(),
                     range.end(),
                     &references_->at(0),
                     this->dim,
                     unaligned_heuristic_,
                     aligned_space_);

  BoundBox effective_left_bounds;
  BoundBox effective_right_bounds;
  const int num_right = range.size() - this->num_left;
  if (aligned_space_ == nullptr) {
    effective_left_bounds = left_bounds;
    effective_right_bounds = right_bounds;
  }
  else {
    effective_left_bounds = BoundBox::empty;
    effective_right_bounds = BoundBox::empty;
    for (int i = 0; i < this->num_left; ++i) {
      const BoundBox prim_boundbox = references_->at(range.start() + i).bounds();
      effective_left_bounds.grow(prim_boundbox);
    }
    for (int i = 0; i < num_right; ++i) {
      const BoundBox prim_boundbox = references_->at(range.start() + this->num_left + i).bounds();
      effective_right_bounds.grow(prim_boundbox);
    }
  }

  /* split node ranges */
  left = BVHRange(effective_left_bounds, range.start(), this->num_left);
  right = BVHRange(effective_right_bounds, left.end(), num_right);
}

/* Spatial Split */

BVHSpatialSplit::BVHSpatialSplit(const BVHBuild &builder,
                                 BVHSpatialStorage *storage,
                                 const BVHRange &range,
                                 vector<BVHReference> &references,
                                 const float nodeSAH,
                                 const BVHUnaligned *unaligned_heuristic,
                                 const Transform *aligned_space)
    : sah(FLT_MAX),
      dim(0),
      pos(0.0f),
      storage_(storage),
      references_(&references),
      unaligned_heuristic_(unaligned_heuristic),
      aligned_space_(aligned_space)
{
  /* initialize bins. */
  BoundBox range_bounds;
  if (aligned_space == nullptr) {
    range_bounds = range.bounds();
  }
  else {
    range_bounds = unaligned_heuristic->compute_aligned_boundbox(
        range, &references_->at(0), *aligned_space);
  }

  float3 origin = range_bounds.min;
  float3 binSize = (range_bounds.max - origin) * (1.0f / (float)BVHParams::NUM_SPATIAL_BINS);
  const float3 invBinSize = safe_divide(make_float3(1.0f), binSize);

  for (int dim = 0; dim < 3; dim++) {
    for (int i = 0; i < BVHParams::NUM_SPATIAL_BINS; i++) {
      BVHSpatialBin &bin = storage_->bins[dim][i];

      bin.bounds = BoundBox::empty;
      bin.enter = 0;
      bin.exit = 0;
    }
  }

  /* chop references into bins. */
  for (unsigned int refIdx = range.start(); refIdx < range.end(); refIdx++) {
    const BVHReference &ref = references_->at(refIdx);
    const BoundBox prim_bounds = get_prim_bounds(ref);
    const float3 firstBinf = (prim_bounds.min - origin) * invBinSize;
    const float3 lastBinf = (prim_bounds.max - origin) * invBinSize;
    int3 firstBin = make_int3((int)firstBinf.x, (int)firstBinf.y, (int)firstBinf.z);
    int3 lastBin = make_int3((int)lastBinf.x, (int)lastBinf.y, (int)lastBinf.z);

    firstBin = clamp(firstBin, 0, BVHParams::NUM_SPATIAL_BINS - 1);
    lastBin = clamp(lastBin, firstBin, BVHParams::NUM_SPATIAL_BINS - 1);

    for (int dim = 0; dim < 3; dim++) {
      BVHReference currRef(
          get_prim_bounds(ref), ref.prim_index(), ref.prim_object(), ref.prim_type());

      for (int i = firstBin[dim]; i < lastBin[dim]; i++) {
        BVHReference leftRef;
        BVHReference rightRef;

        split_reference(
            builder, leftRef, rightRef, currRef, dim, origin[dim] + binSize[dim] * (float)(i + 1));
        storage_->bins[dim][i].bounds.grow(leftRef.bounds());
        currRef = rightRef;
      }

      storage_->bins[dim][lastBin[dim]].bounds.grow(currRef.bounds());
      storage_->bins[dim][firstBin[dim]].enter++;
      storage_->bins[dim][lastBin[dim]].exit++;
    }
  }

  /* select best split plane. */
  storage_->right_bounds.resize(BVHParams::NUM_SPATIAL_BINS);
  for (int dim = 0; dim < 3; dim++) {
    /* sweep right to left and determine bounds. */
    BoundBox right_bounds = BoundBox::empty;
    for (int i = BVHParams::NUM_SPATIAL_BINS - 1; i > 0; i--) {
      right_bounds.grow(storage_->bins[dim][i].bounds);
      storage_->right_bounds[i - 1] = right_bounds;
    }

    /* sweep left to right and select lowest SAH. */
    BoundBox left_bounds = BoundBox::empty;
    int leftNum = 0;
    int rightNum = range.size();

    for (int i = 1; i < BVHParams::NUM_SPATIAL_BINS; i++) {
      left_bounds.grow(storage_->bins[dim][i - 1].bounds);
      leftNum += storage_->bins[dim][i - 1].enter;
      rightNum -= storage_->bins[dim][i - 1].exit;

      const float sah = nodeSAH +
                        left_bounds.safe_area() * builder.params.primitive_cost(leftNum) +
                        storage_->right_bounds[i - 1].safe_area() *
                            builder.params.primitive_cost(rightNum);

      if (sah < this->sah) {
        this->sah = sah;
        this->dim = dim;
        this->pos = origin[dim] + binSize[dim] * (float)i;
      }
    }
  }
}

void BVHSpatialSplit::split(BVHBuild *builder,
                            BVHRange &left,
                            BVHRange &right,
                            const BVHRange &range)
{
  /* Categorize references and compute bounds.
   *
   * Left-hand side:          [left_start, left_end[
   * Uncategorized/split:     [left_end, right_start[
   * Right-hand side:         [right_start, refs.size()[ */

  vector<BVHReference> &refs = *references_;
  const int left_start = range.start();
  int left_end = left_start;
  int right_start = range.end();
  int right_end = range.end();
  BoundBox left_bounds = BoundBox::empty;
  BoundBox right_bounds = BoundBox::empty;

  for (int i = left_end; i < right_start; i++) {
    BoundBox prim_bounds = get_prim_bounds(refs[i]);
    if (prim_bounds.max[this->dim] <= this->pos) {
      /* entirely on the left-hand side */
      left_bounds.grow(prim_bounds);
      swap(refs[i], refs[left_end++]);
    }
    else if (prim_bounds.min[this->dim] >= this->pos) {
      /* entirely on the right-hand side */
      right_bounds.grow(prim_bounds);
      swap(refs[i--], refs[--right_start]);
    }
  }

  /* Duplicate or unsplit references intersecting both sides.
   *
   * Duplication happens into a temporary pre-allocated vector in order to
   * reduce number of memmove() calls happening in vector.insert().
   */
  vector<BVHReference> &new_refs = storage_->new_references;
  new_refs.clear();
  new_refs.reserve(right_start - left_end);
  while (left_end < right_start) {
    /* split reference. */
    const BVHReference curr_ref(get_prim_bounds(refs[left_end]),
                                refs[left_end].prim_index(),
                                refs[left_end].prim_object(),
                                refs[left_end].prim_type());
    BVHReference lref;
    BVHReference rref;
    split_reference(*builder, lref, rref, curr_ref, this->dim, this->pos);

    /* compute SAH for duplicate/unsplit candidates. */
    BoundBox lub = left_bounds;   // Unsplit to left:     new left-hand bounds.
    BoundBox rub = right_bounds;  // Unsplit to right:    new right-hand bounds.
    BoundBox ldb = left_bounds;   // Duplicate:           new left-hand bounds.
    BoundBox rdb = right_bounds;  // Duplicate:           new right-hand bounds.

    lub.grow(curr_ref.bounds());
    rub.grow(curr_ref.bounds());
    ldb.grow(lref.bounds());
    rdb.grow(rref.bounds());

    const float lac = builder->params.primitive_cost(left_end - left_start);
    const float rac = builder->params.primitive_cost(right_end - right_start);
    const float lbc = builder->params.primitive_cost(left_end - left_start + 1);
    const float rbc = builder->params.primitive_cost(right_end - right_start + 1);

    const float unsplitLeftSAH = lub.safe_area() * lbc + right_bounds.safe_area() * rac;
    const float unsplitRightSAH = left_bounds.safe_area() * lac + rub.safe_area() * rbc;
    const float duplicateSAH = ldb.safe_area() * lbc + rdb.safe_area() * rbc;
    const float minSAH = min(min(unsplitLeftSAH, unsplitRightSAH), duplicateSAH);

    if (minSAH == unsplitLeftSAH) {
      /* unsplit to left */
      left_bounds = lub;
      left_end++;
    }
    else if (minSAH == unsplitRightSAH) {
      /* unsplit to right */
      right_bounds = rub;
      swap(refs[left_end], refs[--right_start]);
    }
    else {
      /* duplicate */
      left_bounds = ldb;
      right_bounds = rdb;
      refs[left_end++] = lref;
      new_refs.push_back(rref);
      right_end++;
    }
  }
  /* Insert duplicated references into actual array in one go. */
  if (!new_refs.empty()) {
    refs.insert(refs.begin() + (right_end - new_refs.size()), new_refs.begin(), new_refs.end());
  }
  if (aligned_space_ != nullptr) {
    left_bounds = right_bounds = BoundBox::empty;
    for (int i = left_start; i < left_end - left_start; ++i) {
      const BoundBox prim_boundbox = references_->at(i).bounds();
      left_bounds.grow(prim_boundbox);
    }
    for (int i = right_start; i < right_end - right_start; ++i) {
      const BoundBox prim_boundbox = references_->at(i).bounds();
      right_bounds.grow(prim_boundbox);
    }
  }
  left = BVHRange(left_bounds, left_start, left_end - left_start);
  right = BVHRange(right_bounds, right_start, right_end - right_start);
}

void BVHSpatialSplit::split_triangle_primitive(const Mesh *mesh,
                                               const Transform *tfm,
                                               const int prim_index,
                                               const int dim,
                                               const float pos,
                                               BoundBox &left_bounds,
                                               BoundBox &right_bounds)
{
  const Mesh::Triangle t = mesh->get_triangle(prim_index);
  const float3 *verts = mesh->verts.data();
  float3 v1 = tfm ? transform_point(tfm, verts[t.v[2]]) : verts[t.v[2]];
  v1 = get_unaligned_point(v1);

  for (int i = 0; i < 3; i++) {
    float3 v0 = v1;
    const int vindex = t.v[i];
    v1 = tfm ? transform_point(tfm, verts[vindex]) : verts[vindex];
    v1 = get_unaligned_point(v1);
    const float v0p = v0[dim];
    const float v1p = v1[dim];

    /* insert vertex to the boxes it belongs to. */
    if (v0p <= pos) {
      left_bounds.grow(v0);
    }

    if (v0p >= pos) {
      right_bounds.grow(v0);
    }

    /* edge intersects the plane => insert intersection to both boxes. */
    if ((v0p < pos && v1p > pos) || (v0p > pos && v1p < pos)) {
      const float3 t = mix(v0, v1, clamp((pos - v0p) / (v1p - v0p), 0.0f, 1.0f));
      left_bounds.grow(t);
      right_bounds.grow(t);
    }
  }
}

void BVHSpatialSplit::split_curve_primitive(const Hair *hair,
                                            const Transform *tfm,
                                            const int prim_index,
                                            const int segment_index,
                                            const int dim,
                                            const float pos,
                                            BoundBox &left_bounds,
                                            BoundBox &right_bounds)
{
  /* curve split: NOTE - Currently ignores curve width and needs to be fixed. */
  const Hair::Curve curve = hair->get_curve(prim_index);
  const int k0 = curve.first_key + segment_index;
  const int k1 = k0 + 1;
  float3 v0 = hair->get_curve_keys()[k0];
  float3 v1 = hair->get_curve_keys()[k1];

  if (tfm != nullptr) {
    v0 = transform_point(tfm, v0);
    v1 = transform_point(tfm, v1);
  }
  v0 = get_unaligned_point(v0);
  v1 = get_unaligned_point(v1);

  const float v0p = v0[dim];
  const float v1p = v1[dim];

  /* insert vertex to the boxes it belongs to. */
  if (v0p <= pos) {
    left_bounds.grow(v0);
  }

  if (v0p >= pos) {
    right_bounds.grow(v0);
  }

  if (v1p <= pos) {
    left_bounds.grow(v1);
  }

  if (v1p >= pos) {
    right_bounds.grow(v1);
  }

  /* edge intersects the plane => insert intersection to both boxes. */
  if ((v0p < pos && v1p > pos) || (v0p > pos && v1p < pos)) {
    const float3 t = mix(v0, v1, clamp((pos - v0p) / (v1p - v0p), 0.0f, 1.0f));
    left_bounds.grow(t);
    right_bounds.grow(t);
  }
}

void BVHSpatialSplit::split_point_primitive(const PointCloud *pointcloud,
                                            const Transform *tfm,
                                            const int prim_index,
                                            const int dim,
                                            const float pos,
                                            BoundBox &left_bounds,
                                            BoundBox &right_bounds)
{
  /* No real splitting support for points, assume they are small enough for it
   * not to matter. */
  float3 point = pointcloud->get_points()[prim_index];
  const float radius = pointcloud->get_radius()[prim_index];

  if (tfm != nullptr) {
    point = transform_point(tfm, point);
  }
  point = get_unaligned_point(point);

  if (point[dim] <= pos) {
    left_bounds.grow(point, radius);
  }

  if (point[dim] >= pos) {
    right_bounds.grow(point, radius);
  }
}

void BVHSpatialSplit::split_triangle_reference(const BVHReference &ref,
                                               const Mesh *mesh,
                                               const int dim,
                                               const float pos,
                                               BoundBox &left_bounds,
                                               BoundBox &right_bounds)
{
  split_triangle_primitive(mesh, nullptr, ref.prim_index(), dim, pos, left_bounds, right_bounds);
}

void BVHSpatialSplit::split_curve_reference(const BVHReference &ref,
                                            const Hair *hair,
                                            const int dim,
                                            const float pos,
                                            BoundBox &left_bounds,
                                            BoundBox &right_bounds)
{
  split_curve_primitive(hair,
                        nullptr,
                        ref.prim_index(),
                        PRIMITIVE_UNPACK_SEGMENT(ref.prim_type()),
                        dim,
                        pos,
                        left_bounds,
                        right_bounds);
}

void BVHSpatialSplit::split_point_reference(const BVHReference &ref,
                                            const PointCloud *pointcloud,
                                            const int dim,
                                            const float pos,
                                            BoundBox &left_bounds,
                                            BoundBox &right_bounds)
{
  split_point_primitive(
      pointcloud, nullptr, ref.prim_index(), dim, pos, left_bounds, right_bounds);
}

void BVHSpatialSplit::split_object_reference(const Object *object,
                                             const int dim,
                                             const float pos,
                                             BoundBox &left_bounds,
                                             BoundBox &right_bounds)
{
  Geometry *geom = object->get_geometry();

  if (geom->is_mesh() || geom->is_volume()) {
    Mesh *mesh = static_cast<Mesh *>(geom);
    for (int tri_idx = 0; tri_idx < mesh->num_triangles(); ++tri_idx) {
      split_triangle_primitive(
          mesh, &object->get_tfm(), tri_idx, dim, pos, left_bounds, right_bounds);
    }
  }
  else if (geom->is_hair()) {
    Hair *hair = static_cast<Hair *>(geom);
    for (int curve_idx = 0; curve_idx < hair->num_curves(); ++curve_idx) {
      const Hair::Curve curve = hair->get_curve(curve_idx);
      for (int segment_idx = 0; segment_idx < curve.num_keys - 1; ++segment_idx) {
        split_curve_primitive(
            hair, &object->get_tfm(), curve_idx, segment_idx, dim, pos, left_bounds, right_bounds);
      }
    }
  }
  else if (geom->is_pointcloud()) {
    PointCloud *pointcloud = static_cast<PointCloud *>(geom);
    for (int point_idx = 0; point_idx < pointcloud->num_points(); ++point_idx) {
      split_point_primitive(
          pointcloud, &object->get_tfm(), point_idx, dim, pos, left_bounds, right_bounds);
    }
  }
}

void BVHSpatialSplit::split_reference(const BVHBuild &builder,
                                      BVHReference &left,
                                      BVHReference &right,
                                      const BVHReference &ref,
                                      const int dim,
                                      const float pos)
{
  /* Initialize bounding-boxes. */
  BoundBox left_bounds = BoundBox::empty;
  BoundBox right_bounds = BoundBox::empty;

  /* loop over vertices/edges. */
  const Object *ob = builder.objects[ref.prim_object()];

  if (ref.prim_type() & PRIMITIVE_TRIANGLE) {
    Mesh *mesh = static_cast<Mesh *>(ob->get_geometry());
    split_triangle_reference(ref, mesh, dim, pos, left_bounds, right_bounds);
  }
  else if (ref.prim_type() & PRIMITIVE_CURVE) {
    Hair *hair = static_cast<Hair *>(ob->get_geometry());
    split_curve_reference(ref, hair, dim, pos, left_bounds, right_bounds);
  }
  else if (ref.prim_type() & PRIMITIVE_POINT) {
    PointCloud *pointcloud = static_cast<PointCloud *>(ob->get_geometry());
    split_point_reference(ref, pointcloud, dim, pos, left_bounds, right_bounds);
  }
  else {
    split_object_reference(ob, dim, pos, left_bounds, right_bounds);
  }

  /* intersect with original bounds. */
  left_bounds.max[dim] = pos;
  right_bounds.min[dim] = pos;

  left_bounds.intersect(ref.bounds());
  right_bounds.intersect(ref.bounds());

  /* set references */
  left = BVHReference(left_bounds, ref.prim_index(), ref.prim_object(), ref.prim_type());
  right = BVHReference(right_bounds, ref.prim_index(), ref.prim_object(), ref.prim_type());
}

}
