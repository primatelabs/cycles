/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

/* Constant Globals */

#pragma once

#include "kernel/types.h"

#include "kernel/integrator/state.h"
#include "kernel/util/profiler.h"

#include "util/color.h"
#include "util/texture.h"

namespace ccl {

/* Not actually used, just a nullptr pointer that gets passed everywhere, which we
 * hope gets optimized out by the compiler. */
struct KernelGlobalsGPU {
  int unused[1];
};
using KernelGlobals = const ccl_global KernelGlobalsGPU *ccl_restrict;

struct KernelParamsCUDA {
  /* Global scene data and textures */
  KernelData data;
#define KERNEL_DATA_ARRAY(type, name) const type *name;
#include "kernel/data_arrays.h"

  /* Integrator state */
  IntegratorStateGPU integrator_state;
};

#ifdef __KERNEL_GPU__
__constant__ KernelParamsCUDA kernel_params;
#endif

/* Abstraction macros */
#define kernel_data kernel_params.data
#define kernel_data_fetch(name, index) kernel_params.name[(index)]
#define kernel_data_array(name) (kernel_params.name)
#define kernel_integrator_state kernel_params.integrator_state

}
