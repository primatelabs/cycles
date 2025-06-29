/* SPDX-FileCopyrightText: 2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#ifdef WITH_PATH_GUIDING
#  include <openpgl/cpp/OpenPGL.h>  // IWYU pragma: export
#  include <openpgl/version.h>      // IWYU pragma: export
#endif

#include "util/system.h"  // IWYU pragma: keep

namespace ccl {

static int guiding_device_type()
{
#ifdef WITH_PATH_GUIDING
#  if defined(__ARM_NEON)
  return 8;
#  else
  if (system_cpu_support_avx2()) {
    return 8;
  }
  if (system_cpu_support_sse42()) {
    return 4;
  }
  return 0;
#  endif
#else
  return 0;
#endif
}

static inline bool guiding_supported()
{
  return guiding_device_type() != 0;
}

}
