/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

/* Functions to evaluate displacement shader. */

#pragma once

#include "kernel/globals.h"

#ifdef __SVM__
#  include "kernel/svm/svm.h"
#endif
#ifdef __OSL__
#  include "kernel/osl/osl.h"
#endif

namespace ccl {

template<typename ConstIntegratorGenericState>
ccl_device void displacement_shader_eval(KernelGlobals kg,
                                         ConstIntegratorGenericState state,
                                         ccl_private ShaderData *sd)
{
  sd->num_closure = 0;
  sd->num_closure_left = 0;

  /* this will modify sd->P */
#ifdef __OSL__
  if (kernel_data.kernel_features & KERNEL_FEATURE_OSL) {
    osl_eval_nodes<SHADER_TYPE_DISPLACEMENT>(kg, state, sd, 0);
  }
  else
#endif
  {
#ifdef __SVM__
    svm_eval_nodes<KERNEL_FEATURE_NODE_MASK_DISPLACEMENT, SHADER_TYPE_DISPLACEMENT>(
        kg, state, sd, nullptr, 0);
#endif
  }
}

}
