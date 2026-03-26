#pragma once
#include "../Pager/pager.h"
#include "../node/leaf_node.h"
#include "../row/row.h"

class B_Plus_Tree {
public:
  static void leaf_node_insert(Cursor *cursor, uint32_t key, const Row *value);
  static void leaf_node_split_and_insert(Cursor *cursor, uint32_t key,
                                        const Row *value);
  static void create_new_root(Table *table, uint32_t right_child_page_num);
  static void print_tree(Pager *pager, uint32_t page_num,
                         uint32_t indentation_level);
  static void update_internal_node_key(void *node, uint32_t old_key,
                                       uint32_t new_key);
  static void internal_node_insert(Table *table, uint32_t parent_page_num,
                                   uint32_t child_page_num);
  static void internal_node_split_and_insert(Table *table,
                                             uint32_t parent_page_num,
                                             uint32_t child_page_num);
  static Cursor *internal_node_find(Table *table, uint32_t page_num,
                                    uint32_t key);
  static uint32_t internal_node_find_child(void *node, uint32_t key);
  static Cursor *leaf_node_find(Table *table, uint32_t page_num, uint32_t key);
};