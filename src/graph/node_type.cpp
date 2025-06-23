/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "graph/node_type.h"

#include "util/transform.h"

namespace ccl {

/* Node Socket Type */

size_t SocketType::size() const
{
  return size(type);
}

bool SocketType::is_array() const
{
  return (type >= BOOLEAN_ARRAY);
}

size_t SocketType::size(Type type)
{
  switch (type) {
    case UNDEFINED:
    case NUM_TYPES:
      return 0;

    case BOOLEAN:
      return sizeof(bool);
    case FLOAT:
      return sizeof(float);
    case INT:
      return sizeof(int);
    case UINT:
      return sizeof(uint);
    case UINT64:
      return sizeof(uint64_t);
    case COLOR:
      return sizeof(float3);
    case VECTOR:
      return sizeof(float3);
    case POINT:
      return sizeof(float3);
    case NORMAL:
      return sizeof(float3);
    case POINT2:
      return sizeof(float2);
    case CLOSURE:
      return 0;
    case STRING:
      return sizeof(string);
    case ENUM:
      return sizeof(int);
    case TRANSFORM:
      return sizeof(Transform);
    case NODE:
      return sizeof(void *);

    case BOOLEAN_ARRAY:
      return sizeof(array<bool>);
    case FLOAT_ARRAY:
      return sizeof(array<float>);
    case INT_ARRAY:
      return sizeof(array<int>);
    case COLOR_ARRAY:
      return sizeof(array<float3>);
    case VECTOR_ARRAY:
      return sizeof(array<float3>);
    case POINT_ARRAY:
      return sizeof(array<float3>);
    case NORMAL_ARRAY:
      return sizeof(array<float3>);
    case POINT2_ARRAY:
      return sizeof(array<float2>);
    case STRING_ARRAY:
      return sizeof(array<string>);
    case TRANSFORM_ARRAY:
      return sizeof(array<Transform>);
    case NODE_ARRAY:
      return sizeof(array<void *>);
  }

  assert(0);
  return 0;
}

size_t SocketType::max_size()
{
  return sizeof(Transform);
}

void *SocketType::zero_default_value()
{
  static Transform zero_transform = transform_zero();
  return &zero_transform;
}

string SocketType::type_name(Type type)
{
  static const string names[] = {string("undefined"),

                                  string("boolean"),       string("float"),
                                  string("int"),           string("uint"),
                                  string("uint64"),        string("color"),
                                  string("vector"),        string("point"),
                                  string("normal"),        string("point2"),
                                  string("closure"),       string("string"),
                                  string("enum"),          string("transform"),
                                  string("node"),

                                  string("array_boolean"), string("array_float"),
                                  string("array_int"),     string("array_color"),
                                  string("array_vector"),  string("array_point"),
                                  string("array_normal"),  string("array_point2"),
                                  string("array_string"),  string("array_transform"),
                                  string("array_node")};

  constexpr size_t num_names = sizeof(names) / sizeof(*names);
  static_assert(num_names == NUM_TYPES);

  return names[(int)type];
}

bool SocketType::is_float3(Type type)
{
  return (type == COLOR || type == VECTOR || type == POINT || type == NORMAL);
}

/* Node Type */

NodeType::NodeType(Type type, const NodeType *base) : type(type), base(base)
{
  if (base) {
    /* Inherit sockets. */
    inputs = base->inputs;
    outputs = base->outputs;
  }
}

NodeType::~NodeType() = default;

void NodeType::register_input(string name,
                              string ui_name,
                              SocketType::Type type,
                              const int struct_offset,
                              const void *default_value,
                              const NodeEnum *enum_values,
                              const NodeType *node_type,
                              const int flags,
                              const int extra_flags)
{
  SocketType socket;
  socket.name = name;
  socket.ui_name = ui_name;
  socket.type = type;
  socket.struct_offset = struct_offset;
  socket.default_value = default_value;
  socket.enum_values = enum_values;
  socket.node_type = node_type;
  socket.flags = flags | extra_flags;
  assert(inputs.size() < std::numeric_limits<SocketModifiedFlags>::digits);
  socket.modified_flag_bit = (1ull << inputs.size());
  inputs.push_back(socket);
}

void NodeType::register_output(string name, string ui_name, SocketType::Type type)
{
  SocketType socket;
  socket.name = name;
  socket.ui_name = ui_name;
  socket.type = type;
  socket.struct_offset = 0;
  socket.default_value = nullptr;
  socket.enum_values = nullptr;
  socket.node_type = nullptr;
  socket.flags = SocketType::LINKABLE;
  outputs.push_back(socket);
}

const SocketType *NodeType::find_input(string name) const
{
  for (const SocketType &socket : inputs) {
    if (socket.name == name) {
      return &socket;
    }
  }

  return nullptr;
}

const SocketType *NodeType::find_output(string name) const
{
  for (const SocketType &socket : outputs) {
    if (socket.name == name) {
      return &socket;
    }
  }

  return nullptr;
}

/* Node Type Registry */

unordered_map<string, NodeType> &NodeType::types()
{
  static unordered_map<string, NodeType> _types;
  return _types;
}

NodeType *NodeType::add(const char *name_, CreateFunc create_, Type type_, const NodeType *base_)
{
  const string name(name_);

  if (types().find(name) != types().end()) {
    fprintf(stderr, "Node type %s registered twice!\n", name_);
    assert(0);
    return nullptr;
  }

  types()[name] = NodeType(type_, base_);

  NodeType *type = &types()[name];
  type->name = name;
  type->create = create_;
  return type;
}

const NodeType *NodeType::find(string name)
{
  const unordered_map<string, NodeType>::iterator it = types().find(name);
  return (it == types().end()) ? nullptr : &it->second;
}

}
