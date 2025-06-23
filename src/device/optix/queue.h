/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#ifdef WITH_OPTIX

#  include "device/cuda/queue.h"

namespace ccl {

class OptiXDevice;

/* Base class for CUDA queues. */
class OptiXDeviceQueue : public CUDADeviceQueue {
 public:
  OptiXDeviceQueue(OptiXDevice *device);

  void init_execution() override;

  bool enqueue(DeviceKernel kernel,
               const int work_size,
               const DeviceKernelArguments &args) override;
};

}

#endif /* WITH_OPTIX */
