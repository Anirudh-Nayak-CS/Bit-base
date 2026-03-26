#pragma once

#include "node.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>

#define INVALID_PAGE_NUM UINT32_MAX

/* Internal Node Header Layout*/

const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
    INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                           INTERNAL_NODE_NUM_KEYS_SIZE +
                                           INTERNAL_NODE_RIGHT_CHILD_SIZE;

/* Internal Node Body Layout */

const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_MAX_CELLS = 3;
const uint32_t INTERNAL_NODE_CELL_SIZE =
    INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;

// functions

inline uint32_t *internal_node_num_keys(void *node) {
  return reinterpret_cast<uint32_t *>(reinterpret_cast<char *>(node) +
                                      INTERNAL_NODE_NUM_KEYS_OFFSET);
}

inline uint32_t *internal_node_right_child(void *node) {
  return reinterpret_cast<uint32_t *>(reinterpret_cast<char *>(node) +
                                      INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

inline uint32_t *internal_node_cell(void *node, uint32_t cell_num) {
  return reinterpret_cast<uint32_t *>(reinterpret_cast<char *>(node) +
                                      INTERNAL_NODE_HEADER_SIZE +
                                      cell_num * INTERNAL_NODE_CELL_SIZE);
}

inline uint32_t *internal_node_child(void *node, uint32_t child_num) {
  uint32_t num_keys = *internal_node_num_keys(node);

  if (child_num > num_keys) {
    std::cout << "Tried to access child_num " << child_num << " > num_keys "
              << num_keys << std::endl;
    exit(EXIT_FAILURE);
  } else if (child_num == num_keys) {
    uint32_t* right_child = internal_node_right_child(node);
    if (*right_child == INVALID_PAGE_NUM) {
     printf("Tried to access right child of node, but was invalid page\n");
      exit(EXIT_FAILURE);
    }
    return right_child;
  } else {
    uint32_t* child = internal_node_cell(node, child_num);
    if (*child == INVALID_PAGE_NUM) {
      printf("Tried to access child %d of node, but was invalid page\n", child_num);
      exit(EXIT_FAILURE);
    }
    return child;
  }
}

inline uint32_t *internal_node_key(void *node, uint32_t key_num) {
  return reinterpret_cast<uint32_t *>(
      reinterpret_cast<char *>(internal_node_cell(node, key_num)) +
      INTERNAL_NODE_CHILD_SIZE);
}


// Initialization

inline void initialize_internal_node(void* node) {
 set_node_type(node, NodeType::INTERNAL);
  set_node_root(node, false);
  *internal_node_num_keys(node) = 0;
  *internal_node_right_child(node) = INVALID_PAGE_NUM;
}