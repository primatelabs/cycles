/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <limits>
#include <string_view>

namespace ccl {

// Stride lengths between pixels, scanlines or planes.
using stride_t = int64_t;

// Pixel counts in scanlines, tiles, or images.
using imagesize_t = uint64_t;

// Constant that indicates stride lengths should be computed.
const stride_t AutoStride = std::numeric_limits<stride_t>::min();

}
