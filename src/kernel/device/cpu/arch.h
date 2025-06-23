/* SPDX-FileCopyrightText: 2025 Primate Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

// On x86-64, the minimum architecture is SSE2 (which differs from upstream
// where the minimum architecture is SSE4.2).

#if (defined(__x86_64__) || defined(_M_X64))
#  define __KERNEL_SSE__
#  define __KERNEL_SSE2__
#endif

// On AVX2, enable optimized AVX2 kernels.

#if defined(__AVX2__)
# define __KERNEL_SSE3__
# define __KERNEL_SSSE3__
# define __KERNEL_SSE42__
# define __KERNEL_AVX__
# define __KERNEL_AVX2__
# define WITH_CYCLES_OPTIMIZED_KERNEL_AVX2
#endif

// On ARM, the minimum architecture is SSE4.2 (via SSE2NEON). Most code is
// shared with SSE, some specializations for performance and compatibility
// are made made testing for __KERNEL_NEON__.

#if (defined(__ARM_NEON) || defined(_M_ARM64))
# define __KERNEL_NEON__
# define __KERNEL_SSE__
# define __KERNEL_SSE2__
# define __KERNEL_SSE3__
# define __KERNEL_SSE42__
#endif
