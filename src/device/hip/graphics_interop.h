/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#ifdef WITH_HIP

#  include "device/graphics_interop.h"

#  ifdef WITH_HIP_DYNLOAD
#    include "hipew.h"
#  endif

namespace ccl {

class HIPDevice;
class HIPDeviceQueue;

class HIPDeviceGraphicsInterop : public DeviceGraphicsInterop {
 public:
  explicit HIPDeviceGraphicsInterop(HIPDeviceQueue *queue);

  HIPDeviceGraphicsInterop(const HIPDeviceGraphicsInterop &other) = delete;
  HIPDeviceGraphicsInterop(HIPDeviceGraphicsInterop &&other) noexcept = delete;

  ~HIPDeviceGraphicsInterop() override;

  HIPDeviceGraphicsInterop &operator=(const HIPDeviceGraphicsInterop &other) = delete;
  HIPDeviceGraphicsInterop &operator=(HIPDeviceGraphicsInterop &&other) = delete;

  void set_display_interop(const DisplayDriver::GraphicsInterop &display_interop) override;

  device_ptr map() override;
  void unmap() override;

 protected:
  HIPDeviceQueue *queue_ = nullptr;
  HIPDevice *device_ = nullptr;

  /* OpenGL PBO which is currently registered as the destination for the HIP buffer. */
  int64_t opengl_pbo_id_ = 0;
  /* Buffer area in pixels of the corresponding PBO. */
  int64_t buffer_area_ = 0;

  /* The destination was requested to be cleared. */
  bool need_clear_ = false;

  hipGraphicsResource hip_graphics_resource_ = nullptr;
};

}

#endif
