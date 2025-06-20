/* SPDX-FileCopyrightText: 2011-2023 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <OSL/oslconfig.h>

CCL_NAMESPACE_BEGIN

using OSLUStringHash = OSL::stringhash;
#if OSL_LIBRARY_VERSION_CODE >= 11400
using OSLUStringRep = OSL::stringhash;
#else
using OSLUStringRep = OSL::stringrep;
#endif

static inline OSL::string to_string(OSLUStringHash h)
{
  return OSL::string::from_hash(h.hash());
}

CCL_NAMESPACE_END
