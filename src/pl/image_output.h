/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <memory>

#include "pl/image_spec.h"
#include "pl/type_desc.h"

namespace ccl {

class ImageOutput {
 protected:
  ImageSpec spec_;

 public:
  using unique_ptr = std::unique_ptr<ImageOutput>;

  ImageOutput();

  virtual ~ImageOutput();

  /// Return the name of the format implemented by this class.
  virtual const char *format_name(void) const = 0;

  virtual int supports(string_view feature) const
  {
    return false;
  }

  static unique_ptr create(string_view filename);

  virtual bool open(string_view filename, const ImageSpec &spec) = 0;

  virtual const ImageSpec &spec() const
  {
    return spec_;
  }

  virtual bool close() = 0;

  virtual bool write_image(TypeDesc format,
                           const void *data,
                           stride_t xstride = AutoStride,
                           stride_t ystride = AutoStride,
                           stride_t zstride = AutoStride) = 0;

  virtual bool write_tiles(int xbegin,
                           int xend,
                           int ybegin,
                           int yend,
                           int zbegin,
                           int zend,
                           TypeDesc format,
                           const void *data,
                           stride_t xstride = AutoStride,
                           stride_t ystride = AutoStride,
                           stride_t zstride = AutoStride);

  std::string geterror(bool clear = true) const;
};

}
