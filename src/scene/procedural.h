/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "graph/node.h"

#include "util/unique_ptr_vector.h"

namespace ccl {

class Progress;
class Scene;

/* A Procedural is a Node which can create other Nodes before rendering starts.
 *
 * The Procedural is supposed to be the owner of any nodes that it creates. It can also create
 * Nodes directly in the Scene (through Scene.create_node), it should still be set as the owner of
 * those Nodes.
 */
class Procedural : public Node, public NodeOwner {
 public:
  NODE_ABSTRACT_DECLARE

  explicit Procedural(const NodeType *type);
  ~Procedural() override;

  /* Called each time the ProceduralManager is tagged for an update, this function is the entry
   * point for the data generated by this Procedural. */
  virtual void generate(Scene *scene, Progress &progress) = 0;

  /* Create a node and set this Procedural as the owner. */
  template<typename T> T *create_node()
  {
    unique_ptr<T> node = make_unique<T>();
    T *node_ptr = node.get();
    node->set_owner(this);
    nodes.push_back(std::move(node));
    return node_ptr;
  }

  /* Delete a Node created and owned by this Procedural. */
  template<typename T> void delete_node(T *node)
  {
    assert(node->get_owner() == this);
    nodes.erase(node);
  }

 protected:
  unique_ptr_vector<Node> nodes;
};

class ProceduralManager {
  bool need_update_;

 public:
  ProceduralManager();
  ~ProceduralManager();

  void update(Scene *scene, Progress &progress);

  void tag_update();

  bool need_update() const;
};

}
