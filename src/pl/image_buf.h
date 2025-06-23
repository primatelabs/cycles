/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "pl/image_output.h"
#include "pl/image_spec.h"

namespace ccl {

class ImageBuf {
 protected:
  const ImageSpec &buffer_spec_;

  TypeDesc output_format_;

  void *buffer_;

  stride_t xstride_;

  stride_t ystride_;

  stride_t zstride_;

 public:
  ImageBuf() = delete;

  ImageBuf(const ImageSpec &spec,
           void *buffer,
           stride_t xstride = AutoStride,
           stride_t ystride = AutoStride,
           stride_t zstride = AutoStride);

  void set_write_format(TypeDesc format);

  stride_t pixel_stride() const;

  stride_t scanline_stride() const;

  stride_t z_stride() const;

  bool write(ImageOutput *out) const;
};

}
