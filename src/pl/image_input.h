/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <memory>

#include "pl/image_spec.h"
#include "pl/type_desc.h"

CCL_NAMESPACE_BEGIN

class ImageInput {
 public:
  using unique_ptr = std::unique_ptr<ImageInput>;

  ImageInput();

  virtual ~ImageInput();

  static unique_ptr open(string_view filename, const ImageSpec *config = nullptr);

  static unique_ptr create(string_view filename);

  virtual bool open(string_view name, ImageSpec &newspec) = 0;

  virtual bool open(string_view name, ImageSpec &newspec, const ImageSpec &config)
  {
    return open(name, newspec);
  }

  virtual const char *format_name(void) const = 0;

  virtual bool close() = 0;

  virtual bool read_image(int subimage,
                          int miplevel,
                          int chbegin,
                          int chend,
                          TypeDesc format,
                          void *data,
                          stride_t xstride = AutoStride,
                          stride_t ystride = AutoStride,
                          stride_t zstride = AutoStride);

  virtual const ImageSpec &spec(void) const;

  bool has_error() const;

  std::string geterror(bool clear = true) const;
};

CCL_NAMESPACE_END
