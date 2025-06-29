/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "kernel/svm/util.h"

namespace ccl {

ccl_device_noinline int svm_node_normal(KernelGlobals kg,
                                        ccl_private ShaderData *sd,
                                        ccl_private float *stack,
                                        const uint in_normal_offset,
                                        const uint out_normal_offset,
                                        const uint out_dot_offset,
                                        int offset)
{
  /* read extra data */
  const uint4 node1 = read_node(kg, &offset);
  const float3 normal = stack_load_float3(stack, in_normal_offset);

  float3 direction;
  direction.x = __int_as_float(node1.x);
  direction.y = __int_as_float(node1.y);
  direction.z = __int_as_float(node1.z);
  direction = normalize(direction);

  if (stack_valid(out_normal_offset)) {
    stack_store_float3(stack, out_normal_offset, direction);
  }

  if (stack_valid(out_dot_offset)) {
    stack_store_float(stack, out_dot_offset, dot(direction, normalize(normal)));
  }
  return offset;
}

}
