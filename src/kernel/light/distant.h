/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "kernel/geom/object.h"

#include "kernel/light/common.h"

#include "util/math_fast.h"

namespace ccl {

ccl_device_inline void distant_light_uv(KernelGlobals kg,
                                        const ccl_global KernelLight *klight,
                                        const float3 D,
                                        ccl_private float *u,
                                        ccl_private float *v)
{
  /* Map direction (x, y, z) to disk [-0.5, 0.5]^2:
   * r^2 = (1 - z) / (1 - cos(klight->distant.angle))
   * u_ = 0.5 * x * r / sin_angle(D, -klight->co)
   * v_ = 0.5 * y * r / sin_angle(D, -klight->co) */
  const float fac = klight->distant.half_inv_sin_half_angle / len(D - klight->co);

  /* Get u axis and v axis. */
  const Transform itfm = lamp_get_inverse_transform(kg, klight);
  const float u_ = dot(D, make_float3(itfm.x)) * fac;
  const float v_ = dot(D, make_float3(itfm.y)) * fac;

  /* NOTE: Return barycentric coordinates in the same notation as Embree and OptiX. */
  *u = v_ + 0.5f;
  *v = -u_ - v_;
}

ccl_device_inline bool distant_light_sample(KernelGlobals kg,
                                            const ccl_global KernelLight *klight,
                                            const float2 rand,
                                            ccl_private LightSample *ls)
{
  float unused;
  ls->Ng = sample_uniform_cone(
      klight->co, klight->distant.one_minus_cosangle, rand, &unused, &ls->pdf);

  ls->P = ls->Ng;
  ls->D = -ls->Ng;
  ls->t = FLT_MAX;

  ls->eval_fac = klight->distant.eval_fac;

  distant_light_uv(kg, klight, ls->D, &ls->u, &ls->v);

  return true;
}

/* Special intersection check.
 * Returns true if the distant_light_sample_from_intersection() for this light would return true.
 *
 * The intersection parameters t, u, v are optimized for the shadow ray towards a dedicated light:
 * u = v = 0, t = FLT_MAX.
 */
ccl_device bool distant_light_intersect(const ccl_global KernelLight *klight,
                                        const ccl_private Ray *ccl_restrict ray,
                                        ccl_private float *t,
                                        ccl_private float *u,
                                        ccl_private float *v)
{
  kernel_assert(klight->type == LIGHT_DISTANT);

  if (klight->distant.angle == 0.0f) {
    return false;
  }

  if (vector_angle(-klight->co, ray->D) > klight->distant.angle) {
    return false;
  }

  *t = FLT_MAX;
  *u = 0.0f;
  *v = 0.0f;

  return true;
}

ccl_device bool distant_light_sample_from_intersection(KernelGlobals kg,
                                                       const float3 ray_D,
                                                       const int lamp,
                                                       ccl_private LightSample *ccl_restrict ls)
{
  const ccl_global KernelLight *klight = &kernel_data_fetch(lights, lamp);
  const int shader = klight->shader_id;
  const LightType type = (LightType)klight->type;

  if (type != LIGHT_DISTANT) {
    return false;
  }
  if (!(shader & SHADER_USE_MIS)) {
    return false;
  }
  if (klight->distant.angle == 0.0f) {
    return false;
  }

  /* Workaround to prevent a hang in the classroom scene with AMD HIP drivers 22.10,
   * Remove when a compiler fix is available. */
#ifdef __HIP__
  ls->shader = klight->shader_id;
#endif

  if (vector_angle(-klight->co, ray_D) > klight->distant.angle) {
    return false;
  }

  ls->type = type;
#ifndef __HIP__
  ls->shader = klight->shader_id;
#endif
  ls->object = klight->object_id;
  ls->prim = lamp;
  ls->t = FLT_MAX;
  ls->P = -ray_D;
  ls->Ng = -ray_D;
  ls->D = ray_D;
  ls->group = object_lightgroup(kg, ls->object);

  ls->pdf = klight->distant.pdf;
  ls->eval_fac = klight->distant.eval_fac;

  distant_light_uv(kg, klight, ray_D, &ls->u, &ls->v);

  return true;
}

template<bool in_volume_segment>
ccl_device_forceinline bool distant_light_tree_parameters(const float3 centroid,
                                                          const float theta_e,
                                                          const float t,
                                                          ccl_private float &cos_theta_u,
                                                          ccl_private float2 &distance,
                                                          ccl_private float3 &point_to_centroid,
                                                          ccl_private float &theta_d)
{
  if (in_volume_segment) {
    if (t == FLT_MAX) {
      /* In world volumes, distant lights can contribute to the lighting of the volume with
       * specific configurations of procedurally generated volumes. Use a ray length of 1.0 in this
       * case to give the distant light some weight, but one that isn't too high for a typical
       * world volume use case. */
      theta_d = 1.0f;
    }
    else {
      theta_d = t;
    }
  }

  /* Treating it as a disk light 1 unit away */
  cos_theta_u = fast_cosf(theta_e);

  distance = make_float2(1.0f / cos_theta_u, 1.0f);

  point_to_centroid = -centroid;

  return true;
}

}
