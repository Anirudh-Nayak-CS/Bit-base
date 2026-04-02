#pragma once

#include <cstdint>
#include <cstdio>
#include "unordered_set"

constexpr uint32_t PAGE_SIZE = 4096;
constexpr uint32_t TABLE_MAX_PAGES=10000;

class Pager {
    public:
    int file_descriptor;
    uint32_t file_length;
    void* pages[TABLE_MAX_PAGES];
    uint32_t num_pages=0;
 
    // Dirty-page tracking for WAL commit/rollback
    std::unordered_set<uint32_t> dirty_pages_;

    void mark_dirty(uint32_t page_num);
    bool is_dirty(uint32_t page_num) const;
    void* get_page(uint32_t page_num);
    void flush_page(uint32_t page_num);
    void flush_all_dirty();
   
   
    static Pager* pager_open(const char* filename);
    uint32_t get_unused_page_num(); 
    void pager_close();
    ~Pager();  // Destructor to free malloc'd memory


};