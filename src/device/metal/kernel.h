/* SPDX-FileCopyrightText: 2021-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#ifdef WITH_METAL

#  include "device/kernel.h"

#  include <Metal/Metal.h>

namespace ccl {

class MetalDevice;

enum {
  METALRT_TABLE_DEFAULT,
  METALRT_TABLE_SHADOW,
  METALRT_TABLE_SHADOW_ALL,
  METALRT_TABLE_VOLUME,
  METALRT_TABLE_LOCAL,
  METALRT_TABLE_LOCAL_MBLUR,
  METALRT_TABLE_LOCAL_SINGLE_HIT,
  METALRT_TABLE_LOCAL_SINGLE_HIT_MBLUR,
  METALRT_TABLE_NUM
};

/* Pipeline State Object types */
enum MetalPipelineType {
  /* A kernel that can be used with all scenes, supporting all features.
   * It is slow to compile, but only needs to be compiled once and is then
   * cached for future render sessions. This allows a render to get underway
   * on the GPU quickly.
   */
  PSO_GENERIC,

  /* A intersection kernel that is very quick to specialize and results in faster intersection
   * kernel performance. It uses Metal function constants to replace several KernelData variables
   * with fixed constants.
   */
  PSO_SPECIALIZED_INTERSECT,

  /* A shading kernel that is slow to specialize, but results in faster shading kernel performance
   * rendered. It uses Metal function constants to replace several KernelData variables with fixed
   * constants and short-circuit all unused SVM node case handlers.
   */
  PSO_SPECIALIZED_SHADE,

  PSO_NUM
};

#  define METALRT_FEATURE_MASK \
    (KERNEL_FEATURE_HAIR | KERNEL_FEATURE_HAIR_THICK | KERNEL_FEATURE_POINTCLOUD)

const char *kernel_type_as_string(MetalPipelineType pso_type);

/* A pipeline object that can be shared between multiple instances of MetalDeviceQueue. */
class MetalKernelPipeline {
 public:
  void compile();

  int pipeline_id;
  int originating_device_id;

  id<MTLLibrary> mtlLibrary = nil;
  MetalPipelineType pso_type;
  string kernels_md5;
  size_t usage_count = 0;

  KernelData kernel_data_;
  bool use_metalrt;
  uint32_t kernel_features = 0;

  int threads_per_threadgroup;

  DeviceKernel device_kernel;
  bool loaded = false;
  id<MTLDevice> mtlDevice = nil;
  id<MTLFunction> function = nil;
  id<MTLComputePipelineState> pipeline = nil;
  int num_threads_per_block = 0;

  bool should_use_binary_archive() const;
  id<MTLFunction> make_intersection_function(const char *function_name);

  string error_str;

  NSArray *table_functions[METALRT_TABLE_NUM] = {nil};
};

/* An actively instanced pipeline that can only be used by a single instance of MetalDeviceQueue.
 */
class MetalDispatchPipeline {
 public:
  ~MetalDispatchPipeline();

  bool update(MetalDevice *metal_device, DeviceKernel kernel);
  void free_intersection_function_tables();

 private:
  friend class MetalDeviceQueue;
  friend struct ShaderCache;

  int pipeline_id = -1;

  MetalPipelineType pso_type;
  id<MTLComputePipelineState> pipeline = nil;
  int num_threads_per_block = 0;

  API_AVAILABLE(macos(11.0))
  id<MTLIntersectionFunctionTable> intersection_func_table[METALRT_TABLE_NUM] = {nil};
};

/* Cache of Metal kernels for each DeviceKernel. */
namespace MetalDeviceKernels {

int num_incomplete_specialization_requests();
int get_loaded_kernel_count(const MetalDevice *device, MetalPipelineType pso_type);
bool should_load_kernels(const MetalDevice *device, MetalPipelineType pso_type);
bool load(MetalDevice *device, MetalPipelineType pso_type);
const MetalKernelPipeline *get_best_pipeline(const MetalDevice *device, DeviceKernel kernel);
void wait_for_all();
bool is_benchmark_warmup();

/* Deinitialize all static variables, so that no code would run on application exit. */
void static_deinitialize();

} /* namespace MetalDeviceKernels */

}

#endif /* WITH_METAL */
