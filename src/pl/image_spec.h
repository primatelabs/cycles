/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <vector>

#include "pl/image_io.h"
#include "pl/param_value.h"
#include "pl/type_desc.h"

namespace ccl {

class ImageSpec {
 public:
  int width;
  int height;
  int depth;

  int tile_width;
  int tile_height;
  int tile_depth;

  int nchannels;

  TypeDesc format;

  std::vector<TypeDesc> channelformats;

  std::vector<std::string> channelnames;

  int alpha_channel;

  int z_channel;

  bool deep;

  std::vector<ParamValue> extra_attribs;

  ImageSpec(TypeDesc format = TypeDesc::UNKNOWN);

  ImageSpec(int xres, int yres, int nchans, TypeDesc format);

  size_t channel_bytes() const;

  size_t pixel_bytes() const;

  imagesize_t scanline_bytes() const;

  void attribute(string_view name, TypeDesc type, const void *value);

  void attribute(string_view name, unsigned int value);

  void attribute(string_view name, string_view value);

  void attribute(string_view name, string value);

  void attribute(string_view name, TypeDesc type, string_view value);

  ParamValue *find_attribute(string_view name,
                             TypeDesc searchtype = TypeDesc::UNKNOWN,
                             bool casesensitive = false);

  const ParamValue *find_attribute(string_view name,
                                   TypeDesc searchtype = TypeDesc::UNKNOWN,
                                   bool casesensitive = false) const;

  int get_int_attribute(string_view name, int default_value = 0) const;

  float get_float_attribute(string_view name, float default_value = 0) const;

  string_view get_string_attribute(string_view name,
                                   string_view default_value = string_view()) const;
};

}
