/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

/* clang-format off */

#pragma once

/* #define static_assert triggers a bug in some clang-format versions, disable
 * format for entire file to keep results consistent. */

namespace ccl {

#if defined(CYCLES_CUBIN_CC)
#  define static_assert(statement, message)
#endif

#define static_assert_align(st, align) \
  static_assert((sizeof(st) % (align) == 0), "Structure must be strictly aligned")  // NOLINT

}
