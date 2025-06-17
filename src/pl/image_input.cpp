/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "pl/image_input.h"

#include "pl/error.h"

CCL_NAMESPACE_BEGIN

ImageInput::unique_ptr ImageInput::open(const std::string &filename, const ImageSpec *config)
{
  PL_NOT_IMPLEMENTED();
  return nullptr;
}

ImageInput::unique_ptr ImageInput::create(string_view filename)
{
  PL_NOT_IMPLEMENTED();
  return nullptr;
}

bool ImageInput::read_image(int subimage,
                            int miplevel,
                            int chbegin,
                            int chend,
                            TypeDesc format,
                            void *data,
                            stride_t xstride,
                            stride_t ystride,
                            stride_t zstride)
{
  PL_NOT_IMPLEMENTED();
  return false;
}

bool ImageInput::has_error() const
{
  PL_NOT_IMPLEMENTED();
  return false;
}

std::string ImageInput::geterror(bool clear) const
{
  PL_NOT_IMPLEMENTED();
  return std::string();
}

CCL_NAMESPACE_END
