/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

/* Parameter value lists from OpenImageIO are used to store custom properties
 * on various data, which can then later be used in shaders. */

#include <string>

#include "pl/param_value.h"
#include "pl/type_desc.h"

namespace ccl {

static constexpr TypeDesc TypeRGBA(TypeDesc::FLOAT, TypeDesc::VEC4, TypeDesc::COLOR);
static constexpr TypeDesc TypeFloatArray4(TypeDesc::FLOAT,
                                          TypeDesc::SCALAR,
                                          TypeDesc::NOSEMANTICS,
                                          4);

using std::string;

}
