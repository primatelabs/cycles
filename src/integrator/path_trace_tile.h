/* SPDX-FileCopyrightText: 2021-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "session/output_driver.h"

namespace ccl {

/* PathTraceTile
 *
 * Implementation of OutputDriver::Tile interface for path tracer. */

class PathTrace;

class PathTraceTile : public OutputDriver::Tile {
 public:
  PathTraceTile(PathTrace &path_trace);

  bool get_pass_pixels(const string_view pass_name,
                       const int num_channels,
                       float *pixels) const override;
  bool set_pass_pixels(const string_view pass_name,
                       const int num_channels,
                       const float *pixels) const override;

 private:
  PathTrace &path_trace_;
  mutable bool copied_from_device_;
};

}
