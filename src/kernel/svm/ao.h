/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "kernel/globals.h"

#include "kernel/integrator/path_state.h"

#include "kernel/bvh/bvh.h"

#include "kernel/sample/mapping.h"

#include "kernel/svm/util.h"

namespace ccl {

#ifdef __SHADER_RAYTRACE__

#  ifdef __KERNEL_OPTIX__
extern "C" __device__ float __direct_callable__svm_node_ao(
#  else
ccl_device float svm_ao(
#  endif
    KernelGlobals kg,
    ConstIntegratorState state,
    ccl_private ShaderData *sd,
    float3 N,
    float max_dist,
    const int num_samples,
    const int flags)
{
  if (flags & NODE_AO_GLOBAL_RADIUS) {
    max_dist = kernel_data.integrator.ao_bounces_distance;
  }

  /* Early out if no sampling needed. */
  if (max_dist <= 0.0f || num_samples < 1 || sd->object == OBJECT_NONE) {
    return 1.0f;
  }

  /* Can't ray-trace from shaders like displacement, before BVH exists. */
  if (kernel_data.bvh.bvh_layout == BVH_LAYOUT_NONE) {
    return 1.0f;
  }

  if (flags & NODE_AO_INSIDE) {
    N = -N;
  }

  float3 T;
  float3 B;
  make_orthonormals(N, &T, &B);

  /* TODO: support ray-tracing in shadow shader evaluation? */
  RNGState rng_state;
  path_state_rng_load(state, &rng_state);

  int unoccluded = 0;
  for (int sample = 0; sample < num_samples; sample++) {
    const float2 rand_disk = path_branched_rng_2D(
        kg, &rng_state, sample, num_samples, PRNG_SURFACE_AO);

    const float2 d = sample_uniform_disk(rand_disk);
    const float3 D = make_float3(d.x, d.y, safe_sqrtf(1.0f - dot(d, d)));

    /* Create ray. */
    Ray ray;
    ray.P = sd->P;
    ray.D = to_global(D, T, B, N);
    ray.tmin = 0.0f;
    ray.tmax = max_dist;
    ray.time = sd->time;
    ray.self.object = sd->object;
    ray.self.prim = sd->prim;
    ray.self.light_object = OBJECT_NONE;
    ray.self.light_prim = PRIM_NONE;
    ray.dP = differential_zero_compact();
    ray.dD = differential_zero_compact();

    if (flags & NODE_AO_ONLY_LOCAL) {
      if (!scene_intersect_local(kg, &ray, nullptr, sd->object, nullptr, 0)) {
        unoccluded++;
      }
    }
    else {
      if (!scene_intersect_shadow(kg, &ray, PATH_RAY_SHADOW_OPAQUE)) {
        unoccluded++;
      }
    }
  }

  return ((float)unoccluded) / num_samples;
}

template<uint node_feature_mask, typename ConstIntegratorGenericState>
#  if defined(__KERNEL_OPTIX__)
ccl_device_inline
#  else
ccl_device_noinline
#  endif
    void
    svm_node_ao(KernelGlobals kg,
                ConstIntegratorGenericState state,
                ccl_private ShaderData *sd,
                ccl_private float *stack,
                const uint4 node)
{
  uint flags;
  uint dist_offset;
  uint normal_offset;
  uint out_ao_offset;
  svm_unpack_node_uchar4(node.y, &flags, &dist_offset, &normal_offset, &out_ao_offset);

  uint color_offset;
  uint out_color_offset;
  uint samples;
  svm_unpack_node_uchar3(node.z, &color_offset, &out_color_offset, &samples);

  float ao = 1.0f;

  IF_KERNEL_NODES_FEATURE(RAYTRACE)
  {
    float dist = stack_load_float_default(stack, dist_offset, node.w);
    float3 normal = stack_valid(normal_offset) ? stack_load_float3(stack, normal_offset) : sd->N;

#  ifdef __KERNEL_OPTIX__
    ao = optixDirectCall<float>(0, kg, state, sd, normal, dist, samples, flags);
#  else
    ao = svm_ao(kg, state, sd, normal, dist, samples, flags);
#  endif
  }

  if (stack_valid(out_ao_offset)) {
    stack_store_float(stack, out_ao_offset, ao);
  }

  if (stack_valid(out_color_offset)) {
    const float3 color = stack_load_float3(stack, color_offset);
    stack_store_float3(stack, out_color_offset, ao * color);
  }
}

#endif /* __SHADER_RAYTRACE__ */

}
