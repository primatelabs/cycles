/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "util/math_cdf.h"

#include <cassert>

#include "util/algorithm.h"

namespace ccl {

/* Invert pre-calculated CDF function. */
void util_cdf_invert(const int resolution,
                     const float from,
                     const float to,
                     const vector<float> &cdf,
                     const bool make_symmetric,
                     vector<float> &inv_cdf)
{
  const int cdf_size = cdf.size();
  assert(cdf[0] == 0.0f && cdf[cdf_size - 1] == 1.0f);

  const float inv_resolution = 1.0f / (float)resolution;
  const float range = to - from;
  inv_cdf.resize(resolution);
  if (make_symmetric) {
    const int half_size = (resolution - 1) / 2;
    for (int i = 0; i <= half_size; i++) {
      const float x = i / (float)half_size;
      int index = upper_bound(cdf.begin(), cdf.end(), x) - cdf.begin();
      float t;
      if (index < cdf_size - 1) {
        t = (x - cdf[index]) / (cdf[index + 1] - cdf[index]);
      }
      else {
        t = 0.0f;
        index = cdf_size - 1;
      }
      const float y = ((index + t) / (resolution - 1)) * (2.0f * range);
      inv_cdf[half_size + i] = 0.5f * (1.0f + y);
      inv_cdf[half_size - i] = 0.5f * (1.0f - y);
    }
  }
  else {
    for (int i = 0; i < resolution; i++) {
      const float x = (i + 0.5f) * inv_resolution;
      int index = upper_bound(cdf.begin(), cdf.end(), x) - cdf.begin() - 1;
      float t;
      if (index < cdf_size - 1) {
        t = (x - cdf[index]) / (cdf[index + 1] - cdf[index]);
      }
      else {
        t = 0.0f;
        index = resolution;
      }
      inv_cdf[i] = from + range * (index + t) * inv_resolution;
    }
  }
}

}
