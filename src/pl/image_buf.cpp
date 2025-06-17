/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "pl/image_buf.h"

#include "pl/error.h"

CCL_NAMESPACE_BEGIN

ImageBuf::ImageBuf(
    const ImageSpec &spec, void *buffer, stride_t xstride, stride_t ystride, stride_t zstride)
    : buffer_spec_(spec),
      output_format_(TypeDesc::UNKNOWN),
      buffer_(buffer),
      xstride_(xstride),
      ystride_(ystride),
      zstride_(zstride)
{
}

void ImageBuf::set_write_format(TypeDesc format)
{
  output_format_ = format;
}

stride_t ImageBuf::pixel_stride() const
{
  return xstride_;
}

stride_t ImageBuf::scanline_stride() const
{
  return ystride_;
}

stride_t ImageBuf::z_stride() const
{
  return zstride_;
}

bool ImageBuf::write(ImageOutput *out) const
{
  bool ok = out->write_image(
      buffer_spec_.format, buffer_, pixel_stride(), scanline_stride(), z_stride());

  return ok;
}

CCL_NAMESPACE_END
