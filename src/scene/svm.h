/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <atomic>

#include "scene/shader.h"
#include "scene/shader_graph.h"

#include "util/array.h"
#include "util/string.h"

namespace ccl {

class Device;
class DeviceScene;
class ImageManager;
class Scene;
class ShaderGraph;
class ShaderInput;
class ShaderNode;
class ShaderOutput;

/* Shader Manager */

class SVMShaderManager : public ShaderManager {
 public:
  SVMShaderManager();
  ~SVMShaderManager() override;

  void device_update_specific(Device *device,
                              DeviceScene *dscene,
                              Scene *scene,
                              Progress &progress) override;
  void device_free(Device *device, DeviceScene *dscene, Scene *scene) override;

 protected:
  void device_update_shader(Scene *scene,
                            Shader *shader,
                            Progress &progress,
                            array<int4> *svm_nodes);
};

/* Graph Compiler */

class SVMCompiler {
 public:
  struct Summary {
    Summary();

    /* Number of SVM nodes shader was compiled into. */
    int num_svm_nodes;

    /* Peak stack usage during shader evaluation. */
    int peak_stack_usage;

    /* Time spent on surface graph finalization. */
    double time_finalize;

    /* Time spent on generating SVM nodes for surface shader. */
    double time_generate_surface;

    /* Time spent on generating SVM nodes for bump shader. */
    double time_generate_bump;

    /* Time spent on generating SVM nodes for volume shader. */
    double time_generate_volume;

    /* Time spent on generating SVM nodes for displacement shader. */
    double time_generate_displacement;

    /* Total time spent on all routines. */
    double time_total;

    /* A full multi-line description of the state of the compiler after compilation. */
    string full_report() const;
  };

  SVMCompiler(Scene *scene);
  void compile(Shader *shader,
               array<int4> &svm_nodes,
               const int index,
               Summary *summary = nullptr);

  int stack_assign(ShaderOutput *output);
  int stack_assign(ShaderInput *input);
  bool is_linked(ShaderInput *input);
  int stack_assign_if_linked(ShaderInput *input);
  int stack_assign_if_linked(ShaderOutput *output);
  int stack_find_offset(const int size);
  int stack_find_offset(SocketType::Type type);
  void stack_clear_offset(SocketType::Type type, const int offset);
  void stack_link(ShaderInput *input, ShaderOutput *output);

  void add_node(ShaderNodeType type, const int a = 0, const int b = 0, const int c = 0);
  void add_node(const int a = 0, const int b = 0, const int c = 0, const int d = 0);
  void add_node(ShaderNodeType type, const float3 &f);
  void add_node(const float4 &f);
  uint attribute(string name);
  uint attribute(AttributeStandard std);
  uint attribute_standard(string name);
  uint encode_uchar4(const uint x, const uint y = 0, const uint z = 0, const uint w = 0);
  uint closure_mix_weight_offset()
  {
    return mix_weight_offset;
  }
  uint get_bump_state_offset()
  {
    return bump_state_offset;
  }

  ShaderType output_type()
  {
    return current_type;
  }

  Scene *scene;
  ShaderGraph *current_graph;
  bool background;

 protected:
  /* stack */
  struct Stack {
    Stack()
    {
      memset(users, 0, sizeof(users));
    }
    Stack(const Stack &other)
    {
      memcpy(users, other.users, sizeof(users));
    }
    Stack &operator=(const Stack &other)
    {
      memcpy(users, other.users, sizeof(users));
      return *this;
    }

    bool empty()
    {
      for (int i = 0; i < SVM_STACK_SIZE; i++) {
        if (users[i]) {
          return false;
        }
      }

      return true;
    }

    void print()
    {
      printf("stack <");

      for (int i = 0; i < SVM_STACK_SIZE; i++) {
        printf((users[i]) ? "*" : " ");
      }

      printf(">\n");
    }

    int users[SVM_STACK_SIZE];
  };

  /* Global state of the compiler accessible from the compilation routines. */
  struct CompilerState {
    explicit CompilerState(ShaderGraph *graph);

    /* ** Global state, used by various compilation steps. ** */

    /* Set of nodes which were already compiled. */
    ShaderNodeSet nodes_done;

    /* Set of closures which were already compiled. */
    ShaderNodeSet closure_done;

    /* Set of nodes used for writing AOVs. */
    ShaderNodeSet aov_nodes;

    /* ** SVM nodes generation state ** */

    /* Flag whether the node with corresponding ID was already compiled or
     * not. Array element with index i corresponds to a node with such if.
     *
     * TODO(sergey): This is actually a copy of nodes_done just in another
     * notation. We can de-duplicate this storage actually after switching
     * all areas to use this flags array.
     */
    vector<bool> nodes_done_flag;

    /* Node features that can be compiled. */
    uint node_feature_mask;
  };

  void stack_clear_temporary(ShaderNode *node);
  int stack_size(SocketType::Type type);
  void stack_clear_users(ShaderNode *node, ShaderNodeSet &done);

  /* single closure */
  void find_dependencies(ShaderNodeSet &dependencies,
                         const ShaderNodeSet &done,
                         ShaderInput *input,
                         ShaderNode *skip_node = nullptr);
  void find_aov_nodes_and_dependencies(ShaderNodeSet &aov_nodes,
                                       ShaderGraph *graph,
                                       CompilerState *state);
  void generate_node(ShaderNode *node, ShaderNodeSet &done);
  void generate_aov_node(ShaderNode *node, CompilerState *state);
  void generate_closure_node(ShaderNode *node, CompilerState *state);
  void generated_shared_closure_nodes(ShaderNode *root_node,
                                      ShaderNode *node,
                                      CompilerState *state,
                                      const ShaderNodeSet &shared);
  void generate_svm_nodes(const ShaderNodeSet &nodes, CompilerState *state);

  /* multi closure */
  void generate_multi_closure(ShaderNode *root_node, ShaderNode *node, CompilerState *state);

  /* compile */
  void compile_type(Shader *shader, ShaderGraph *graph, ShaderType type);

  std::atomic_int *svm_node_types_used;
  array<int4> current_svm_nodes;
  ShaderType current_type;
  Shader *current_shader;
  Stack active_stack;
  int max_stack_use;
  uint mix_weight_offset;
  uint bump_state_offset;
  bool compile_failed;
};

}
