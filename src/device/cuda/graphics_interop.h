/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifdef WITH_CUDA

#  include "device/graphics_interop.h"

#  ifdef WITH_CUDA_DYNLOAD
#    include "cuew.h"
#  else
#    include <cuda.h>
#  endif

namespace ccl {

class CUDADevice;
class CUDADeviceQueue;

class CUDADeviceGraphicsInterop : public DeviceGraphicsInterop {
 public:
  explicit CUDADeviceGraphicsInterop(CUDADeviceQueue *queue);

  CUDADeviceGraphicsInterop(const CUDADeviceGraphicsInterop &other) = delete;
  CUDADeviceGraphicsInterop(CUDADeviceGraphicsInterop &&other) noexcept = delete;

  ~CUDADeviceGraphicsInterop() override;

  CUDADeviceGraphicsInterop &operator=(const CUDADeviceGraphicsInterop &other) = delete;
  CUDADeviceGraphicsInterop &operator=(CUDADeviceGraphicsInterop &&other) = delete;

  void set_display_interop(const DisplayDriver::GraphicsInterop &display_interop) override;

  device_ptr map() override;
  void unmap() override;

 protected:
  CUDADeviceQueue *queue_ = nullptr;
  CUDADevice *device_ = nullptr;

  /* OpenGL PBO which is currently registered as the destination for the CUDA buffer. */
  int64_t opengl_pbo_id_ = 0;
  /* Buffer area in pixels of the corresponding PBO. */
  int64_t buffer_area_ = 0;

  /* The destination was requested to be cleared. */
  bool need_clear_ = false;

  CUgraphicsResource cu_graphics_resource_ = nullptr;
};

}

#endif
