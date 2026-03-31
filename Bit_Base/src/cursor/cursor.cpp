#include "../../headers/cursor/cursor.h"
#include "../../headers/storage/b_plus_tree/b_plus_tree.h"
#include "../../headers/storage/node/leaf_node.h"
#include "../../headers/storage/table/table.h"


std::unique_ptr<Cursor> Cursor::table_start(Table *table) {
  auto cursor = table_find(table, 0);
  void *node = table->pager->get_page(cursor->page_num);
  uint32_t num_cells = *leafNodeNumCells(node);
  cursor->end_of_table = (num_cells == 0);

  return cursor;
}

std::unique_ptr<Cursor> Cursor::table_find(Table *table, uint32_t key) {
  uint32_t root_page_num = table->root_page_num;
  void *root_node = table->pager->get_page(root_page_num);

  if (get_node_type(root_node) == NodeType::LEAF) {
    return B_Plus_Tree::leaf_node_find(table, root_page_num, key);
  } else {
    return B_Plus_Tree::internal_node_find(table, root_page_num, key);
  }
}

void *Cursor::cursor_value() {
  uint32_t page_num = this->page_num;

  void *page = table->pager->get_page(page_num);

  return leafNodeValue(page, this->cell_num);
}

void Cursor::cursor_advance() {
  uint32_t page_num = this->page_num;

  void *node = table->pager->get_page(page_num);

  this->cell_num++;
  if (this->cell_num >= (*leafNodeNumCells(node))) {
    uint32_t next_page_num = *leaf_node_next_leaf(node);
    if(next_page_num == INVALID_PAGE_NUM) {
        this->end_of_table=true;
    }
    else {
       this->cell_num=0;
       this->page_num=next_page_num;
    }
  }
}
