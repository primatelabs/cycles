/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "scene/image.h"

namespace ccl {

class OIIOImageLoader : public ImageLoader {
 public:
  OIIOImageLoader(const string &filepath);
  ~OIIOImageLoader() override;

  bool load_metadata(const ImageDeviceFeatures &features, ImageMetaData &metadata) override;

  bool load_pixels(const ImageMetaData &metadata,
                   void *pixels,
                   const size_t pixels_size,
                   const bool associate_alpha) override;

  string name() const override;

  string osl_filepath() const override;

  bool equals(const ImageLoader &other) const override;

 protected:
  string filepath;
};

}
