// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#include "pl/type_desc.h"

#include "pl/error.h"

CCL_NAMESPACE_BEGIN

namespace {

static int basetype_size[TypeDesc::LASTBASE] = {
    0,                           // UNKNOWN
    0,                           // VOID
    sizeof(unsigned char),       // UCHAR
    sizeof(char),                // CHAR
    sizeof(unsigned short),      // USHORT
    sizeof(short),               // SHORT
    sizeof(unsigned int),        // UINT
    sizeof(int),                 // INT
    sizeof(unsigned long long),  // ULONGLONG
    sizeof(long long),           // LONGLONG
    sizeof(float) / 2,           // HALF
    sizeof(float),               // FLOAT
    sizeof(double),              // DOUBLE
    sizeof(char *),              // STRING
    sizeof(void *),              // PTR
};

static bool isfloat[TypeDesc::LASTBASE] = {
    0,  // UNKNOWN
    0,  // VOID
    0,  // UCHAR
    0,  // CHAR
    0,  // USHORT
    0,  // SHORT
    0,  // UINT
    0,  // INT
    0,  // ULONGLONG
    0,  // LONGLONG
    1,  // HALF
    1,  // FLOAT
    1,  // DOUBLE
    0,  // STRING
    0,  // PTR
};

}  // namespace

size_t TypeDesc::basesize() const noexcept
{
  if (basetype >= TypeDesc::LASTBASE) {
    return 0;
  }
  return basetype_size[basetype];
}

bool TypeDesc::is_floating_point() const noexcept
{
  if (basetype >= TypeDesc::LASTBASE) {
    return false;
  }
  return isfloat[basetype];
}

CCL_NAMESPACE_END
