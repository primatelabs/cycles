/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/math.h"
#include "util/types.h"

namespace ccl {

ccl_device uchar float_to_byte(const float val)
{
  return ((val <= 0.0f) ?
              0 :
              ((val > (1.0f - 0.5f / 255.0f)) ? 255 : (uchar)((255.0f * val) + 0.5f)));  // NOLINT
}

ccl_device float byte_to_float(const uchar val)
{
  return val * (1.0f / 255.0f);
}

ccl_device uchar4 color_float_to_byte(const float3 c)
{
  uchar r;
  uchar g;
  uchar b;

  r = float_to_byte(c.x);
  g = float_to_byte(c.y);
  b = float_to_byte(c.z);

  return make_uchar4(r, g, b, 0);
}

ccl_device uchar4 color_float4_to_uchar4(const float4 c)
{
  uchar r;
  uchar g;
  uchar b;
  uchar a;

  r = float_to_byte(c.x);
  g = float_to_byte(c.y);
  b = float_to_byte(c.z);
  a = float_to_byte(c.w);

  return make_uchar4(r, g, b, a);
}

ccl_device_inline float3 color_byte_to_float(const uchar4 c)
{
  return make_float3(c.x * (1.0f / 255.0f), c.y * (1.0f / 255.0f), c.z * (1.0f / 255.0f));
}

ccl_device_inline float4 color_uchar4_to_float4(const uchar4 c)
{
  return make_float4(
      c.x * (1.0f / 255.0f), c.y * (1.0f / 255.0f), c.z * (1.0f / 255.0f), c.w * (1.0f / 255.0f));
}

ccl_device float color_srgb_to_linear(const float c)
{
  if (c < 0.04045f) {
    return (c < 0.0f) ? 0.0f : c * (1.0f / 12.92f);
  }
  return powf((c + 0.055f) * (1.0f / 1.055f), 2.4f);
}

ccl_device float color_linear_to_srgb(const float c)
{
  if (c < 0.0031308f) {
    return (c < 0.0f) ? 0.0f : c * 12.92f;
  }
  return 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
}

ccl_device float3 rgb_to_hsv(const float3 rgb)
{
  float cmax;
  float cmin;
  float h;
  float s;
  float v;
  float cdelta;
  float3 c;

  cmax = fmaxf(rgb.x, fmaxf(rgb.y, rgb.z));
  cmin = min(rgb.x, min(rgb.y, rgb.z));
  cdelta = cmax - cmin;

  v = cmax;

  if (cmax != 0.0f) {
    s = cdelta / cmax;
  }
  else {
    s = 0.0f;
    h = 0.0f;
  }

  if (s != 0.0f) {
    const float3 cmax3 = make_float3(cmax, cmax, cmax);
    c = (cmax3 - rgb) / cdelta;

    if (rgb.x == cmax) {
      h = c.z - c.y;
    }
    else if (rgb.y == cmax) {
      h = 2.0f + c.x - c.z;
    }
    else {
      h = 4.0f + c.y - c.x;
    }

    h /= 6.0f;

    if (h < 0.0f) {
      h += 1.0f;
    }
  }
  else {
    h = 0.0f;
  }

  return make_float3(h, s, v);
}

ccl_device float3 hsv_to_rgb(const float3 hsv)
{
  float i;
  float f;
  float p;
  float q;
  float t;
  float h;
  float s;
  float v;
  float3 rgb;

  h = hsv.x;
  s = hsv.y;
  v = hsv.z;

  if (s != 0.0f) {
    if (h == 1.0f) {
      h = 0.0f;
    }

    h *= 6.0f;
    i = floorf(h);
    f = h - i;
    rgb = make_float3(f, f, f);
    p = v * (1.0f - s);
    q = v * (1.0f - (s * f));
    t = v * (1.0f - (s * (1.0f - f)));

    if (i == 0.0f) {
      rgb = make_float3(v, t, p);
    }
    else if (i == 1.0f) {
      rgb = make_float3(q, v, p);
    }
    else if (i == 2.0f) {
      rgb = make_float3(p, v, t);
    }
    else if (i == 3.0f) {
      rgb = make_float3(p, q, v);
    }
    else if (i == 4.0f) {
      rgb = make_float3(t, p, v);
    }
    else {
      rgb = make_float3(v, p, q);
    }
  }
  else {
    rgb = make_float3(v, v, v);
  }

  return rgb;
}

ccl_device float3 rgb_to_hsl(const float3 rgb)
{
  float cmax;
  float cmin;
  float h;
  float s;
  float l;

  cmax = fmaxf(rgb.x, fmaxf(rgb.y, rgb.z));
  cmin = min(rgb.x, min(rgb.y, rgb.z));
  l = min(1.0f, (cmax + cmin) / 2.0f);

  if (cmax == cmin) {
    h = s = 0.0f; /* achromatic */
  }
  else {
    const float cdelta = cmax - cmin;
    s = l > 0.5f ? cdelta / (2.0f - cmax - cmin) : cdelta / (cmax + cmin);
    if (cmax == rgb.x) {
      h = (rgb.y - rgb.z) / cdelta + (rgb.y < rgb.z ? 6.0f : 0.0f);
    }
    else if (cmax == rgb.y) {
      h = (rgb.z - rgb.x) / cdelta + 2.0f;
    }
    else {
      h = (rgb.x - rgb.y) / cdelta + 4.0f;
    }
  }
  h /= 6.0f;

  return make_float3(h, s, l);
}

ccl_device float3 hsl_to_rgb(const float3 hsl)
{
  float nr;
  float ng;
  float nb;
  float chroma;
  float h;
  float s;
  float l;

  h = hsl.x;
  s = hsl.y;
  l = hsl.z;

  nr = fabsf(h * 6.0f - 3.0f) - 1.0f;
  ng = 2.0f - fabsf(h * 6.0f - 2.0f);
  nb = 2.0f - fabsf(h * 6.0f - 4.0f);

  nr = clamp(nr, 0.0f, 1.0f);
  nb = clamp(nb, 0.0f, 1.0f);
  ng = clamp(ng, 0.0f, 1.0f);

  chroma = (1.0f - fabsf(2.0f * l - 1.0f)) * s;

  return make_float3((nr - 0.5f) * chroma + l, (ng - 0.5f) * chroma + l, (nb - 0.5f) * chroma + l);
}

ccl_device float3 xyY_to_xyz(const float x, const float y, float Y)
{
  float X;
  float Z;

  if (y != 0.0f) {
    X = (x / y) * Y;
  }
  else {
    X = 0.0f;
  }

  if (y != 0.0f && Y != 0.0f) {
    Z = (1.0f - x - y) / y * Y;
  }
  else {
    Z = 0.0f;
  }

  return make_float3(X, Y, Z);
}

#ifdef __KERNEL_SSE2__
/*
 * Calculate initial guess for arg^exp based on float representation
 * This method gives a constant bias,
 * which can be easily compensated by multiplication with bias_coeff.
 * Gives better results for exponents near 1 (e. g. 4/5).
 * exp = exponent, encoded as uint32_t
 * e2coeff = 2^(127/exponent - 127) * bias_coeff^(1/exponent), encoded as uint32_t
 */
template<unsigned exp, unsigned e2coeff> ccl_device_inline float4 fastpow_sse2(const float4 &arg)
{
  float4 ret = arg * cast(make_int4(e2coeff));
  ret = make_float4(cast(ret));
  ret = ret * cast(make_int4(exp));
  ret = cast(make_int4(ret));
  return ret;
}

/* Improve x ^ 1.0f/5.0f solution with Newton-Raphson method */
ccl_device_inline float4 improve_5throot_solution_sse2(const float4 &old_result, const float4 &x)
{
  const float4 approx2 = old_result * old_result;
  const float4 approx4 = approx2 * approx2;
  const float4 t = x / approx4;
  const float4 summ = madd(make_float4(4.0f), old_result, t);
  return summ * make_float4(1.0f / 5.0f);
}

/* Calculate powf(x, 2.4). Working domain: 1e-10 < x < 1e+10 */
ccl_device_inline float4 fastpow24_sse2(const float4 &arg)
{
  /* `max`, `avg` and |avg| errors were calculated in GCC without FMA instructions.
   * The final precision should be better than `powf` in GLIBC. */

  /* Calculate x^4/5, coefficient 0.994 was constructed manually to minimize avg error */
  /* 0x3F4CCCCD = 4/5 */
  /* 0x4F55A7FB = 2^(127/(4/5) - 127) * 0.994^(1/(4/5)) */
  float4 x = fastpow_sse2<0x3F4CCCCD, 0x4F55A7FB>(
      arg);  // error max = 0.17  avg = 0.0018    |avg| = 0.05
  const float4 arg2 = arg * arg;
  const float4 arg4 = arg2 * arg2;

  /* error max = 0.018     avg = 0.0031    |avg| = 0.0031 */
  x = improve_5throot_solution_sse2(x, arg4);
  /* error max = 0.00021   avg = 1.6e-05   |avg| = 1.6e-05 */
  x = improve_5throot_solution_sse2(x, arg4);
  /* error max = 6.1e-07   avg = 5.2e-08   |avg| = 1.1e-07 */
  x = improve_5throot_solution_sse2(x, arg4);

  return x * (x * x);
}

ccl_device float4 color_srgb_to_linear_sse2(const float4 &c)
{
  const int4 cmp = c < make_float4(0.04045f);
  const float4 lt = max(c * make_float4(1.0f / 12.92f), make_float4(0.0f));
  const float4 gtebase = (c + make_float4(0.055f)) * make_float4(1.0f / 1.055f); /* fma */
  const float4 gte = fastpow24_sse2(gtebase);
  return select(cmp, lt, gte);
}
#endif /* __KERNEL_SSE2__ */

ccl_device float3 color_srgb_to_linear_v3(const float3 c)
{
  return make_float3(
      color_srgb_to_linear(c.x), color_srgb_to_linear(c.y), color_srgb_to_linear(c.z));
}

ccl_device float3 color_linear_to_srgb_v3(const float3 c)
{
  return make_float3(
      color_linear_to_srgb(c.x), color_linear_to_srgb(c.y), color_linear_to_srgb(c.z));
}

ccl_device float4 color_linear_to_srgb_v4(const float4 c)
{
  return make_float4(
      color_linear_to_srgb(c.x), color_linear_to_srgb(c.y), color_linear_to_srgb(c.z), c.w);
}

ccl_device float4 color_srgb_to_linear_v4(const float4 c)
{
#ifdef __KERNEL_SSE2__
  float4 r = c;
  r = color_srgb_to_linear_sse2(r);
  r.w = c.w;
  return r;
#else
  return make_float4(
      color_srgb_to_linear(c.x), color_srgb_to_linear(c.y), color_srgb_to_linear(c.z), c.w);
#endif
}

ccl_device float3 color_highlight_compress(float3 color, ccl_private float3 *variance)
{
  color += one_float3();
  if (variance) {
    *variance *= sqr(one_float3() / color);
  }
  return log(color);
}

ccl_device float3 color_highlight_uncompress(const float3 color)
{
  return exp(color) - one_float3();
}

/* Color division */

ccl_device_inline Spectrum safe_invert_color(Spectrum a)
{
  FOREACH_SPECTRUM_CHANNEL (i) {
    GET_SPECTRUM_CHANNEL(a, i) = (GET_SPECTRUM_CHANNEL(a, i) != 0.0f) ?
                                     1.0f / GET_SPECTRUM_CHANNEL(a, i) :
                                     0.0f;
  }

  return a;
}

/* Returns `a/b`, and replace the channel value with `fallback` if `b == 0`. */
ccl_device_inline Spectrum safe_divide_color(Spectrum a, Spectrum b, const float fallback = 0.0f)
{
  FOREACH_SPECTRUM_CHANNEL (i) {
    GET_SPECTRUM_CHANNEL(a, i) = (GET_SPECTRUM_CHANNEL(b, i) != 0.0f) ?
                                     GET_SPECTRUM_CHANNEL(a, i) / GET_SPECTRUM_CHANNEL(b, i) :
                                     fallback;
  }

  return a;
}

ccl_device_inline float3 safe_divide_even_color(const float3 a, const float3 b)
{
  float x;
  float y;
  float z;

  x = (b.x != 0.0f) ? a.x / b.x : 0.0f;
  y = (b.y != 0.0f) ? a.y / b.y : 0.0f;
  z = (b.z != 0.0f) ? a.z / b.z : 0.0f;

  /* try to get gray even if b is zero */
  if (b.x == 0.0f) {
    if (b.y == 0.0f) {
      x = z;
      y = z;
    }
    else if (b.z == 0.0f) {
      x = y;
      z = y;
    }
    else {
      x = 0.5f * (y + z);
    }
  }
  else if (b.y == 0.0f) {
    if (b.z == 0.0f) {
      y = x;
      z = x;
    }
    else {
      y = 0.5f * (x + z);
    }
  }
  else if (b.z == 0.0f) {
    z = 0.5f * (x + y);
  }

  return make_float3(x, y, z);
}

}
