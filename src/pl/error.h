/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <iostream>

CCL_NAMESPACE_BEGIN

#define PL_CHECK(expr) \
  { \
    if (!(expr)) { \
      std::cerr << __FILE__ << ":" << __LINE__ << " " << #expr << std::endl; \
    } \
  }

#define PL_NOT_IMPLEMENTED() \
  { \
    std::cerr << __FILE__ << ":" << __LINE__ << " not implemented" << std::endl; \
  }

CCL_NAMESPACE_END
