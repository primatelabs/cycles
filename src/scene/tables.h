/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/list.h"
#include "util/vector.h"

namespace ccl {

class Device;
class DeviceScene;
class Scene;

enum { TABLE_CHUNK_SIZE = 256 };
enum { TABLE_OFFSET_INVALID = -1 };

class LookupTables {
  bool need_update_;

 public:
  struct Table {
    size_t offset;
    size_t size;
  };

  list<Table> lookup_tables;

  LookupTables();
  ~LookupTables();

  void device_update(Device *device, DeviceScene *dscene, Scene *scene);
  void device_free(Device *device, DeviceScene *dscene);

  bool need_update() const;

  size_t add_table(DeviceScene *dscene, vector<float> &data);
  void remove_table(size_t *offset);
};

}
