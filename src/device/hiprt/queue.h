/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#ifdef WITH_HIPRT

#  include "device/memory.h"
#  include "device/queue.h"

#  include "device/hip/queue.h"

namespace ccl {

class HIPRTDevice;

class HIPRTDeviceQueue : public HIPDeviceQueue {
 public:
  HIPRTDeviceQueue(HIPRTDevice *device);
  ~HIPRTDeviceQueue() override = default;
  bool enqueue(DeviceKernel kernel,
               const int work_size,
               const DeviceKernelArguments &args) override;

 protected:
  HIPRTDevice *hiprt_device_;
};

}

#endif /* WITH_HIPRT */
