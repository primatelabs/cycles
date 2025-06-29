/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "graph/node.h"

namespace ccl {

enum DenoiserType {
  DENOISER_OPTIX = 2,
  DENOISER_OPENIMAGEDENOISE = 4,
  DENOISER_NUM,

  DENOISER_NONE = 0,
  DENOISER_ALL = ~0,
};

/* COnstruct human-readable string which denotes the denoiser type. */
const char *denoiserTypeToHumanReadable(DenoiserType type);

using DenoiserTypeMask = int;

enum DenoiserPrefilter {
  /* Best quality of the result without extra processing time, but requires guiding passes to be
   * noise-free. */
  DENOISER_PREFILTER_NONE = 1,

  /* Denoise color and guiding passes together.
   * Improves quality when guiding passes are noisy using least amount of extra processing time. */
  DENOISER_PREFILTER_FAST = 2,

  /* Prefilter noisy guiding passes before denoising color.
   * Improves quality when guiding passes are noisy using extra processing time. */
  DENOISER_PREFILTER_ACCURATE = 3,

  DENOISER_PREFILTER_NUM,
};

enum DenoiserQuality {
  DENOISER_QUALITY_HIGH = 1,
  DENOISER_QUALITY_BALANCED = 2,
  DENOISER_QUALITY_FAST = 3,
  DENOISER_QUALITY_NUM,
};

/* NOTE: Is not a real scene node. Using Node API for ease of (de)serialization.
 * The default values here do not really matter as they are always initialized from the
 * Integrator node. */
class DenoiseParams : public Node {
 public:
  NODE_DECLARE

  /* Apply denoiser to image. */
  bool use = false;

  /* Denoiser type. */
  DenoiserType type = DENOISER_OPENIMAGEDENOISE;

  /* Viewport start sample. */
  int start_sample = 0;

  /* Auxiliary passes. */
  bool use_pass_albedo = true;
  bool use_pass_normal = true;

  /* Configure the denoiser to use motion vectors, previous image and a temporally stable model. */
  bool temporally_stable = false;

  /* If true, then allow, if supported, OpenImageDenoise to use GPU device.
   * If false, then OpenImageDenoise will always use CPU regardless of GPU device presence. */
  bool use_gpu = true;

  DenoiserPrefilter prefilter = DENOISER_PREFILTER_FAST;
  DenoiserQuality quality = DENOISER_QUALITY_HIGH;

  static const NodeEnum *get_type_enum();
  static const NodeEnum *get_prefilter_enum();
  static const NodeEnum *get_quality_enum();

  DenoiseParams();
};

}
