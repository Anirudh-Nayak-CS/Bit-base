#pragma once
#include "../../cursor/cursor.h"
#include "../row/row.h"
#include "node.h"
#include <iostream>


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

constexpr uint32_t LEAF_NODE_VALUE_SIZE =sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_VALUE_OFFSET =
    LEAF_NODE_KEY_OFFSET  + LEAF_NODE_KEY_SIZE;

constexpr uint32_t LEAF_NODE_CELL_SIZE =
    LEAF_NODE_KEY_SIZE  + LEAF_NODE_VALUE_SIZE;

constexpr uint32_t LEAF_NODE_SPACE_FOR_CELLS =
    PAGE_SIZE - LEAF_NODE_HEADER_SIZE;

constexpr uint32_t LEAF_NODE_MAX_CELLS =
    LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

//Accessors

inline uint32_t* leafNodeNumCells(void* node) {
    return reinterpret_cast<uint32_t*>(
        reinterpret_cast<char*>(node)  + LEAF_NODE_NUM_CELLS_OFFSET
    );
}

inline void* leafNodeCell(void* node, uint32_t cellNum) {
    return reinterpret_cast<void*>(
        reinterpret_cast<char*>(node)  +
        LEAF_NODE_HEADER_SIZE  +
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
        reinterpret_cast<char*>(leafNodeCell(node, cellNum))  +
        LEAF_NODE_KEY_SIZE
    );
}

// Initialization 

inline void initializeLeafNode(void* node) {
     set_node_type(node, NodeType::LEAF);
     set_node_root(node, false);
    *leafNodeNumCells(node) = 0;
}


// node Split limits 
const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;
