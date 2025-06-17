/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "pl/png_image_output.h"

#include <png.h>

#include "pl/error.h"

CCL_NAMESPACE_BEGIN

namespace {

void png_error_fn(png_structp png_ptr, png_const_charp error_msg)
{
  std::cerr << error_msg << std::endl;
}

void png_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
  std::cerr << warning_msg << std::endl;
}

void png_write_data(png_structp png, png_bytep data, png_size_t length)
{
  PNGImageOutput *output = reinterpret_cast<PNGImageOutput *>(png_get_io_ptr(png));

  output->ofs_.write(reinterpret_cast<char *>(data), length);
}

void png_flush_data(png_structp png)
{
  PNGImageOutput *output = reinterpret_cast<PNGImageOutput *>(png_get_io_ptr(png));

  output->ofs_.flush();
}

void png_float_to_uint8(uint8_t *dst, const float *src, int elements)
{
  for (int i = 0; i < elements; i++) {
    dst[i] = static_cast<uint8_t>(src[i] * 255.0f);
  }
}

}  // namespace

PNGImageOutput::PNGImageOutput() {}

PNGImageOutput::~PNGImageOutput()
{
  close();
}

bool PNGImageOutput::open(const std::string &filename, const ImageSpec &spec)
{
  spec_ = spec;
  ofs_.open(filename, std::ios::out | std::ios::binary);

  return !ofs_.fail();
}

bool PNGImageOutput::close()
{
  ofs_.close();

  return true;
}

bool PNGImageOutput::write_image(
    TypeDesc format, const void *data, stride_t xstride, stride_t ystride, stride_t zstride)
{
  if (xstride == AutoStride) {
    xstride = spec_.pixel_bytes();
  }
  if (ystride == AutoStride) {
    ystride = xstride * spec_.width;
  }
  if (zstride == AutoStride) {
    zstride = ystride * spec_.height;
  }

  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info_ptr = png_create_info_struct(png_ptr);

  if (!png_ptr || !info_ptr) {
    // TODO: Throw an exception.
  }

  // Override libpng's default error and warning functions.
  png_set_error_fn(png_ptr, nullptr, png_error_fn, png_warning_fn);

  png_set_write_fn(png_ptr, this, png_write_data, png_flush_data);

  int bit_depth = 8;
  int color_type = PNG_COLOR_TYPE_RGB_ALPHA;

  PL_CHECK(spec_.nchannels == 4);

  png_set_IHDR(png_ptr,
               info_ptr,
               spec_.width,
               spec_.height,
               bit_depth,
               color_type,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);

  png_bytep *row_pointers = static_cast<png_bytep *>(malloc(spec_.height * sizeof(png_bytep)));
  for (uint32_t i = 0; i < spec_.height; i++) {
    if (format.basetype == TypeDesc::FLOAT) {
      size_t elements = spec_.width * spec_.nchannels;
      row_pointers[i] = static_cast<png_bytep>(malloc(elements));
      png_float_to_uint8(row_pointers[i],
                         reinterpret_cast<const float *>((const char *)data + (ystride * i)),
                         elements);
    }
    else {
      row_pointers[i] = reinterpret_cast<uint8_t *>((char *)data + (ystride * i));
    }
  }
  png_write_image(png_ptr, row_pointers);

  if (format.basetype == TypeDesc::FLOAT) {
    for (uint32_t i = 0; i < spec_.height; i++) {
      free(row_pointers[i]);
    }
  }

  free(row_pointers);

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
}

CCL_NAMESPACE_END
