/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "bvh/unaligned.h"

#include "scene/hair.h"
#include "scene/object.h"

#include "bvh/binning.h"
#include "bvh/params.h"

#include "util/boundbox.h"
#include "util/transform.h"

namespace ccl {

BVHUnaligned::BVHUnaligned(const vector<Object *> &objects) : objects_(objects) {}

Transform BVHUnaligned::compute_aligned_space(const BVHObjectBinning &range,
                                              const BVHReference *references) const
{
  for (int i = range.start(); i < range.end(); ++i) {
    const BVHReference &ref = references[i];
    Transform aligned_space;
    /* Use first primitive which defines correct direction to define
     * the orientation space.
     */
    if (compute_aligned_space(ref, &aligned_space)) {
      return aligned_space;
    }
  }
  return transform_identity();
}

Transform BVHUnaligned::compute_aligned_space(const BVHRange &range,
                                              const BVHReference *references) const
{
  for (int i = range.start(); i < range.end(); ++i) {
    const BVHReference &ref = references[i];
    Transform aligned_space;
    /* Use first primitive which defines correct direction to define
     * the orientation space.
     */
    if (compute_aligned_space(ref, &aligned_space)) {
      return aligned_space;
    }
  }
  return transform_identity();
}

bool BVHUnaligned::compute_aligned_space(const BVHReference &ref, Transform *aligned_space) const
{
  const Object *object = objects_[ref.prim_object()];
  const int packed_type = ref.prim_type();
  const int type = (packed_type & PRIMITIVE_ALL);
  /* No motion blur curves here, we can't fit them to aligned boxes well. */
  if ((type & PRIMITIVE_CURVE) && !(type & PRIMITIVE_MOTION)) {
    const int curve_index = ref.prim_index();
    const int segment = PRIMITIVE_UNPACK_SEGMENT(packed_type);
    const Hair *hair = static_cast<const Hair *>(object->get_geometry());
    const Hair::Curve &curve = hair->get_curve(curve_index);
    const int key = curve.first_key + segment;
    const float3 v1 = hair->get_curve_keys()[key];
    const float3 v2 = hair->get_curve_keys()[key + 1];
    float length;
    const float3 axis = normalize_len(v2 - v1, &length);
    if (length > 1e-6f) {
      *aligned_space = make_transform_frame(axis);
      return true;
    }
  }
  *aligned_space = transform_identity();
  return false;
}

BoundBox BVHUnaligned::compute_aligned_prim_boundbox(const BVHReference &prim,
                                                     const Transform &aligned_space) const
{
  BoundBox bounds = BoundBox::empty;
  const Object *object = objects_[prim.prim_object()];
  const int packed_type = prim.prim_type();
  const int type = (packed_type & PRIMITIVE_ALL);
  /* No motion blur curves here, we can't fit them to aligned boxes well. */
  if ((type & PRIMITIVE_CURVE) && !(type & PRIMITIVE_MOTION)) {
    const int curve_index = prim.prim_index();
    const int segment = PRIMITIVE_UNPACK_SEGMENT(packed_type);
    const Hair *hair = static_cast<const Hair *>(object->get_geometry());
    const Hair::Curve &curve = hair->get_curve(curve_index);
    curve.bounds_grow(segment,
                      hair->get_curve_keys().data(),
                      hair->get_curve_radius().data(),
                      aligned_space,
                      bounds);
  }
  else {
    bounds = prim.bounds().transformed(&aligned_space);
  }
  return bounds;
}

BoundBox BVHUnaligned::compute_aligned_boundbox(const BVHObjectBinning &range,
                                                const BVHReference *references,
                                                const Transform &aligned_space,
                                                BoundBox *cent_bounds) const
{
  BoundBox bounds = BoundBox::empty;
  if (cent_bounds != nullptr) {
    *cent_bounds = BoundBox::empty;
  }
  for (int i = range.start(); i < range.end(); ++i) {
    const BVHReference &ref = references[i];
    const BoundBox ref_bounds = compute_aligned_prim_boundbox(ref, aligned_space);
    bounds.grow(ref_bounds);
    if (cent_bounds != nullptr) {
      cent_bounds->grow(ref_bounds.center2());
    }
  }
  return bounds;
}

BoundBox BVHUnaligned::compute_aligned_boundbox(const BVHRange &range,
                                                const BVHReference *references,
                                                const Transform &aligned_space,
                                                BoundBox *cent_bounds) const
{
  BoundBox bounds = BoundBox::empty;
  if (cent_bounds != nullptr) {
    *cent_bounds = BoundBox::empty;
  }
  for (int i = range.start(); i < range.end(); ++i) {
    const BVHReference &ref = references[i];
    const BoundBox ref_bounds = compute_aligned_prim_boundbox(ref, aligned_space);
    bounds.grow(ref_bounds);
    if (cent_bounds != nullptr) {
      cent_bounds->grow(ref_bounds.center2());
    }
  }
  return bounds;
}

Transform BVHUnaligned::compute_node_transform(const BoundBox &bounds,
                                               const Transform &aligned_space)
{
  Transform space = aligned_space;
  space.x.w -= bounds.min.x;
  space.y.w -= bounds.min.y;
  space.z.w -= bounds.min.z;
  const float3 dim = bounds.max - bounds.min;
  return transform_scale(
             1.0f / max(1e-18f, dim.x), 1.0f / max(1e-18f, dim.y), 1.0f / max(1e-18f, dim.z)) *
         space;
}

}
