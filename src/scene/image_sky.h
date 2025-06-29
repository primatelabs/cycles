/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "scene/image.h"

namespace ccl {

class SkyLoader : public ImageLoader {
 private:
  float sun_elevation;
  float altitude;
  float air_density;
  float dust_density;
  float ozone_density;

 public:
  SkyLoader(const float sun_elevation,
            const float altitude,
            const float air_density,
            const float dust_density,
            float ozone_density);
  ~SkyLoader() override;

  bool load_metadata(const ImageDeviceFeatures &features, ImageMetaData &metadata) override;

  bool load_pixels(const ImageMetaData &metadata,
                   void *pixels,
                   const size_t /*pixels_size*/,
                   const bool /*associate_alpha*/) override;

  string name() const override;

  bool equals(const ImageLoader & /*other*/) const override;
};

}
