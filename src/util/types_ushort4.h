/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/types_base.h"

namespace ccl {

#ifndef __KERNEL_NATIVE_VECTOR_TYPES__

struct ushort4 {
  uint16_t x, y, z, w;
};

#endif

}
