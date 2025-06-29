/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "integrator/denoiser.h"
#include "util/thread.h"

namespace ccl {

/* Implementation of a CPU based denoiser which uses OpenImageDenoise library. */
class OIDNDenoiser : public Denoiser {
 public:
  /* Forwardly declared state which might be using compile-flag specific fields, such as
   * OpenImageDenoise device and filter handles. */
  class State;

  OIDNDenoiser(Device *denoiser_device, const DenoiseParams &params);

  bool denoise_buffer(const BufferParams &buffer_params,
                      RenderBuffers *render_buffers,
                      const int num_samples,
                      bool allow_inplace_modification) override;

 protected:
  uint get_device_type_mask() const override;

  /* We only perform one denoising at a time, since OpenImageDenoise itself is multithreaded.
   * Use this mutex whenever images are passed to the OIDN and needs to be denoised. */
  static thread_mutex mutex_;
};

}
