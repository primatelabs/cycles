/* SPDX-FileCopyrightText: 2021-2022 Intel Corporation
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

/* NOTE(@nsirgien): With SYCL we can't declare __constant__ global variable, which will be
 * accessible from device code, like it has been done for Cycles CUDA backend. So, the backend will
 * allocate this "constant" memory regions and store pointers to them in oneAPI context class */

struct IntegratorStateGPU;
struct IntegratorQueueCounter;

struct KernelGlobalsGPU {

#define KERNEL_DATA_ARRAY(type, name) const type *__##name = nullptr;
#include "kernel/data_arrays.h"
#undef KERNEL_DATA_ARRAY
  IntegratorStateGPU *integrator_state;
  const KernelData *__data;

#ifdef WITH_ONEAPI_SYCL_HOST_TASK
  size_t nd_item_local_id_0;
  size_t nd_item_local_range_0;
  size_t nd_item_group_id_0;
  size_t nd_item_group_range_0;
  size_t nd_item_global_id_0;
  size_t nd_item_global_range_0;
#else
  sycl::kernel_handler kernel_handler;
#endif
};

using KernelGlobals = ccl_global KernelGlobalsGPU *ccl_restrict;

#define kernel_data (*(__data))
#define kernel_integrator_state (*(integrator_state))

/* data lookup defines */

#define kernel_data_fetch(name, index) __##name[index]
#define kernel_data_array(name) __##name

}
