/* SPDX-FileCopyrightText: 2004 NVIDIA Corporation
 * SPDX-FileCopyrightText: 2008-2014 Larry Gritz
 * SPDX-FileCopyrightText: 2009-2014 Sony Pictures Imageworks Inc., et al.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Adapted from OpenImageIO
 * Copyright 2008-2014 Larry Gritz and the other authors and contributors.
 * All Rights Reserved.
 *
 * A few bits here are based upon code from NVIDIA that was also released
 * under the same modified BSD license, and marked as:
 *    `Copyright 2004 NVIDIA Corporation. All Rights Reserved.`
 *
 * Some parts of this file were first open-sourced in Open Shading Language,
 * then later moved here. The original copyright notice was:
 *    `Copyright (c) 2009-2014 Sony Pictures Imageworks Inc., et al.`
 *
 * Many of the math functions were copied from or inspired by other
 * public domain sources or open source packages with compatible licenses.
 * The individual functions give references were applicable.
 */

#pragma once

#include "util/math_base.h"
#include "util/math_float3.h"
#include "util/math_float4.h"
#include "util/math_int4.h"
#include "util/types_float3.h"
#include "util/types_float4.h"

namespace ccl {

ccl_device_inline float madd(const float a, const float b, const float c)
{
  /* NOTE: In the future we may want to explicitly ask for a fused
   * multiply-add in a specialized version for float.
   *
   * NOTE: GCC/ICC will turn this (for float) into a FMA unless
   * explicitly asked not to, clang seems to leave the code alone.
   */
  return a * b + c;
}

ccl_device_inline float4 madd4(const float4 a, const float4 b, const float4 c)
{
  return a * b + c;
}

/*
 * FAST & APPROXIMATE MATH
 *
 * The functions named "fast_*" provide a set of replacements to `libm` that
 * are much faster at the expense of some accuracy and robust handling of
 * extreme values. One design goal for these approximation was to avoid
 * branches as much as possible and operate on single precision values only
 * so that SIMD versions should be straightforward ports We also try to
 * implement "safe" semantics (ie: clamp to valid range where possible)
 * natively since wrapping these inline calls in another layer would be
 * wasteful.
 *
 * Some functions are fast_safe_*, which is both a faster approximation as
 * well as clamped input domain to ensure no NaN, Inf, or divide by zero.
 */

/* Round to nearest integer, returning as an int. */
ccl_device_inline int fast_rint(const float x)
{
  /* used by sin/cos/tan range reduction. */
#ifdef __KERNEL_SSE42__
  /* Single `roundps` instruction on SSE4.1+ for gcc/clang but not MSVC 19.35:
   * float_to_int(rintf(x)); so we use the equivalent intrinsics. */
  __m128 vec = _mm_set_ss(x);
  vec = _mm_round_ss(vec, vec, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
  return _mm_cvtss_si32(vec);
#else
  /* emulate rounding by adding/subtracting 0.5. */
  return float_to_int(x + copysignf(0.5f, x));
#endif
}

ccl_device float fast_sinf(float x)
{
  /* Very accurate argument reduction from SLEEF,
   * starts failing around x=262000
   *
   * Results on: [-2pi,2pi].
   *
   * Examined 2173837240 values of sin: 0.00662760244 avg ULP diff, 2 max ULP,
   * 1.19209e-07 max error
   */
  const int q = fast_rint(x * M_1_PI_F);
  const float qf = (float)q;
  x = madd(qf, -0.78515625f * 4, x);
  x = madd(qf, -0.00024187564849853515625f * 4, x);
  x = madd(qf, -3.7747668102383613586e-08f * 4, x);
  x = madd(qf, -1.2816720341285448015e-12f * 4, x);
  x = M_PI_2_F - (M_PI_2_F - x); /* Crush denormals */
  const float s = x * x;
  if ((q & 1) != 0) {
    x = -x;
  }
  /* This polynomial approximation has very low error on [-pi/2,+pi/2]
   * 1.19209e-07 max error in total over [-2pi,+2pi]. */
  float u = 2.6083159809786593541503e-06f;
  u = madd(u, s, -0.0001981069071916863322258f);
  u = madd(u, s, +0.00833307858556509017944336f);
  u = madd(u, s, -0.166666597127914428710938f);
  u = madd(s, u * x, x);
  /* For large x, the argument reduction can fail and the polynomial can be
   * evaluated with arguments outside the valid internal. Just clamp the bad
   * values away. */
  u = clamp(u, -1.0f, 1.0f);
  return u;
}

ccl_device float fast_cosf(float x)
{
  /* Same argument reduction as fast_sinf(). */
  const int q = fast_rint(x * M_1_PI_F);
  const float qf = (float)q;
  x = madd(qf, -0.78515625f * 4, x);
  x = madd(qf, -0.00024187564849853515625f * 4, x);
  x = madd(qf, -3.7747668102383613586e-08f * 4, x);
  x = madd(qf, -1.2816720341285448015e-12f * 4, x);
  x = M_PI_2_F - (M_PI_2_F - x); /* Crush denormals. */
  const float s = x * x;
  /* Polynomial from SLEEF's sincosf, max error is
   * 4.33127e-07 over [-2pi,2pi] (98% of values are "exact"). */
  float u = -2.71811842367242206819355e-07f;
  u = madd(u, s, +2.47990446951007470488548e-05f);
  u = madd(u, s, -0.00138888787478208541870117f);
  u = madd(u, s, +0.0416666641831398010253906f);
  u = madd(u, s, -0.5f);
  u = madd(u, s, +1.0f);
  if ((q & 1) != 0) {
    u = -u;
  }
  u = clamp(u, -1.0f, 1.0f);
  return u;
}

ccl_device void fast_sincosf(float x, ccl_private float *sine, ccl_private float *cosine)
{
  /* Same argument reduction as fast_sin. */
  const int q = fast_rint(x * M_1_PI_F);
  const float qf = (float)q;
  x = madd(qf, -0.78515625f * 4, x);
  x = madd(qf, -0.00024187564849853515625f * 4, x);
  x = madd(qf, -3.7747668102383613586e-08f * 4, x);
  x = madd(qf, -1.2816720341285448015e-12f * 4, x);
  x = M_PI_2_F - (M_PI_2_F - x);  // crush denormals
  const float s = x * x;
  /* NOTE: same exact polynomials as fast_sinf() and fast_cosf() above. */
  if ((q & 1) != 0) {
    x = -x;
  }
  float su = 2.6083159809786593541503e-06f;
  su = madd(su, s, -0.0001981069071916863322258f);
  su = madd(su, s, +0.00833307858556509017944336f);
  su = madd(su, s, -0.166666597127914428710938f);
  su = madd(s, su * x, x);
  float cu = -2.71811842367242206819355e-07f;
  cu = madd(cu, s, +2.47990446951007470488548e-05f);
  cu = madd(cu, s, -0.00138888787478208541870117f);
  cu = madd(cu, s, +0.0416666641831398010253906f);
  cu = madd(cu, s, -0.5f);
  cu = madd(cu, s, +1.0f);
  if ((q & 1) != 0) {
    cu = -cu;
  }
  su = clamp(su, -1.0f, 1.0f);
  cu = clamp(cu, -1.0f, 1.0f);
  *sine = su;
  *cosine = cu;
}

/* NOTE: this approximation is only valid on [-8192.0,+8192.0], it starts
 * becoming really poor outside of this range because the reciprocal amplifies
 * errors.
 */
ccl_device float fast_tanf(float x)
{
  /* Derived from SLEEF implementation.
   *
   * Note that we cannot apply the "denormal crush" trick everywhere because
   * we sometimes need to take the reciprocal of the polynomial
   */
  const int q = fast_rint(x * 2.0f * M_1_PI_F);
  const float qf = (float)q;
  x = madd(qf, -0.78515625f * 2, x);
  x = madd(qf, -0.00024187564849853515625f * 2, x);
  x = madd(qf, -3.7747668102383613586e-08f * 2, x);
  x = madd(qf, -1.2816720341285448015e-12f * 2, x);
  if ((q & 1) == 0) {
    /* Crush denormals (only if we aren't inverting the result later). */
    x = M_PI_4_F - (M_PI_4_F - x);
  }
  const float s = x * x;
  float u = 0.00927245803177356719970703f;
  u = madd(u, s, 0.00331984995864331722259521f);
  u = madd(u, s, 0.0242998078465461730957031f);
  u = madd(u, s, 0.0534495301544666290283203f);
  u = madd(u, s, 0.133383005857467651367188f);
  u = madd(u, s, 0.333331853151321411132812f);
  u = madd(s, u * x, x);
  if ((q & 1) != 0) {
    u = -1.0f / u;
  }
  return u;
}

/* Fast, approximate sin(x*M_PI) with maximum absolute error of 0.000918954611.
 *
 * Adapted from http://devmaster.net/posts/9648/fast-and-accurate-sine-cosine#comment-76773
 */
ccl_device float fast_sinpif(const float x)
{
  /* Fast trick to strip the integral part off, so our domain is [-1, 1]. */
  const float z = x - ((x + 25165824.0f) - 25165824.0f);
  const float y = z - z * fabsf(z);
  const float Q = 3.10396624f;
  const float P = 3.584135056f; /* P = 16-4*Q */
  return y * (Q + P * fabsf(y));

  /* The original article used inferior constants for Q and P and
   * so had max error 1.091e-3.
   *
   * The optimal value for Q was determined by exhaustive search, minimizing
   * the absolute numerical error relative to float(std::sin(double(phi*M_PI)))
   * over the interval [0,2] (which is where most of the invocations happen).
   *
   * The basic idea of this approximation starts with the coarse approximation:
   *      sin(pi*x) ~= f(x) =  4 * (x - x * abs(x))
   *
   * This approximation always _over_ estimates the target. On the other hand,
   * the curve:
   *      sin(pi*x) ~= f(x) * abs(f(x)) / 4
   *
   * always lies _under_ the target. Thus we can simply numerically search for
   * the optimal constant to LERP these curves into a more precise
   * approximation.
   *
   * After folding the constants together and simplifying the resulting math,
   * we end up with the compact implementation above.
   *
   * NOTE: this function actually computes sin(x * pi) which avoids one or two
   * mults in many cases and guarantees exact values at integer periods.
   */
}

/* Fast approximate cos(x*M_PI) with ~0.1% absolute error. */
ccl_device_inline float fast_cospif(const float x)
{
  return fast_sinpif(x + 0.5f);
}

ccl_device float fast_acosf(const float x)
{
  const float f = fabsf(x);
  /* clamp and crush denormals. */
  const float m = (f < 1.0f) ? 1.0f - (1.0f - f) : 1.0f;
  /* Based on http://www.pouet.net/topic.php?which=9132&page=2
   * 85% accurate (ULP 0)
   * Examined 2130706434 values of acos:
   *   15.2000597 avg ULP diff, 4492 max ULP, 4.51803e-05 max error // without "denormal crush"
   * Examined 2130706434 values of acos:
   *   15.2007108 avg ULP diff, 4492 max ULP, 4.51803e-05 max error // with "denormal crush"
   */
  const float a = sqrtf(1.0f - m) *
                  (1.5707963267f + m * (-0.213300989f + m * (0.077980478f + m * -0.02164095f)));
  return x < 0 ? M_PI_F - a : a;
}

ccl_device float fast_asinf(const float x)
{
  /* Based on acosf approximation above.
   * Max error is 4.51133e-05 (ULPS are higher because we are consistently off
   * by a little amount). */
  const float f = fabsf(x);
  /* Clamp and crush denormals. */
  const float m = (f < 1.0f) ? 1.0f - (1.0f - f) : 1.0f;
  const float a = M_PI_2_F -
                  sqrtf(1.0f - m) * (1.5707963267f +
                                     m * (-0.213300989f + m * (0.077980478f + m * -0.02164095f)));
  return copysignf(a, x);
}

ccl_device float fast_atanf(const float x)
{
  const float a = fabsf(x);
  const float k = a > 1.0f ? 1 / a : a;
  const float s = 1.0f - (1.0f - k); /* Crush denormals. */
  const float t = s * s;
  /* http://mathforum.org/library/drmath/view/62672.html
   * Examined 4278190080 values of atan:
   *   2.36864877 avg ULP diff, 302 max ULP, 6.55651e-06 max error      // (with  denormals)
   * Examined 4278190080 values of atan:
   *   171160502 avg ULP diff, 855638016 max ULP, 6.55651e-06 max error // (crush denormals)
   */
  float r = s * madd(0.43157974f, t, 1.0f) / madd(madd(0.05831938f, t, 0.76443945f), t, 1.0f);
  if (a > 1.0f) {
    r = M_PI_2_F - r;
  }
  return copysignf(r, x);
}

ccl_device float fast_atan2f(const float y, const float x)
{
  /* Based on atan approximation above.
   *
   * The special cases around 0 and infinity were tested explicitly.
   *
   * The only case not handled correctly is x=NaN,y=0 which returns 0 instead
   * of nan.
   */
  const float a = fabsf(x);
  const float b = fabsf(y);

  const float k = (b == 0) ? 0.0f : ((a == b) ? 1.0f : (b > a ? a / b : b / a));
  const float s = 1.0f - (1.0f - k); /* Crush denormals */
  const float t = s * s;

  float r = s * madd(0.43157974f, t, 1.0f) / madd(madd(0.05831938f, t, 0.76443945f), t, 1.0f);

  if (b > a) {
    /* Account for arg reduction. */
    r = M_PI_2_F - r;
  }
  /* Test sign bit of x. */
  if (__float_as_uint(x) & 0x80000000u) {
    r = M_PI_F - r;
  }
  return copysignf(r, y);
}

/* Same as precise_angle, but using fast_atan2f. Still much better that acos(dot(a, b)). */
ccl_device_inline float vector_angle(const float3 a, const float3 b)
{
  return 2.0f * fast_atan2f(len(a - b), len(a + b));
}

/* Based on:
 *
 *   https://github.com/LiraNuna/glsl-sse2/blob/master/source/vec4.h
 */
ccl_device float fast_log2f(float x)
{
  /* NOTE: clamp to avoid special cases and make result "safe" from large
   * negative values/NAN's. */
  x = clamp(x, FLT_MIN, FLT_MAX);
  const unsigned bits = __float_as_uint(x);
  const int exponent = (int)(bits >> 23) - 127;
  const float f = __uint_as_float((bits & 0x007FFFFF) | 0x3f800000) - 1.0f;
  /* Examined 2130706432 values of log2 on [1.17549435e-38,3.40282347e+38]:
   * 0.0797524457 avg ULP diff, 3713596 max ULP, 7.62939e-06 max error.
   * ULP histogram:
   *  0  = 97.46%
   *  1  =  2.29%
   *  2  =  0.11%
   */
  const float f2 = f * f;
  const float f4 = f2 * f2;
  float hi = madd(f, -0.00931049621349f, 0.05206469089414f);
  float lo = madd(f, 0.47868480909345f, -0.72116591947498f);
  hi = madd(f, hi, -0.13753123777116f);
  hi = madd(f, hi, 0.24187369696082f);
  hi = madd(f, hi, -0.34730547155299f);
  lo = madd(f, lo, 1.442689881667200f);
  return ((f4 * hi) + (f * lo)) + exponent;
}

ccl_device_inline float fast_logf(const float x)
{
  /* Examined 2130706432 values of logf on [1.17549435e-38,3.40282347e+38]:
   * 0.313865375 avg ULP diff, 5148137 max ULP, 7.62939e-06 max error.
   */
  return fast_log2f(x) * M_LN2_F;
}

ccl_device_inline float fast_log10(const float x)
{
  /* Examined 2130706432 values of log10f on [1.17549435e-38,3.40282347e+38]:
   * 0.631237033 avg ULP diff, 4471615 max ULP, 3.8147e-06 max error.
   */
  return fast_log2f(x) * M_LN2_F / M_LN10_F;
}

ccl_device float fast_logb(float x)
{
  /* Don't bother with denormals. */
  x = fabsf(x);
  x = clamp(x, FLT_MIN, FLT_MAX);
  const unsigned bits = __float_as_uint(x);
  return (float)((int)(bits >> 23) - 127);
}

ccl_device float fast_exp2f(float x)
{
  /* Clamp to safe range for final addition. */
  x = clamp(x, -126.0f, 126.0f);
  /* Range reduction. */
  const int m = (int)x;
  x -= m;
  x = 1.0f - (1.0f - x); /* Crush denormals (does not affect max ULPS!). */
  /* 5th degree polynomial generated with sollya
   * Examined 2247622658 values of exp2 on [-126,126]: 2.75764912 avg ULP diff,
   * 232 max ULP.
   *
   * ULP histogram:
   *  0  = 87.81%
   *  1  =  4.18%
   */
  float r = 1.33336498402e-3f;
  r = madd(x, r, 9.810352697968e-3f);
  r = madd(x, r, 5.551834031939e-2f);
  r = madd(x, r, 0.2401793301105f);
  r = madd(x, r, 0.693144857883f);
  r = madd(x, r, 1.0f);
  /* Multiply by 2 ^ m by adding in the exponent. */
  /* NOTE: left-shift of negative number is undefined behavior. */
  return __uint_as_float(__float_as_uint(r) + ((unsigned)m << 23));
}

ccl_device_inline float fast_expf(const float x)
{
  /* Examined 2237485550 values of exp on [-87.3300018,87.3300018]:
   * 2.6666452 avg ULP diff, 230 max ULP.
   */
  return fast_exp2f(x / M_LN2_F);
}

#if !defined(__KERNEL_GPU__) && !defined(_MSC_VER)
/* MSVC seems to have a code-gen bug here in at least SSE41/AVX, see
 * #78047 and #78869 for details. Just disable for now, it only makes
 * a small difference in denoising performance. */
ccl_device float4 fast_exp2f4(float4 x)
{
  const float4 one = make_float4(1.0f);
  const float4 limit = make_float4(126.0f);
  x = clamp(x, -limit, limit);
  const int4 m = make_int4(x);
  x = one - (one - (x - make_float4(m)));
  float4 r = make_float4(1.33336498402e-3f);
  r = madd4(x, r, make_float4(9.810352697968e-3f));
  r = madd4(x, r, make_float4(5.551834031939e-2f));
  r = madd4(x, r, make_float4(0.2401793301105f));
  r = madd4(x, r, make_float4(0.693144857883f));
  r = madd4(x, r, make_float4(1.0f));
  return __int4_as_float4(__float4_as_int4(r) + (m << 23));
}

ccl_device_inline float4 fast_expf4(const float4 x)
{
  return fast_exp2f4(x / M_LN2_F);
}
#else
ccl_device_inline float4 fast_expf4(const float4 x)
{
  return make_float4(fast_expf(x.x), fast_expf(x.y), fast_expf(x.z), fast_expf(x.w));
}
#endif

ccl_device_inline float fast_exp10(const float x)
{
  /* Examined 2217701018 values of exp10 on [-37.9290009,37.9290009]:
   * 2.71732409 avg ULP diff, 232 max ULP.
   */
  return fast_exp2f(x * M_LN10_F / M_LN2_F);
}

ccl_device_inline float fast_expm1f(float x)
{
  if (fabsf(x) < 1e-5f) {
    x = 1.0f - (1.0f - x); /* Crush denormals. */
    return madd(0.5f, x * x, x);
  }
  return fast_expf(x) - 1.0f;
}

ccl_device float fast_sinhf(const float x)
{
  float a = fabsf(x);
  if (a > 1.0f) {
    /* Examined 53389559 values of sinh on [1,87.3300018]:
     * 33.6886442 avg ULP diff, 178 max ULP. */
    const float e = fast_expf(a);
    return copysignf(0.5f * e - 0.5f / e, x);
  }
  a = 1.0f - (1.0f - a); /* Crush denorms. */
  const float a2 = a * a;
  /* Degree 7 polynomial generated with sollya. */
  /* Examined 2130706434 values of sinh on [-1,1]: 1.19209e-07 max error. */
  float r = 2.03945513931e-4f;
  r = madd(r, a2, 8.32990277558e-3f);
  r = madd(r, a2, 0.1666673421859f);
  r = madd(r * a, a2, a);
  return copysignf(r, x);
}

ccl_device_inline float fast_coshf(const float x)
{
  /* Examined 2237485550 values of cosh on [-87.3300018,87.3300018]:
   * 1.78256726 avg ULP diff, 178 max ULP.
   */
  const float e = fast_expf(fabsf(x));
  return 0.5f * e + 0.5f / e;
}

ccl_device_inline float fast_tanhf(const float x)
{
  /* Examined 4278190080 values of tanh on [-3.40282347e+38,3.40282347e+38]:
   * 3.12924e-06 max error.
   */
  /* NOTE: ULP error is high because of sub-optimal handling around the origin. */
  const float e = fast_expf(2.0f * fabsf(x));
  return copysignf(1.0f - 2.0f / (1.0f + e), x);
}

ccl_device float fast_safe_powf(const float x, const float y)
{
  if (y == 0) {
    return 1.0f; /* x^1=1 */
  }
  if (x == 0) {
    return 0.0f; /* 0^y=0 */
  }
  float sign = 1.0f;
  if (x < 0.0f) {
    /* if x is negative, only deal with integer powers
     * powf returns NaN for non-integers, we will return 0 instead.
     */
    const int ybits = __float_as_int(y) & 0x7fffffff;
    if (ybits >= 0x4b800000) {
      // always even int, keep positive
    }
    else if (ybits >= 0x3f800000) {
      /* Bigger than 1, check. */
      const int k = (ybits >> 23) - 127; /* Get exponent. */
      const int j = ybits >> (23 - k);   /* Shift out possible fractional bits. */
      if ((j << (23 - k)) == ybits) {    /* rebuild number and check for a match. */
        /* +1 for even, -1 for odd. */
        sign = __int_as_float(0x3f800000 | (j << 31));
      }
      else {
        /* Not an integer. */
        return 0.0f;
      }
    }
    else {
      /* Not an integer. */
      return 0.0f;
    }
  }
  return sign * fast_exp2f(y * fast_log2f(fabsf(x)));
}

/* TODO(sergey): Check speed  with our erf functions implementation from
 * bsdf_microfacet.h.
 */

ccl_device_inline float fast_erff(const float x)
{
  /* Examined 1082130433 values of erff on [0,4]: 1.93715e-06 max error. */
  /* Abramowitz and Stegun, 7.1.28. */
  const float a1 = 0.0705230784f;
  const float a2 = 0.0422820123f;
  const float a3 = 0.0092705272f;
  const float a4 = 0.0001520143f;
  const float a5 = 0.0002765672f;
  const float a6 = 0.0000430638f;
  const float a = fabsf(x);
  if (a >= 12.3f) {
    return copysignf(1.0f, x);
  }
  const float b = 1.0f - (1.0f - a); /* Crush denormals. */
  const float r = madd(
      madd(madd(madd(madd(madd(a6, b, a5), b, a4), b, a3), b, a2), b, a1), b, 1.0f);
  const float s = r * r; /* ^2 */
  const float t = s * s; /* ^4 */
  const float u = t * t; /* ^8 */
  const float v = u * u; /* ^16 */
  return copysignf(1.0f - 1.0f / v, x);
}

ccl_device_inline float fast_erfcf(const float x)
{
  /* Examined 2164260866 values of erfcf on [-4,4]: 1.90735e-06 max error.
   *
   * ULP histogram:
   *
   *  0  = 80.30%
   */
  return 1.0f - fast_erff(x);
}

ccl_device_inline float fast_ierff(const float x)
{
  /* From: Approximating the `erfinv` function by Mike Giles. */
  /* To avoid trouble at the limit, clamp input to 1-epsilon. */
  const float a = min(fabsf(x), 0.99999994f);
  float w = -fast_logf((1.0f - a) * (1.0f + a));
  float p;
  if (w < 5.0f) {
    w = w - 2.5f;
    p = 2.81022636e-08f;
    p = madd(p, w, 3.43273939e-07f);
    p = madd(p, w, -3.5233877e-06f);
    p = madd(p, w, -4.39150654e-06f);
    p = madd(p, w, 0.00021858087f);
    p = madd(p, w, -0.00125372503f);
    p = madd(p, w, -0.00417768164f);
    p = madd(p, w, 0.246640727f);
    p = madd(p, w, 1.50140941f);
  }
  else {
    w = sqrtf(w) - 3.0f;
    p = -0.000200214257f;
    p = madd(p, w, 0.000100950558f);
    p = madd(p, w, 0.00134934322f);
    p = madd(p, w, -0.00367342844f);
    p = madd(p, w, 0.00573950773f);
    p = madd(p, w, -0.0076224613f);
    p = madd(p, w, 0.00943887047f);
    p = madd(p, w, 1.00167406f);
    p = madd(p, w, 2.83297682f);
  }
  return p * x;
}

/* Fast inverse cube root for positive x, with two Newton iterations to improve accuracy. */
ccl_device_inline float fast_inv_cbrtf(const float x)
{
  util_assert(x >= 0.0f);

  /* Constant is roughly `cbrt(2^127)`, but tweaked a bit to balance the error across the entire
   * range. The exact value is not critical. */
  float y = __int_as_float(0x54a24242 - __float_as_int(x) / 3);
  y = (2.0f / 3) * y + 1 / (3 * y * y * x);
  y = (2.0f / 3) * y + 1 / (3 * y * y * x);
  return y;
}

}
