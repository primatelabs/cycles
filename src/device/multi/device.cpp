/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "device/multi/device.h"
#include "device/device.h"
#include "device/queue.h"

#include <cstdlib>
#include <functional>

#include "bvh/multi.h"

#include "scene/geometry.h"

#include "util/list.h"
#include "util/map.h"

namespace ccl {

class MultiDevice : public Device {
 public:
  struct SubDevice {
    Stats stats;
    unique_ptr<Device> device;
    map<device_ptr, device_ptr> ptr_map;
    int peer_island_index = -1;
  };

  list<SubDevice> devices;
  device_ptr unique_key = 1;
  vector<vector<SubDevice *>> peer_islands;

  MultiDevice(const DeviceInfo &info_, Stats &stats, Profiler &profiler, bool headless)
      : Device(info_, stats, profiler, headless)
  {
    verify_hardware_raytracing();

    for (const DeviceInfo &subinfo : this->info.multi_devices) {
      /* Always add CPU devices at the back since GPU devices can change
       * host memory pointers, which CPU uses as device pointer. */
      SubDevice *sub;
      if (subinfo.type == DEVICE_CPU) {
        devices.emplace_back();
        sub = &devices.back();
      }
      else {
        devices.emplace_front();
        sub = &devices.front();
      }

      /* The pointer to 'sub->stats' will stay valid even after new devices
       * are added, since 'devices' is a linked list. */
      sub->device = Device::create(subinfo, sub->stats, profiler, headless);
    }

    /* Build a list of peer islands for the available render devices */
    for (SubDevice &sub : devices) {
      /* First ensure that every device is in at least once peer island */
      if (sub.peer_island_index < 0) {
        peer_islands.emplace_back();
        sub.peer_island_index = (int)peer_islands.size() - 1;
        peer_islands[sub.peer_island_index].push_back(&sub);
      }

      if (!info.has_peer_memory) {
        continue;
      }

      /* Second check peer access between devices and fill up the islands accordingly */
      for (SubDevice &peer_sub : devices) {
        if (peer_sub.peer_island_index < 0 &&
            peer_sub.device->info.type == sub.device->info.type &&
            peer_sub.device->check_peer_access(sub.device.get()))
        {
          peer_sub.peer_island_index = sub.peer_island_index;
          peer_islands[sub.peer_island_index].push_back(&peer_sub);
        }
      }
    }
  }

  void verify_hardware_raytracing()
  {
    /* Determine if we can use hardware ray-tracing. It is only supported if all selected
     * GPU devices support it. Both the backends and scene update code do not support mixed
     * BVH2 and hardware raytracing. The CPU device will ignore this setting. */
    bool have_disabled_hardware_rt = false;
    bool have_enabled_hardware_rt = false;

    for (const DeviceInfo &subinfo : info.multi_devices) {
      if (subinfo.type != DEVICE_CPU) {
        if (subinfo.use_hardware_raytracing) {
          have_enabled_hardware_rt = true;
        }
        else {
          have_disabled_hardware_rt = true;
        }
      }
    }

    info.use_hardware_raytracing = have_enabled_hardware_rt && !have_disabled_hardware_rt;

    for (DeviceInfo &subinfo : info.multi_devices) {
      if (subinfo.type != DEVICE_CPU) {
        subinfo.use_hardware_raytracing = info.use_hardware_raytracing;
      }
    }
  }

  const string &error_message() override
  {
    error_msg.clear();

    for (SubDevice &sub : devices) {
      error_msg += sub.device->error_message();
    }

    return error_msg;
  }

  BVHLayoutMask get_bvh_layout_mask(const uint kernel_features) const override
  {
    BVHLayoutMask bvh_layout_mask = BVH_LAYOUT_ALL;
    BVHLayoutMask bvh_layout_mask_all = BVH_LAYOUT_NONE;
    for (const SubDevice &sub_device : devices) {
      BVHLayoutMask device_bvh_layout_mask = sub_device.device->get_bvh_layout_mask(
          kernel_features);
      bvh_layout_mask &= device_bvh_layout_mask;
      bvh_layout_mask_all |= device_bvh_layout_mask;
    }

    /* With multiple OptiX devices, every device needs its own acceleration structure */
    if (bvh_layout_mask == BVH_LAYOUT_OPTIX) {
      return BVH_LAYOUT_MULTI_OPTIX;
    }

    /* With multiple Metal devices, every device needs its own acceleration structure */
    if (bvh_layout_mask == BVH_LAYOUT_METAL) {
      return BVH_LAYOUT_MULTI_METAL;
    }

    if (bvh_layout_mask == BVH_LAYOUT_HIPRT) {
      return BVH_LAYOUT_MULTI_HIPRT;
    }

    /* With multiple oneAPI devices, every device needs its own acceleration structure */
    if (bvh_layout_mask == BVH_LAYOUT_EMBREEGPU) {
      return BVH_LAYOUT_MULTI_EMBREEGPU;
    }

    /* When devices do not share a common BVH layout, fall back to creating one for each */
    const BVHLayoutMask BVH_LAYOUT_OPTIX_EMBREE = (BVH_LAYOUT_OPTIX | BVH_LAYOUT_EMBREE);
    if ((bvh_layout_mask_all & BVH_LAYOUT_OPTIX_EMBREE) == BVH_LAYOUT_OPTIX_EMBREE) {
      return BVH_LAYOUT_MULTI_OPTIX_EMBREE;
    }
    const BVHLayoutMask BVH_LAYOUT_METAL_EMBREE = (BVH_LAYOUT_METAL | BVH_LAYOUT_EMBREE);
    if ((bvh_layout_mask_all & BVH_LAYOUT_METAL_EMBREE) == BVH_LAYOUT_METAL_EMBREE) {
      return BVH_LAYOUT_MULTI_METAL_EMBREE;
    }
    const BVHLayoutMask BVH_LAYOUT_EMBREEGPU_EMBREE = (BVH_LAYOUT_EMBREEGPU | BVH_LAYOUT_EMBREE);
    if ((bvh_layout_mask_all & BVH_LAYOUT_EMBREEGPU_EMBREE) == BVH_LAYOUT_EMBREEGPU_EMBREE) {
      return BVH_LAYOUT_MULTI_EMBREEGPU_EMBREE;
    }

    const BVHLayoutMask BVH_LAYOUT_HIPRT_EMBREE = (BVH_LAYOUT_HIPRT | BVH_LAYOUT_EMBREE);
    if ((bvh_layout_mask_all & BVH_LAYOUT_HIPRT_EMBREE) == BVH_LAYOUT_HIPRT_EMBREE) {
      return BVH_LAYOUT_MULTI_HIPRT_EMBREE;
    }

    return bvh_layout_mask;
  }

  bool load_kernels(const uint kernel_features) override
  {
    for (SubDevice &sub : devices) {
      if (!sub.device->load_kernels(kernel_features)) {
        return false;
      }
    }

    return true;
  }

  bool load_osl_kernels() override
  {
    for (SubDevice &sub : devices) {
      if (!sub.device->load_osl_kernels()) {
        return false;
      }
    }

    return true;
  }

  void build_bvh(BVH *bvh, Progress &progress, bool refit) override
  {
    /* Try to build and share a single acceleration structure, if possible */
    if (bvh->params.bvh_layout == BVH_LAYOUT_BVH2 || bvh->params.bvh_layout == BVH_LAYOUT_EMBREE) {
      devices.back().device->build_bvh(bvh, progress, refit);
      return;
    }

    assert(bvh->params.bvh_layout == BVH_LAYOUT_MULTI_OPTIX ||
           bvh->params.bvh_layout == BVH_LAYOUT_MULTI_METAL ||
           bvh->params.bvh_layout == BVH_LAYOUT_MULTI_HIPRT ||
           bvh->params.bvh_layout == BVH_LAYOUT_MULTI_EMBREEGPU ||
           bvh->params.bvh_layout == BVH_LAYOUT_MULTI_OPTIX_EMBREE ||
           bvh->params.bvh_layout == BVH_LAYOUT_MULTI_METAL_EMBREE ||
           bvh->params.bvh_layout == BVH_LAYOUT_MULTI_HIPRT_EMBREE ||
           bvh->params.bvh_layout == BVH_LAYOUT_MULTI_EMBREEGPU_EMBREE);

    BVHMulti *const bvh_multi = static_cast<BVHMulti *>(bvh);
    bvh_multi->sub_bvhs.resize(devices.size());

    /* Temporarily move ownership of BVH on geometry to this vector, to swap
     * it for each sub device. Need to find a better way to handle this. */
    vector<unique_ptr<BVH>> geom_bvhs;
    geom_bvhs.reserve(bvh->geometry.size());
    for (Geometry *geom : bvh->geometry) {
      geom_bvhs.push_back(std::move(geom->bvh));
    }

    /* Broadcast acceleration structure build to all render devices */
    size_t i = 0;
    for (SubDevice &sub : devices) {
      /* Change geometry BVH pointers to the sub BVH */
      for (size_t k = 0; k < bvh->geometry.size(); ++k) {
        bvh->geometry[k]->bvh.release();  // NOLINT: was not actually the owner
        bvh->geometry[k]->bvh.reset(
            static_cast<BVHMulti *>(geom_bvhs[k].get())->sub_bvhs[i].get());
      }

      if (!bvh_multi->sub_bvhs[i]) {
        BVHParams params = bvh->params;
        if (bvh->params.bvh_layout == BVH_LAYOUT_MULTI_OPTIX) {
          params.bvh_layout = BVH_LAYOUT_OPTIX;
        }
        else if (bvh->params.bvh_layout == BVH_LAYOUT_MULTI_METAL) {
          params.bvh_layout = BVH_LAYOUT_METAL;
        }
        else if (bvh->params.bvh_layout == BVH_LAYOUT_MULTI_HIPRT) {
          params.bvh_layout = BVH_LAYOUT_HIPRT;
        }
        else if (bvh->params.bvh_layout == BVH_LAYOUT_MULTI_EMBREEGPU) {
          params.bvh_layout = BVH_LAYOUT_EMBREEGPU;
        }
        else if (bvh->params.bvh_layout == BVH_LAYOUT_MULTI_OPTIX_EMBREE) {
          params.bvh_layout = sub.device->info.type == DEVICE_OPTIX ? BVH_LAYOUT_OPTIX :
                                                                      BVH_LAYOUT_EMBREE;
        }
        else if (bvh->params.bvh_layout == BVH_LAYOUT_MULTI_METAL_EMBREE) {
          params.bvh_layout = sub.device->info.type == DEVICE_METAL ? BVH_LAYOUT_METAL :
                                                                      BVH_LAYOUT_EMBREE;
        }
        else if (bvh->params.bvh_layout == BVH_LAYOUT_MULTI_HIPRT_EMBREE) {
          params.bvh_layout = sub.device->info.type == DEVICE_HIP ? BVH_LAYOUT_HIPRT :
                                                                    BVH_LAYOUT_EMBREE;
        }
        else if (bvh->params.bvh_layout == BVH_LAYOUT_MULTI_EMBREEGPU_EMBREE) {
          params.bvh_layout = sub.device->info.type == DEVICE_ONEAPI ? BVH_LAYOUT_EMBREEGPU :
                                                                       BVH_LAYOUT_EMBREE;
        }
        /* Skip building a bottom level acceleration structure for non-instanced geometry on Embree
         * (since they are put into the top level directly, see bvh_embree.cpp) */
        if (!params.top_level && params.bvh_layout == BVH_LAYOUT_EMBREE &&
            !bvh->geometry[0]->is_instanced())
        {
          i++;
          continue;
        }

        bvh_multi->sub_bvhs[i] = BVH::create(
            params, bvh->geometry, bvh->objects, sub.device.get());
      }

      sub.device->build_bvh(bvh_multi->sub_bvhs[i].get(), progress, refit);
      i++;
    }

    /* Change BVH ownership back to Geometry. */
    for (size_t k = 0; k < bvh->geometry.size(); ++k) {
      bvh->geometry[k]->bvh.release();  // NOLINT: was not actually the owner
      bvh->geometry[k]->bvh = std::move(geom_bvhs[k]);
    }
  }

  OSLGlobals *get_cpu_osl_memory() override
  {
    /* Always return the OSL memory of the CPU device (this works since the constructor above
     * guarantees that CPU devices are always added to the back). */
    if (devices.size() > 1 && devices.back().device->info.type != DEVICE_CPU) {
      return nullptr;
    }
    return devices.back().device->get_cpu_osl_memory();
  }

  bool is_resident(device_ptr key, Device *sub_device) override
  {
    for (SubDevice &sub : devices) {
      if (sub.device.get() == sub_device) {
        return find_matching_mem_device(key, sub)->device.get() == sub_device;
      }
    }
    return false;
  }

  SubDevice *find_matching_mem_device(device_ptr key, SubDevice &sub)
  {
    assert(key != 0 && (sub.peer_island_index >= 0 || sub.ptr_map.find(key) != sub.ptr_map.end()));

    /* Get the memory owner of this key (first try current device, then peer devices) */
    SubDevice *owner_sub = &sub;
    if (owner_sub->ptr_map.find(key) == owner_sub->ptr_map.end()) {
      for (SubDevice *island_sub : peer_islands[sub.peer_island_index]) {
        if (island_sub != owner_sub && island_sub->ptr_map.find(key) != island_sub->ptr_map.end())
        {
          owner_sub = island_sub;
        }
      }
    }
    return owner_sub;
  }

  SubDevice *find_suitable_mem_device(device_ptr key, const vector<SubDevice *> &island)
  {
    assert(!island.empty());

    /* Get the memory owner of this key or the device with the lowest memory usage when new */
    SubDevice *owner_sub = island.front();
    for (SubDevice *island_sub : island) {
      if (key ? (island_sub->ptr_map.find(key) != island_sub->ptr_map.end()) :
                (island_sub->device->stats.mem_used < owner_sub->device->stats.mem_used))
      {
        owner_sub = island_sub;
      }
    }
    return owner_sub;
  }

  device_ptr find_matching_mem(device_ptr key, SubDevice &sub)
  {
    return find_matching_mem_device(key, sub)->ptr_map[key];
  }

  void *host_alloc(const MemoryType type, const size_t size) override
  {
    for (SubDevice &sub : devices) {
      if (sub.device->info.type != DEVICE_CPU) {
        return sub.device->host_alloc(type, size);
      }
    }

    return Device::host_alloc(type, size);
  }

  void host_free(const MemoryType type, void *host_pointer, const size_t size) override
  {
    for (SubDevice &sub : devices) {
      if (sub.device->info.type != DEVICE_CPU) {
        sub.device->host_free(type, host_pointer, size);
        return;
      }
    }

    Device::host_free(type, host_pointer, size);
  }

  void mem_alloc(device_memory &mem) override
  {
    device_ptr key = unique_key++;

    assert(mem.type == MEM_READ_ONLY || mem.type == MEM_READ_WRITE || mem.type == MEM_DEVICE_ONLY);
    /* The remaining memory types can be distributed across devices */
    for (const vector<SubDevice *> &island : peer_islands) {
      SubDevice *owner_sub = find_suitable_mem_device(key, island);
      mem.device = owner_sub->device.get();
      mem.device_pointer = 0;
      mem.device_size = 0;

      owner_sub->device->mem_alloc(mem);
      owner_sub->ptr_map[key] = mem.device_pointer;
    }

    mem.device = this;
    mem.device_pointer = key;
    stats.mem_alloc(mem.device_size);
  }

  void mem_copy_to(device_memory &mem) override
  {
    device_ptr existing_key = mem.device_pointer;
    device_ptr key = (existing_key) ? existing_key : unique_key++;
    size_t existing_size = mem.device_size;

    for (const vector<SubDevice *> &island : peer_islands) {
      SubDevice *owner_sub = find_suitable_mem_device(existing_key, island);
      mem.device = owner_sub->device.get();
      mem.device_pointer = (existing_key) ? owner_sub->ptr_map[existing_key] : 0;
      mem.device_size = existing_size;

      owner_sub->device->mem_copy_to(mem);
      owner_sub->ptr_map[key] = mem.device_pointer;

      if (mem.type == MEM_GLOBAL || mem.type == MEM_TEXTURE) {
        /* Need to create texture objects and update pointer in kernel globals on all devices */
        for (SubDevice *island_sub : island) {
          if (island_sub != owner_sub) {
            island_sub->device->mem_copy_to(mem);
          }
        }
      }
    }

    mem.device = this;
    mem.device_pointer = key;
    stats.mem_alloc(mem.device_size - existing_size);
  }

  void mem_move_to_host(device_memory &mem) override
  {
    assert(mem.type == MEM_GLOBAL || mem.type == MEM_TEXTURE);

    device_ptr existing_key = mem.device_pointer;
    device_ptr key = (existing_key) ? existing_key : unique_key++;
    size_t existing_size = mem.device_size;

    for (const vector<SubDevice *> &island : peer_islands) {
      SubDevice *owner_sub = find_suitable_mem_device(existing_key, island);
      mem.device = owner_sub->device.get();
      mem.device_pointer = (existing_key) ? owner_sub->ptr_map[existing_key] : 0;
      mem.device_size = existing_size;

      if (!owner_sub->device->is_shared(
              mem.shared_pointer, mem.device_pointer, owner_sub->device.get()))
      {
        owner_sub->device->mem_move_to_host(mem);
        owner_sub->ptr_map[key] = mem.device_pointer;

        /* Need to create texture objects and update pointer in kernel globals on all devices */
        for (SubDevice *island_sub : island) {
          if (island_sub != owner_sub) {
            island_sub->device->mem_move_to_host(mem);
          }
        }
      }
    }

    mem.device = this;
    mem.device_pointer = key;
    stats.mem_alloc(mem.device_size - existing_size);
  }

  bool is_shared(const void *shared_pointer, const device_ptr key, Device *sub_device) override
  {
    if (key == 0) {
      return false;
    }

    for (const SubDevice &sub : devices) {
      if (sub.device.get() == sub_device) {
        return sub_device->is_shared(shared_pointer, sub.ptr_map.at(key), sub_device);
      }
    }

    assert(!"is_shared failed to find matching device");
    return false;
  }

  void mem_copy_from(
      device_memory &mem, const size_t y, size_t w, const size_t h, size_t elem) override
  {
    device_ptr key = mem.device_pointer;
    const size_t sub_h = h / devices.size();
    size_t i = 0;

    for (SubDevice &sub : devices) {
      size_t sy = y + i * sub_h;
      size_t sh = (i == (size_t)devices.size() - 1) ? h - sub_h * i : sub_h;

      SubDevice *owner_sub = find_matching_mem_device(key, sub);
      mem.device = owner_sub->device.get();
      mem.device_pointer = owner_sub->ptr_map[key];

      owner_sub->device->mem_copy_from(mem, sy, w, sh, elem);
      i++;
    }

    mem.device = this;
    mem.device_pointer = key;
  }

  void mem_zero(device_memory &mem) override
  {
    device_ptr existing_key = mem.device_pointer;
    device_ptr key = (existing_key) ? existing_key : unique_key++;
    size_t existing_size = mem.device_size;

    for (const vector<SubDevice *> &island : peer_islands) {
      SubDevice *owner_sub = find_suitable_mem_device(existing_key, island);
      mem.device = owner_sub->device.get();
      mem.device_pointer = (existing_key) ? owner_sub->ptr_map[existing_key] : 0;
      mem.device_size = existing_size;

      owner_sub->device->mem_zero(mem);
      owner_sub->ptr_map[key] = mem.device_pointer;
    }

    mem.device = this;
    mem.device_pointer = key;
    stats.mem_alloc(mem.device_size - existing_size);
  }

  void mem_free(device_memory &mem) override
  {
    device_ptr key = mem.device_pointer;
    size_t existing_size = mem.device_size;

    /* Free memory that was allocated for all devices (see above) on each device */
    for (const vector<SubDevice *> &island : peer_islands) {
      SubDevice *owner_sub = find_matching_mem_device(key, *island.front());
      mem.device = owner_sub->device.get();
      mem.device_pointer = owner_sub->ptr_map[key];
      mem.device_size = existing_size;

      owner_sub->device->mem_free(mem);
      owner_sub->ptr_map.erase(owner_sub->ptr_map.find(key));

      if (mem.type == MEM_TEXTURE) {
        /* Free texture objects on all devices */
        for (SubDevice *island_sub : island) {
          if (island_sub != owner_sub) {
            island_sub->device->mem_free(mem);
          }
        }
      }
    }

    mem.device = this;
    mem.device_pointer = 0;
    mem.device_size = 0;
    stats.mem_free(existing_size);
  }

  void const_copy_to(const char *name, void *host, const size_t size) override
  {
    for (SubDevice &sub : devices) {
      sub.device->const_copy_to(name, host, size);
    }
  }

  int device_number(Device *sub_device) override
  {
    int i = 0;

    for (SubDevice &sub : devices) {
      if (sub.device.get() == sub_device) {
        return i;
      }
      i++;
    }

    return -1;
  }

  void foreach_device(const std::function<void(Device *)> &callback) override
  {
    for (SubDevice &sub : devices) {
      sub.device->foreach_device(callback);
    }
  }
};

unique_ptr<Device> device_multi_create(const DeviceInfo &info,
                                       Stats &stats,
                                       Profiler &profiler,
                                       bool headless)
{
  return make_unique<MultiDevice>(info, stats, profiler, headless);
}

}
