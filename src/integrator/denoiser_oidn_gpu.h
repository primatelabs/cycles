/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#if defined(WITH_OPENIMAGEDENOISE)

#  include "integrator/denoiser_gpu.h"
#  include "util/openimagedenoise.h"  // IWYU pragma: keep

namespace ccl {

/* Implementation of a GPU denoiser which uses OpenImageDenoise library. */
class OIDNDenoiserGPU : public DenoiserGPU {
  friend class OIDNDenoiseContext;

 public:
  /* Forwardly declared state which might be using compile-flag specific fields, such as
   * OpenImageDenoise device and filter handles. */
  class State;

  OIDNDenoiserGPU(Device *denoiser_device, const DenoiseParams &params);
  ~OIDNDenoiserGPU() override;

  bool denoise_buffer(const BufferParams &buffer_params,
                      RenderBuffers *render_buffers,
                      const int num_samples,
                      bool allow_inplace_modification) override;

  static bool is_device_supported(const DeviceInfo &device);

 protected:
  enum class ExecMode {
    SYNC,
    ASYNC,
  };

  uint get_device_type_mask() const override;

  /* Create OIDN denoiser descriptor if needed.
   * Will do nothing if the current OIDN descriptor is usable for the given parameters.
   * If the OIDN denoiser descriptor did re-allocate here it is left unconfigured. */
  bool denoise_create_if_needed(DenoiseContext &context) override;

  /* Configure existing OIDN denoiser descriptor for the use for the given task. */
  bool denoise_configure_if_needed(DenoiseContext &context) override;

  /* Run configured denoiser. */
  bool denoise_run(const DenoiseContext &context, const DenoisePass &pass) override;

  OIDNFilter create_filter();
  bool commit_and_execute_filter(OIDNFilter filter, ExecMode mode = ExecMode::SYNC);

  void set_filter_pass(OIDNFilter filter,
                       const char *name,
                       device_ptr ptr,
                       const int format,
                       const int width,
                       const int height,
                       const size_t offset_in_bytes,
                       const size_t pixel_stride_in_bytes,
                       size_t row_stride_in_bytes);

  /* Delete all allocated OIDN objects. */
  void release_all_resources();

  OIDNDevice oidn_device_ = nullptr;
  OIDNFilter oidn_filter_ = nullptr;
  OIDNFilter albedo_filter_ = nullptr;
  OIDNFilter normal_filter_ = nullptr;

  bool is_configured_ = false;
  int2 configured_size_ = make_int2(0, 0);

  vector<uint8_t> custom_weights;

  bool use_pass_albedo_ = false;
  bool use_pass_normal_ = false;
  DenoiserQuality quality_ = DENOISER_QUALITY_HIGH;

  /* Filter memory usage limit if we ran out of memory with OIDN's default limit. */
  int max_mem_ = 768;
};

}

#endif
