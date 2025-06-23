/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <fstream>
#include <memory>

#include "pl/image_output.h"

namespace ccl {

class PNGImageOutput : public ImageOutput {
 public:
  std::ofstream ofs_;

 public:
  PNGImageOutput();

  virtual ~PNGImageOutput();

  virtual const char *format_name(void) const
  {
    return "png";
  }

  virtual int supports(string_view feature) const
  {
    return (feature == "alpha");
  }

  virtual bool open(string_view filename, const ImageSpec &newspec);

  virtual bool close();

  virtual bool write_image(TypeDesc format,
                           const void *data,
                           stride_t xstride = AutoStride,
                           stride_t ystride = AutoStride,
                           stride_t zstride = AutoStride);
};

}
