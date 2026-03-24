#pragma once
#include <cstdint>


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

// Leaf Node Header Layout
 
constexpr uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET =
    COMMON_NODE_HEADER_SIZE;

constexpr uint32_t LEAF_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE +
    LEAF_NODE_NUM_CELLS_SIZE;

//Leaf Node Body Layout


constexpr uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_KEY_OFFSET = 0;

constexpr uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
constexpr uint32_t LEAF_NODE_VALUE_OFFSET =
    LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;

constexpr uint32_t LEAF_NODE_CELL_SIZE =
    LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;

constexpr uint32_t LEAF_NODE_SPACE_FOR_CELLS =
    PAGE_SIZE - LEAF_NODE_HEADER_SIZE;

constexpr uint32_t LEAF_NODE_MAX_CELLS =
    LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

//Accessors

inline uint32_t* leafNodeNumCells(void* node) {
    return reinterpret_cast<uint32_t*>(
        reinterpret_cast<char*>(node) + LEAF_NODE_NUM_CELLS_OFFSET
    );
}

inline void* leafNodeCell(void* node, uint32_t cellNum) {
    return reinterpret_cast<void*>(
        reinterpret_cast<char*>(node) +
        LEAF_NODE_HEADER_SIZE +
        cellNum * LEAF_NODE_CELL_SIZE
    );
}

inline uint32_t* leafNodeKey(void* node, uint32_t cellNum) {
    return reinterpret_cast<uint32_t*>(
        leafNodeCell(node, cellNum)
    );
}

inline void* leafNodeValue(void* node, uint32_t cellNum) {
    return reinterpret_cast<void*>(
        reinterpret_cast<char*>(leafNodeCell(node, cellNum)) +
        LEAF_NODE_KEY_SIZE
    );
}

// Initialization 

inline void initializeLeafNode(void* node) {
    *leafNodeNumCells(node) = 0;
}
