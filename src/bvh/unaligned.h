/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/vector.h"

namespace ccl {

class BoundBox;
class BVHObjectBinning;
class BVHRange;
class BVHReference;
struct Transform;
class Object;

/* Helper class to perform calculations needed for unaligned nodes. */
class BVHUnaligned {
 public:
  BVHUnaligned(const vector<Object *> &objects);

  /* Calculate alignment for the oriented node for a given range. */
  Transform compute_aligned_space(const BVHObjectBinning &range,
                                  const BVHReference *references) const;
  Transform compute_aligned_space(const BVHRange &range, const BVHReference *references) const;

  /* Calculate alignment for the oriented node for a given reference.
   *
   * Return true when space was calculated successfully.
   */
  bool compute_aligned_space(const BVHReference &ref, Transform *aligned_space) const;

  /* Calculate primitive's bounding box in given space. */
  BoundBox compute_aligned_prim_boundbox(const BVHReference &prim,
                                         const Transform &aligned_space) const;

  /* Calculate bounding box in given space. */
  BoundBox compute_aligned_boundbox(const BVHObjectBinning &range,
                                    const BVHReference *references,
                                    const Transform &aligned_space,
                                    BoundBox *cent_bounds = nullptr) const;
  BoundBox compute_aligned_boundbox(const BVHRange &range,
                                    const BVHReference *references,
                                    const Transform &aligned_space,
                                    BoundBox *cent_bounds = nullptr) const;

  /* Calculate affine transform for node packing.
   * Bounds will be in the range of 0..1.
   */
  static Transform compute_node_transform(const BoundBox &bounds, const Transform &aligned_space);

 protected:
  /* List of objects BVH is being created for. */
  const vector<Object *> &objects_;
};

}
