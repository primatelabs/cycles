/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/map.h"
#include "util/param.h"

namespace ccl {

/* Enum
 *
 * Utility class for enum values. */

struct NodeEnum {
  bool empty() const
  {
    return left.empty();
  }
  void insert(const char *x, const int y)
  {
    const string ustr_x(x);

    left[ustr_x] = y;
    right[y] = ustr_x;
  }

  bool exists(string x) const
  {
    return left.find(x) != left.end();
  }
  bool exists(const int y) const
  {
    return right.find(y) != right.end();
  }

  int operator[](const char *x) const
  {
    return left.find(string(x))->second;
  }
  int operator[](string x) const
  {
    return left.find(x)->second;
  }
  string operator[](int y) const
  {
    return right.find(y)->second;
  }

  unordered_map<string, int>::const_iterator begin() const
  {
    return left.begin();
  }
  unordered_map<string, int>::const_iterator end() const
  {
    return left.end();
  }

 private:
  unordered_map<string, int> left;
  unordered_map<int, string> right;
};

}
