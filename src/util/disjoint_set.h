/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/array.h"
#include <utility>

namespace ccl {

class DisjointSet {
 private:
  array<size_t> parents;
  array<size_t> ranks;

 public:
  DisjointSet(const size_t size) : parents(size), ranks(size)
  {
    for (size_t i = 0; i < size; i++) {
      parents[i] = i;
      ranks[i] = 0;
    }
  }

  size_t find(size_t x)
  {
    size_t root = x;
    while (parents[root] != root) {
      root = parents[root];
    }
    while (parents[x] != root) {
      const size_t parent = parents[x];
      parents[x] = root;
      x = parent;
    }
    return root;
  }

  void join(const size_t x, const size_t y)
  {
    size_t x_root = find(x);
    size_t y_root = find(y);

    if (x_root == y_root) {
      return;
    }

    if (ranks[x_root] < ranks[y_root]) {
      std::swap(x_root, y_root);
    }
    parents[y_root] = x_root;

    if (ranks[x_root] == ranks[y_root]) {
      ranks[x_root]++;
    }
  }
};

}
