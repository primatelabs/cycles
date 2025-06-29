/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

/* Templated common implementation part of all CPU kernels.
 *
 * The idea is that particular .cpp files sets needed optimization flags and
 * simply includes this file without worry of copying actual implementation over.
 */

#pragma once

// clang-format off
#include "kernel/device/cpu/compat.h"

#ifndef KERNEL_STUB
#    include "kernel/globals.h"

#    include "kernel/device/cpu/image.h"

#    include "kernel/integrator/state.h"
#    include "kernel/integrator/state_flow.h"
#    include "kernel/integrator/state_util.h"

#    include "kernel/integrator/init_from_camera.h"
#    include "kernel/integrator/init_from_bake.h"
#    include "kernel/integrator/megakernel.h"

#    include "kernel/film/adaptive_sampling.h"
#    include "kernel/film/cryptomatte_passes.h"
#    include "kernel/film/read.h"

#    include "kernel/bake/bake.h"

#else
#  define STUB_ASSERT(arch, name) \
    assert(!(#name " kernel stub for architecture " #arch " was called!"))
#endif   /* KERNEL_STUB */
// clang-format on

namespace ccl {

/* --------------------------------------------------------------------
 * Integrator.
 */

#ifdef KERNEL_STUB
#  define KERNEL_INVOKE(name, ...) (STUB_ASSERT(KERNEL_ARCH, name), 0)
#else
#  define KERNEL_INVOKE(name, ...) integrator_##name(__VA_ARGS__)
#endif

/* TODO: Either use something like get_work_pixel(), or simplify tile which is passed here, so
 * that it does not contain unused fields. */
#define DEFINE_INTEGRATOR_INIT_KERNEL(name) \
  bool KERNEL_FUNCTION_FULL_NAME(integrator_##name)(const ThreadKernelGlobalsCPU *kg, \
                                                    IntegratorStateCPU *state, \
                                                    KernelWorkTile *tile, \
                                                    ccl_global float *render_buffer) \
  { \
    return KERNEL_INVOKE( \
        name, kg, state, tile, render_buffer, tile->x, tile->y, tile->start_sample); \
  }

#define DEFINE_INTEGRATOR_KERNEL(name) \
  void KERNEL_FUNCTION_FULL_NAME(integrator_##name)(const ThreadKernelGlobalsCPU *kg, \
                                                    IntegratorStateCPU *state) \
  { \
    KERNEL_INVOKE(name, kg, state); \
  }

#define DEFINE_INTEGRATOR_SHADE_KERNEL(name) \
  void KERNEL_FUNCTION_FULL_NAME(integrator_##name)(const ThreadKernelGlobalsCPU *kg, \
                                                    IntegratorStateCPU *state, \
                                                    ccl_global float *render_buffer) \
  { \
    KERNEL_INVOKE(name, kg, state, render_buffer); \
  }

#define DEFINE_INTEGRATOR_SHADOW_KERNEL(name) \
  void KERNEL_FUNCTION_FULL_NAME(integrator_##name)(const ThreadKernelGlobalsCPU *kg, \
                                                    IntegratorStateCPU *state) \
  { \
    KERNEL_INVOKE(name, kg, &state->shadow); \
  }

#define DEFINE_INTEGRATOR_SHADOW_SHADE_KERNEL(name) \
  void KERNEL_FUNCTION_FULL_NAME(integrator_##name)(const ThreadKernelGlobalsCPU *kg, \
                                                    IntegratorStateCPU *state, \
                                                    ccl_global float *render_buffer) \
  { \
    KERNEL_INVOKE(name, kg, &state->shadow, render_buffer); \
  }

DEFINE_INTEGRATOR_INIT_KERNEL(init_from_camera)
DEFINE_INTEGRATOR_INIT_KERNEL(init_from_bake)
DEFINE_INTEGRATOR_SHADE_KERNEL(megakernel)

/* --------------------------------------------------------------------
 * Shader evaluation.
 */

void KERNEL_FUNCTION_FULL_NAME(shader_eval_displace)(const ThreadKernelGlobalsCPU *kg,
                                                     const KernelShaderEvalInput *input,
                                                     float *output,
                                                     const int offset)
{
#ifdef KERNEL_STUB
  STUB_ASSERT(KERNEL_ARCH, shader_eval_displace);
#else
  kernel_displace_evaluate(kg, input, output, offset);
#endif
}

void KERNEL_FUNCTION_FULL_NAME(shader_eval_background)(const ThreadKernelGlobalsCPU *kg,
                                                       const KernelShaderEvalInput *input,
                                                       float *output,
                                                       const int offset)
{
#ifdef KERNEL_STUB
  STUB_ASSERT(KERNEL_ARCH, shader_eval_background);
#else
  kernel_background_evaluate(kg, input, output, offset);
#endif
}

void KERNEL_FUNCTION_FULL_NAME(shader_eval_curve_shadow_transparency)(
    const ThreadKernelGlobalsCPU *kg,
    const KernelShaderEvalInput *input,
    float *output,
    const int offset)
{
#ifdef KERNEL_STUB
  STUB_ASSERT(KERNEL_ARCH, shader_eval_curve_shadow_transparency);
#else
  kernel_curve_shadow_transparency_evaluate(kg, input, output, offset);
#endif
}

/* --------------------------------------------------------------------
 * Adaptive sampling.
 */

bool KERNEL_FUNCTION_FULL_NAME(adaptive_sampling_convergence_check)(
    const ThreadKernelGlobalsCPU *kg,
    ccl_global float *render_buffer,
    const int x,
    const int y,
    const float threshold,
    const int reset,
    const int offset,
    const int stride)
{
#ifdef KERNEL_STUB
  STUB_ASSERT(KERNEL_ARCH, adaptive_sampling_convergence_check);
  return false;
#else
  return film_adaptive_sampling_convergence_check(
      kg, render_buffer, x, y, threshold, reset, offset, stride);
#endif
}

void KERNEL_FUNCTION_FULL_NAME(adaptive_sampling_filter_x)(const ThreadKernelGlobalsCPU *kg,
                                                           ccl_global float *render_buffer,
                                                           const int y,
                                                           const int start_x,
                                                           const int width,
                                                           const int offset,
                                                           const int stride)
{
#ifdef KERNEL_STUB
  STUB_ASSERT(KERNEL_ARCH, adaptive_sampling_filter_x);
#else
  film_adaptive_sampling_filter_x(kg, render_buffer, y, start_x, width, offset, stride);
#endif
}

void KERNEL_FUNCTION_FULL_NAME(adaptive_sampling_filter_y)(const ThreadKernelGlobalsCPU *kg,
                                                           ccl_global float *render_buffer,
                                                           const int x,
                                                           const int start_y,
                                                           const int height,
                                                           const int offset,
                                                           const int stride)
{
#ifdef KERNEL_STUB
  STUB_ASSERT(KERNEL_ARCH, adaptive_sampling_filter_y);
#else
  film_adaptive_sampling_filter_y(kg, render_buffer, x, start_y, height, offset, stride);
#endif
}

/* --------------------------------------------------------------------
 * Cryptomatte.
 */

void KERNEL_FUNCTION_FULL_NAME(cryptomatte_postprocess)(const ThreadKernelGlobalsCPU *kg,
                                                        ccl_global float *render_buffer,
                                                        const int pixel_index)
{
#ifdef KERNEL_STUB
  STUB_ASSERT(KERNEL_ARCH, cryptomatte_postprocess);
#else
  film_cryptomatte_post(kg, render_buffer, pixel_index);
#endif
}

/* --------------------------------------------------------------------
 * Film Convert.
 */

#ifdef KERNEL_STUB

#  define KERNEL_FILM_CONVERT_FUNCTION(name, is_float) \
    void KERNEL_FUNCTION_FULL_NAME(film_convert_##name)(const KernelFilmConvert *kfilm_convert, \
                                                        const float *buffer, \
                                                        float *pixel, \
                                                        const int width, \
                                                        const int buffer_stride, \
                                                        const int pixel_stride) \
    { \
      STUB_ASSERT(KERNEL_ARCH, film_convert_##name); \
    } \
    void KERNEL_FUNCTION_FULL_NAME(film_convert_half_rgba_##name)( \
        const KernelFilmConvert *kfilm_convert, \
        const float *buffer, \
        half4 *pixel, \
        const int width, \
        const int buffer_stride) \
    { \
      STUB_ASSERT(KERNEL_ARCH, film_convert_##name); \
    }

#else

#  define KERNEL_FILM_CONVERT_FUNCTION(name, is_float) \
    void KERNEL_FUNCTION_FULL_NAME(film_convert_##name)(const KernelFilmConvert *kfilm_convert, \
                                                        const float *buffer, \
                                                        float *pixel, \
                                                        const int width, \
                                                        const int buffer_stride, \
                                                        const int pixel_stride) \
    { \
      for (int i = 0; i < width; i++, buffer += buffer_stride, pixel += pixel_stride) { \
        film_get_pass_pixel_##name(kfilm_convert, buffer, pixel); \
      } \
    } \
    void KERNEL_FUNCTION_FULL_NAME(film_convert_half_rgba_##name)( \
        const KernelFilmConvert *kfilm_convert, \
        const float *buffer, \
        half4 *pixel, \
        const int width, \
        const int buffer_stride) \
    { \
      for (int i = 0; i < width; i++, buffer += buffer_stride, pixel++) { \
        float pixel_rgba[4] = {0.0f, 0.0f, 0.0f, 1.0f}; \
        film_get_pass_pixel_##name(kfilm_convert, buffer, pixel_rgba); \
        if (is_float) { \
          pixel_rgba[1] = pixel_rgba[0]; \
          pixel_rgba[2] = pixel_rgba[0]; \
        } \
        film_apply_pass_pixel_overlays_rgba(kfilm_convert, buffer, pixel_rgba); \
        *pixel = float4_to_half4_display( \
            make_float4(pixel_rgba[0], pixel_rgba[1], pixel_rgba[2], pixel_rgba[3])); \
      } \
    }

#endif

KERNEL_FILM_CONVERT_FUNCTION(depth, true)
KERNEL_FILM_CONVERT_FUNCTION(mist, true)
KERNEL_FILM_CONVERT_FUNCTION(sample_count, true)
KERNEL_FILM_CONVERT_FUNCTION(float, true)

KERNEL_FILM_CONVERT_FUNCTION(light_path, false)
KERNEL_FILM_CONVERT_FUNCTION(float3, false)

KERNEL_FILM_CONVERT_FUNCTION(motion, false)
KERNEL_FILM_CONVERT_FUNCTION(cryptomatte, false)
KERNEL_FILM_CONVERT_FUNCTION(shadow_catcher, false)
KERNEL_FILM_CONVERT_FUNCTION(shadow_catcher_matte_with_shadow, false)
KERNEL_FILM_CONVERT_FUNCTION(combined, false)
KERNEL_FILM_CONVERT_FUNCTION(float4, false)

#undef KERNEL_FILM_CONVERT_FUNCTION

#undef KERNEL_INVOKE
#undef DEFINE_INTEGRATOR_KERNEL
#undef DEFINE_INTEGRATOR_SHADE_KERNEL
#undef DEFINE_INTEGRATOR_INIT_KERNEL

#undef KERNEL_STUB
#undef STUB_ASSERT
#undef KERNEL_ARCH

}
