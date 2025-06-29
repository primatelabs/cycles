/* SPDX-FileCopyrightText: 2009-2010 NVIDIA Corporation
 * SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Adapted code from NVIDIA Corporation. */

#pragma once

#include "bvh/params.h"
#include "util/array.h"
#include "util/types.h"
#include "util/unique_ptr.h"
#include "util/vector.h"

namespace ccl {

class BoundBox;
class BVHNode;
class BVHParams;
class Device;
class DeviceScene;
class Geometry;
class LeafNode;
class Object;
class Progress;
class Stats;

#define BVH_ALIGN 4096   // NOLINT
#define TRI_NODE_SIZE 3  // NOLINT

/* Packed BVH
 *
 * BVH stored as it will be used for traversal on the rendering device. */

struct PackedBVH {
  /* BVH nodes storage, one node is 4x int4, and contains two bounding boxes,
   * and child, triangle or object indexes depending on the node type */
  array<int4> nodes;
  /* BVH leaf nodes storage. */
  array<int4> leaf_nodes;
  /* object index to BVH node index mapping for instances */
  array<int> object_node;
  /* primitive type - triangle or strand */
  array<int> prim_type;
  /* Visibility visibilities for primitives. */
  array<uint> prim_visibility;
  /* mapping from BVH primitive index to true primitive index, as primitives
   * may be duplicated due to spatial splits. -1 for instances. */
  array<int> prim_index;
  /* mapping from BVH primitive index, to the object id of that primitive. */
  array<int> prim_object;
  /* Time range of BVH primitive. */
  array<float2> prim_time;

  /* index of the root node. */
  int root_index;

  PackedBVH()
  {
    root_index = 0;
  }
};

/* BVH */

class BVH {
 public:
  BVHParams params;
  vector<Geometry *> geometry;
  vector<Object *> objects;

  static unique_ptr<BVH> create(const BVHParams &params,
                                const vector<Geometry *> &geometry,
                                const vector<Object *> &objects,
                                Device *device);
  virtual ~BVH() = default;

  virtual void replace_geometry(const vector<Geometry *> &geometry,
                                const vector<Object *> &objects)
  {
    this->geometry = geometry;
    this->objects = objects;
  }

 protected:
  BVH(const BVHParams &params,
      const vector<Geometry *> &geometry,
      const vector<Object *> &objects);
};

}
