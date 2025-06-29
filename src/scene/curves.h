/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/array.h"
#include "util/types.h"

namespace ccl {

class Device;
class DeviceScene;
class Progress;
class Scene;

void curvebounds(float *lower, float *upper, float3 *p, const int dim);

class ParticleCurveData {

 public:
  ParticleCurveData();
  ~ParticleCurveData();

  array<int> psys_firstcurve;
  array<int> psys_curvenum;
  array<int> psys_shader;

  array<float> psys_rootradius;
  array<float> psys_tipradius;
  array<float> psys_shape;
  array<bool> psys_closetip;

  array<int> curve_firstkey;
  array<int> curve_keynum;
  array<float> curve_length;
  array<float2> curve_uv;
  array<float4> curve_vcol;

  array<float3> curvekey_co;
  array<float> curvekey_time;
};

}
