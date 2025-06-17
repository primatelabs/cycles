/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

/* Use string view implementation from OIIO.
 * Ideally, need to switch to `std::string_view`, but this first requires getting rid of using
 * namespace OIIO as it causes symbol collision. */
#include <OpenImageIO/string_view.h>

CCL_NAMESPACE_BEGIN

using OIIO::string_view;

// Stride lengths between pixels, scanlines or planes.
using stride_t = int64_t;

// Pixel counts in scanlines, tiles, or images.
using imagesize_t = uint64_t;

// Constant that indicates stride lengths should be computed.
const stride_t AutoStride = std::numeric_limits<stride_t>::min();

CCL_NAMESPACE_END
