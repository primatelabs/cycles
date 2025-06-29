/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "device/queue.h"

#include "integrator/pass_accessor_gpu.h"

#include "session/buffers.h"

namespace ccl {

PassAccessorGPU::PassAccessorGPU(DeviceQueue *queue,
                                 const PassAccessInfo &pass_access_info,
                                 const float exposure,
                                 const int num_samples)
    : PassAccessor(pass_access_info, exposure, num_samples), queue_(queue)
{
}

/* --------------------------------------------------------------------
 * Kernel execution.
 */

void PassAccessorGPU::run_film_convert_kernels(DeviceKernel kernel,
                                               const RenderBuffers *render_buffers,
                                               const BufferParams &buffer_params,
                                               const Destination &destination) const
{
  KernelFilmConvert kfilm_convert;
  init_kernel_film_convert(&kfilm_convert, buffer_params, destination);

  const int work_size = buffer_params.window_width * buffer_params.window_height;

  const int destination_stride = destination.stride != 0 ? destination.stride :
                                                           buffer_params.window_width;

  const int offset = buffer_params.window_x * buffer_params.pass_stride +
                     buffer_params.window_y * buffer_params.stride * buffer_params.pass_stride;

  queue_->init_execution();
  if (destination.d_pixels) {
    DCHECK_EQ(destination.stride, 0) << "Custom stride for float destination is not implemented.";

    const DeviceKernelArguments args(&kfilm_convert,
                                     &destination.d_pixels,
                                     &render_buffers->buffer.device_pointer,
                                     &work_size,
                                     &buffer_params.window_width,
                                     &offset,
                                     &buffer_params.stride,
                                     &destination.pixel_offset,
                                     &destination.offset,
                                     &destination_stride);

    queue_->enqueue(kernel, work_size, args);
  }
  if (destination.d_pixels_half_rgba) {
    const DeviceKernel kernel_half_float = static_cast<DeviceKernel>(kernel + 1);

    const DeviceKernelArguments args(&kfilm_convert,
                                     &destination.d_pixels_half_rgba,
                                     &render_buffers->buffer.device_pointer,
                                     &work_size,
                                     &buffer_params.window_width,
                                     &offset,
                                     &buffer_params.stride,
                                     &destination.offset,
                                     &destination_stride);

    queue_->enqueue(kernel_half_float, work_size, args);
  }

  queue_->synchronize();
}

/* --------------------------------------------------------------------
 * Pass accessors.
 */

#define DEFINE_PASS_ACCESSOR(pass, kernel_pass) \
  void PassAccessorGPU::get_pass_##pass(const RenderBuffers *render_buffers, \
                                        const BufferParams &buffer_params, \
                                        const Destination &destination) const \
  { \
    run_film_convert_kernels( \
        DEVICE_KERNEL_FILM_CONVERT_##kernel_pass, render_buffers, buffer_params, destination); \
  }

/* Float (scalar) passes. */
DEFINE_PASS_ACCESSOR(depth, DEPTH);
DEFINE_PASS_ACCESSOR(mist, MIST);
DEFINE_PASS_ACCESSOR(sample_count, SAMPLE_COUNT);
DEFINE_PASS_ACCESSOR(float, FLOAT);

/* Float3 passes. */
DEFINE_PASS_ACCESSOR(light_path, LIGHT_PATH);
DEFINE_PASS_ACCESSOR(float3, FLOAT3);

/* Float4 passes. */
DEFINE_PASS_ACCESSOR(motion, MOTION);
DEFINE_PASS_ACCESSOR(cryptomatte, CRYPTOMATTE);
DEFINE_PASS_ACCESSOR(shadow_catcher, SHADOW_CATCHER);
DEFINE_PASS_ACCESSOR(shadow_catcher_matte_with_shadow, SHADOW_CATCHER_MATTE_WITH_SHADOW);
DEFINE_PASS_ACCESSOR(combined, COMBINED);
DEFINE_PASS_ACCESSOR(float4, FLOAT4);

#undef DEFINE_PASS_ACCESSOR

}
