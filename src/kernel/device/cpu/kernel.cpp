/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

/* CPU kernel entry points */

#include "kernel/device/cpu/arch.h"

#include "kernel/device/cpu/globals.h"

#include "kernel/device/cpu/kernel.h"
#define KERNEL_ARCH cpu
#include "kernel/device/cpu/kernel_arch_impl.h"

namespace ccl {

/* Memory Copy */

void kernel_const_copy(KernelGlobalsCPU *kg, const char *name, void *host, size_t /*unused*/)
{
  if (strcmp(name, "data") == 0) {
    kg->data = *(KernelData *)host;
  }
  else {
    assert(0);
  }
}

void kernel_global_memory_copy(KernelGlobalsCPU *kg,
                               const char *name,
                               void *mem,
                               const size_t size)
{
  if (false) {
  }

#define KERNEL_DATA_ARRAY(type, tname) \
  else if (strcmp(name, #tname) == 0) { \
    kg->tname.data = (type *)mem; \
    kg->tname.width = size; \
  }
#include "kernel/data_arrays.h"
  else {
    assert(0);
  }
}

}
