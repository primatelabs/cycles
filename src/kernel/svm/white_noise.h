/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "kernel/svm/util.h"
#include "util/hash.h"

namespace ccl {

ccl_device_noinline void svm_node_tex_white_noise(KernelGlobals kg,
                                                  ccl_private ShaderData *sd,
                                                  ccl_private float *stack,
                                                  const uint dimensions,
                                                  const uint inputs_stack_offsets,
                                                  const uint outputs_stack_offsets)
{
  uint vector_stack_offset;
  uint w_stack_offset;
  uint value_stack_offset;
  uint color_stack_offset;
  svm_unpack_node_uchar2(inputs_stack_offsets, &vector_stack_offset, &w_stack_offset);
  svm_unpack_node_uchar2(outputs_stack_offsets, &value_stack_offset, &color_stack_offset);

  const float3 vector = stack_load_float3(stack, vector_stack_offset);
  const float w = stack_load_float(stack, w_stack_offset);

  if (stack_valid(color_stack_offset)) {
    float3 color;
    switch (dimensions) {
      case 1:
        color = hash_float_to_float3(w);
        break;
      case 2:
        color = hash_float2_to_float3(make_float2(vector.x, vector.y));
        break;
      case 3:
        color = hash_float3_to_float3(vector);
        break;
      case 4:
        color = hash_float4_to_float3(make_float4(vector, w));
        break;
      default:
        color = make_float3(1.0f, 0.0f, 1.0f);
        kernel_assert(0);
        break;
    }
    stack_store_float3(stack, color_stack_offset, color);
  }

  if (stack_valid(value_stack_offset)) {
    float value;
    switch (dimensions) {
      case 1:
        value = hash_float_to_float(w);
        break;
      case 2:
        value = hash_float2_to_float(make_float2(vector.x, vector.y));
        break;
      case 3:
        value = hash_float3_to_float(vector);
        break;
      case 4:
        value = hash_float4_to_float(make_float4(vector, w));
        break;
      default:
        value = 0.0f;
        kernel_assert(0);
        break;
    }
    stack_store_float(stack, value_stack_offset, value);
  }
}

}
