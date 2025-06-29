/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/math_base.h"
#include "util/vector.h"

namespace ccl {

/* Evaluate CDF of a given functor with given range and resolution. */
template<typename Functor>
void util_cdf_evaluate(
    const int resolution, const float from, const float to, Functor functor, vector<float> &cdf)
{
  const int cdf_count = resolution + 1;
  const float range = to - from;
  cdf.resize(cdf_count);
  cdf[0] = 0.0f;
  /* Actual CDF evaluation. */
  for (int i = 0; i < resolution; ++i) {
    float x = from + range * (float)i / (resolution - 1);
    float y = functor(x);
    cdf[i + 1] = cdf[i] + fabsf(y);
  }
  /* Normalize the CDF. */
  const float fac = (cdf[resolution] == 0.0f) ? 0.0f : 1.0f / cdf[resolution];
  for (int i = 0; i <= resolution; i++) {
    cdf[i] *= fac;
  }
  cdf[resolution] = 1.0f;
}

/* Invert pre-calculated CDF function. */
void util_cdf_invert(const int resolution,
                     const float from,
                     const float to,
                     const vector<float> &cdf,
                     const bool make_symmetric,
                     vector<float> &inv_cdf);

/* Evaluate inverted CDF of a given functor with given range and resolution. */
template<typename Functor>
void util_cdf_inverted(const int resolution,
                       const float from,
                       const float to,
                       Functor functor,
                       const bool make_symmetric,
                       vector<float> &inv_cdf)
{
  vector<float> cdf;
  /* There is no much smartness going around lower resolution for the CDF table,
   * this just to match the old code from pixel filter so it all stays exactly
   * the same and no regression tests are failed.
   */
  util_cdf_evaluate(resolution - 1, from, to, functor, cdf);
  util_cdf_invert(resolution, from, to, cdf, make_symmetric, inv_cdf);
}

}
