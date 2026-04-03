#pragma once
#include <memory>

class Pager;
class Cursor;
class Table;
struct Row;

class B_Plus_Tree {
public:
  
 static void leaf_node_insert(Cursor* cursor, uint32_t key,
                                 const void* data, uint32_t size);
 
 static void leaf_node_split_and_insert(Cursor* cursor, uint32_t key,
                                           const void* data, uint32_t size);
  static void create_new_root(Table* table, uint32_t right_child_page_num);

  static void print_tree(Pager* pager, uint32_t page_num,
                         uint32_t indentation_level);

  static void update_internal_node_key(void* node, uint32_t old_key,
                                       uint32_t new_key);

  static bool leaf_node_delete_cell(void* node, uint32_t cell_num);                                     

  static void internal_node_insert(Table* table, uint32_t parent_page_num,
                                   uint32_t child_page_num);

  static void internal_node_split_and_insert(Table* table,
                                             uint32_t parent_page_num,
                                             uint32_t child_page_num);

  static std::unique_ptr<Cursor> internal_node_find(Table* table,
                                                    uint32_t page_num,
                                                    uint32_t key);

  static uint32_t internal_node_find_child(void* node, uint32_t key);

  static std::unique_ptr<Cursor> leaf_node_find(Table* table,
                                                uint32_t page_num,
                                                uint32_t key);
};