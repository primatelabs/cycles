/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/unique_ptr.h"
#include "util/vector.h"

namespace ccl {

class Device;
class DeviceInfo;
class Profiler;
class Stats;

bool device_optix_init();

unique_ptr<Device> device_optix_create(const DeviceInfo &info,
                                       Stats &stats,
                                       Profiler &profiler,
                                       bool headless);

void device_optix_info(const vector<DeviceInfo> &cuda_devices, vector<DeviceInfo> &devices);

}
