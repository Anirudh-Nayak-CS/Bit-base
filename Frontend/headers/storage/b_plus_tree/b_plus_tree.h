#pragma once
#include "../node/leaf_node.h"
#include "../Pager/pager.h"
#include "../row/row.h"

class B_Plus_Tree {
 public :

 void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value);
 static Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key);
 void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value);
 void create_new_root(Table* table, uint32_t right_child_page_num);
 void  print_tree(Pager* pager,uint32_t page_num,uint32_t indentation_level);
 static Cursor* internal_node_find(Table* table,uint32_t page_num,uint32_t key);

};