/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

/* TBB includes <windows.h>, do it ourselves first so we are sure
 * WIN32_LEAN_AND_MEAN and similar are defined beforehand. */
#ifdef _WIN32
#  include "util/windows.h"
#endif

#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#if TBB_INTERFACE_VERSION_MAJOR >= 10
#  define WITH_TBB_GLOBAL_CONTROL
#  define TBB_PREVIEW_GLOBAL_CONTROL 1
#  include <tbb/global_control.h>
#endif

namespace ccl {

using tbb::blocked_range;
using tbb::enumerable_thread_specific;
using tbb::parallel_for;
using tbb::parallel_for_each;
using tbb::parallel_reduce;

static inline void thread_capture_fp_settings()
{
#if TBB_INTERFACE_VERSION_MAJOR >= 12
  tbb::task_group_context *ctx = tbb::task::current_context();
#else
  tbb::task_group_context *ctx = tbb::task::self().group();
#endif
  if (ctx) {
    ctx->capture_fp_settings();
  }
}

static inline void parallel_for_cancel()
{
#if TBB_INTERFACE_VERSION_MAJOR >= 12
  tbb::task_group_context *ctx = tbb::task::current_context();
  if (ctx) {
    ctx->cancel_group_execution();
  }
#else
  tbb::task::self().cancel_group_execution();
#endif
}

}
