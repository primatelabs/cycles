/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "pl/image_output.h"

#include <filesystem>
#include <memory>

#include "pl/error.h"
#include "pl/png_image_output.h"

CCL_NAMESPACE_BEGIN

ImageOutput::ImageOutput() {}

ImageOutput::~ImageOutput() {}

ImageOutput::unique_ptr ImageOutput::create(string_view filename)
{
  std::filesystem::path ext = std::filesystem::path(filename).extension();

  if (ext == ".png") {
    return std::make_unique<PNGImageOutput>();
  }

  return nullptr;
}

bool ImageOutput::write_tiles(int xbegin,
                              int xend,
                              int ybegin,
                              int yend,
                              int zbegin,
                              int zend,
                              TypeDesc format,
                              const void *data,
                              stride_t xstride,
                              stride_t ystride,
                              stride_t zstride)
{
  PL_NOT_IMPLEMENTED();
  return false;
}

std::string ImageOutput::geterror(bool clear) const
{
  PL_NOT_IMPLEMENTED();
  return std::string();
}

CCL_NAMESPACE_END
