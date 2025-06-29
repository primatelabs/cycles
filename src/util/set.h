/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <set>
#include <unordered_set>

#if defined(_MSC_VER) && (_MSC_VER >= 1900)
#  include <iterator>
#endif

namespace ccl {

using std::set;
using std::unordered_set;

}
