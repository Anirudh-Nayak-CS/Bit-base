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
     set_node_type(node, NodeType::LEAF);
    *leafNodeNumCells(node) = 0;
}

inline void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value)  {
    void* node = cursor->table->pager->get_page(cursor->page_num);

    uint32_t num_cells = *leaf_node_num_cells(node);

    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        std::cerr << "Need to implement splitting a leaf node.\n";
        std::exit(EXIT_FAILURE);
    }

    if (cursor->cell_num < num_cells) {
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            std::memcpy(
                leaf_node_cell(node, i),
                leaf_node_cell(node, i - 1),
                LEAF_NODE_CELL_SIZE
            );
        }
    }

    (*leaf_node_num_cells(node))++;

    *leaf_node_key(node, cursor->cell_num) = key;

    value->serialize(leaf_node_value(node, cursor->cell_num));
}


inline Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
   void* node = table->pager->get_page(page_num);
   uint32_t num_cells = *leaf_node_num_cells(node);
 
   Cursor* cursor = new Cursor();
   cursor->table = table;
   cursor->page_num = page_num;

   // Binary search to find key 
   // with this we get 3 cases -> key already there, position where to insert or at end
   uint32_t min_index = 0;
   uint32_t one_past_max_index = num_cells;
   while (one_past_max_index != min_index) {
     uint32_t index = (min_index + one_past_max_index) / 2;
     uint32_t key_at_index = *leaf_node_key(node, index);
     if (key == key_at_index) {
       cursor->cell_num = index;
      return cursor;
     }
     if (key < key_at_index) {
      one_past_max_index = index;
    } else {
      min_index = index + 1;
    }
  }

  cursor->cell_num = min_index;
  return cursor;
}
