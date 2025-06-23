/* SPDX-FileCopyrightText: 2020-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "graph/node.h"

#include "scene/mesh.h"

namespace ccl {

class Volume : public Mesh {
 public:
  NODE_DECLARE

  Volume();

  NODE_SOCKET_API(float, clipping)
  NODE_SOCKET_API(float, step_size)
  NODE_SOCKET_API(bool, object_space)
  NODE_SOCKET_API(float, velocity_scale)

  void clear(bool preserve_shaders = false) override;
};

}
