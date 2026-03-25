#pragma once
#include <cstdint>
#include "../Pager/pager.h"


//Node Types
 
enum class NodeType : uint8_t {
    INTERNAL=0,
    LEAF=1,
};

// Common Node Header Layout
 
constexpr uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
constexpr uint32_t NODE_TYPE_OFFSET = 0;

constexpr uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
constexpr uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;

constexpr uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
constexpr uint32_t PARENT_POINTER_OFFSET =
    IS_ROOT_OFFSET + IS_ROOT_SIZE;

constexpr uint32_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE +
    IS_ROOT_SIZE +
    PARENT_POINTER_SIZE;

inline NodeType get_node_type(void* node) {
  uint8_t value = *(reinterpret_cast<uint8_t*>(
            reinterpret_cast<char*>(node) + NODE_TYPE_OFFSET
        ));
  return (NodeType)value;
}

inline void set_node_type(void* node, NodeType type) {
   uint8_t value = static_cast<uint8_t>(type);
   *(reinterpret_cast<uint8_t*>(
            reinterpret_cast<char*>(node) + NODE_TYPE_OFFSET
        )) = value;
}

inline bool is_node_root(void* node) {
    uint8_t value = *(reinterpret_cast<uint8_t*>(
        reinterpret_cast<char*>(node) + IS_ROOT_OFFSET
    ));
    return static_cast<bool>(value);
}

inline void set_node_root(void* node, bool is_root) {
    uint8_t value = static_cast<uint8_t>(is_root);
    *(reinterpret_cast<uint8_t*>(
        reinterpret_cast<char*>(node) + IS_ROOT_OFFSET
    )) = value;
}