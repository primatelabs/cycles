/* SPDX-FileCopyrightText: 2021-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifdef WITH_METAL

#  include "device/metal/bvh.h"

#  include "util/unique_ptr.h"

namespace ccl {

unique_ptr<BVH> bvh_metal_create(const BVHParams &params,
                                 const vector<Geometry *> &geometry,
                                 const vector<Object *> &objects,
                                 Device *device)
{
  return make_unique<BVHMetal>(params, geometry, objects, device);
}

}

#endif /* WITH_METAL */
