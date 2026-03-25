#pragma once 
#include "../storage/table/table.h"


class Cursor {
    public:
    Table* table;
    bool end_of_table;
    uint32_t page_num;
    uint32_t cell_num;

    Cursor* table_start(Table* table);
    Cursor* table_find(Table* table,uint32_t key);
    void* cursor_value();
    void cursor_advance();


};