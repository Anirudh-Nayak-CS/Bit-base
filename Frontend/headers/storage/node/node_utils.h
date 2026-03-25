#pragma once 
#include "internal_node.h"
#include "leaf_node.h"

inline uint32_t get_node_max_key(void *node) {
  switch (get_node_type(node)) {
  case NodeType::INTERNAL:
    return *internal_node_key(node, *internal_node_num_keys(node) - 1);
  case NodeType::LEAF:
    return *leafNodeKey(node, *leafNodeNumCells(node) - 1);
  }
}