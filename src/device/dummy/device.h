/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/unique_ptr.h"

namespace ccl {

class Device;
class DeviceInfo;
class Profiler;
class Stats;

unique_ptr<Device> device_dummy_create(const DeviceInfo &info,
                                       Stats &stats,
                                       Profiler &profiler,
                                       bool headless);

}
