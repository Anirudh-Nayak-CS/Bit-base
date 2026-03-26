#pragma once 

#include <cstdint>

class Table; 

class Cursor {
    public:
    Table* table;
    bool end_of_table;
    uint32_t page_num;
    uint32_t cell_num;

    Cursor() : table(nullptr), end_of_table(false), page_num(0), cell_num(0) {}

    static Cursor* table_start(Table* table);
    static Cursor* table_find(Table* table,uint32_t key);
    void* cursor_value();
    void cursor_advance();


};