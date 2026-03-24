#include "../../headers/cursor/cursor.h"
#include "../../headers/storage/Pager/pager.h"
#include "../../headers/storage/table/table.h"


Cursor* Cursor::table_start(Table* table) {
    Cursor* cursor = new Cursor();
    cursor->table = table;
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;

    void* root_node = table->pager->get_page(table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (num_cells == 0);

    return cursor;
}

Cursor* Cursor::table_end(Table* table) {
    Cursor* cursor = new Cursor();
    cursor->table = table;
    cursor->page_num = table->root_page_num;

    void* root_node = table->pager->get_page(table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->cell_num = num_cells;
    cursor->end_of_table = true;

    return cursor;
}

void* Cursor::cursor_value() {
     uint32_t page_num = this->page_num;

    void* page = table->pager->get_page(page_num);

    return leaf_node_value(page, this->cell_num);
}

void Cursor::cursor_advance(){
    uint32_t page_num = this->page_num;

    void* node= table->pager->get_page(page_num);


    this->cell_num += 1;
    if (this->cell_num >= (*leaf_node_num_cells(node))) {
      this->end_of_table = true;
    }
}
