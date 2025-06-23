/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "pl/image_spec.h"

#include "pl/error.h"

namespace ccl {

ImageSpec::ImageSpec(TypeDesc format)
    : width(0),
      height(0),
      depth(1),
      tile_width(0),
      tile_height(0),
      tile_depth(1),
      nchannels(0),
      format(format),
      alpha_channel(-1),
      z_channel(-1),
      deep(false)
{
}

ImageSpec::ImageSpec(int xres, int yres, int nchans, TypeDesc format)
    : width(xres),
      height(yres),
      depth(1),
      tile_width(0),
      tile_height(0),
      tile_depth(1),
      nchannels(nchans),
      format(format),
      alpha_channel(-1),
      z_channel(-1),
      deep(false)
{
}

size_t ImageSpec::channel_bytes() const
{
  PL_CHECK(channelformats.empty());

  return format.size();
}

size_t ImageSpec::pixel_bytes() const
{
  PL_CHECK(channelformats.empty());

  if (nchannels < 0) {
    return 0;
  }

  return nchannels * channel_bytes();
}

imagesize_t ImageSpec::scanline_bytes() const
{
  PL_CHECK(channelformats.empty());

  if (width < 0) {
    return 0;
  }

  return width * pixel_bytes();
}

void ImageSpec::attribute(string_view name, unsigned int value)
{
  PL_NOT_IMPLEMENTED();
}

void ImageSpec::attribute(string_view name, string value)
{
  PL_NOT_IMPLEMENTED();
}

int ImageSpec::get_int_attribute(string_view name, int default_value) const
{
  PL_NOT_IMPLEMENTED();
  return default_value;
}

float ImageSpec::get_float_attribute(string_view name, float default_value) const
{
  PL_NOT_IMPLEMENTED();
  return default_value;
}

string_view ImageSpec::get_string_attribute(string_view name, string_view default_value) const
{
  PL_NOT_IMPLEMENTED();
  return default_value;
}

}
