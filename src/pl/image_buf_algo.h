/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <vector>

#include "pl/image_buf.h"

namespace ccl {

namespace ImageBufAlgo {

bool pow(ImageBuf &dst, const ImageBuf &A, const std::vector<float> &B);

}  // namespace ImageBufAlgo

}
