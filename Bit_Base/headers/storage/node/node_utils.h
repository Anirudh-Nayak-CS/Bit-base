#pragma once
#include "internal_node.h"
#include "leaf_node.h"

inline uint32_t get_node_max_key(Pager *pager, void *node) {
  if (get_node_type(node) == NodeType::LEAF) {
    return *leafNodeKey(node, *leafNodeNumCells(node) - 1);
  }
  void *right_child = pager->get_page(*internal_node_right_child(node));

  return get_node_max_key(pager, right_child);
}