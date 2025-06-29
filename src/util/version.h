/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

/* Cycles version number */

namespace ccl {

// NOLINTBEGIN
#define CYCLES_VERSION_MAJOR 4
#define CYCLES_VERSION_MINOR 5
#define CYCLES_VERSION_PATCH 0
// NOLINTEND

#define CYCLES_MAKE_VERSION_STRING2(a, b, c) #a "." #b "." #c
#define CYCLES_MAKE_VERSION_STRING(a, b, c) CYCLES_MAKE_VERSION_STRING2(a, b, c)
#define CYCLES_VERSION_STRING \
  CYCLES_MAKE_VERSION_STRING(CYCLES_VERSION_MAJOR, CYCLES_VERSION_MINOR, CYCLES_VERSION_PATCH)

}
