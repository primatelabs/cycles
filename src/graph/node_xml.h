/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/map.h"
#include "util/param.h"
#include "util/xml.h"

CCL_NAMESPACE_BEGIN

struct Node;

struct XMLReader {
  map<string, Node *> node_map;
};

void xml_read_node(XMLReader &reader, Node *node, const xml_node xml_node);
xml_node xml_write_node(Node *node, const xml_node xml_root);

CCL_NAMESPACE_END
