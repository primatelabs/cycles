/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

/* Optimized CPU kernel entry points. This file is compiled with AVX2
 * optimization flags and nearly all functions inlined, while kernel.cpp
 * is compiled without for other CPU's. */

#include "util/optimization.h"

#ifndef WITH_CYCLES_OPTIMIZED_KERNEL_AVX2
#  define KERNEL_STUB
#endif /* WITH_CYCLES_OPTIMIZED_KERNEL_AVX2 */

#include "kernel/device/cpu/globals.h"
#include "kernel/device/cpu/kernel.h"
#define KERNEL_ARCH cpu_avx2
#include "kernel/device/cpu/kernel_arch_impl.h"
