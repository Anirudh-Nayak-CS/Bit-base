#pragma once

#include <cstdint>
#include <cstdio>

constexpr uint32_t PAGE_SIZE = 4096;
constexpr uint32_t TABLE_MAX_PAGES=100;

class Pager {
    public:
    int file_descriptor;
    uint32_t file_length;
    void* pages[TABLE_MAX_PAGES];
    uint32_t num_pages;

    void* get_page(uint32_t page_num);
    void flush(uint32_t page_num);
   
    static Pager* pager_open(const char* filename);
    uint32_t get_unused_page_num(); 
};