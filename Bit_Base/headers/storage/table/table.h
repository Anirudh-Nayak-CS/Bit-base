#pragma once
#include "../Pager/pager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "../../cursor/cursor.h"
#include "../../schema/schema.h"


class Table {
public:
    std::string  name;
    Schema       schema;      
    uint32_t     root_page_num;
    Pager*       pager;
 
    Table(const std::string& name,
          const Schema&      schema,
          const std::string& filename);
 
    
    // insert serializes the Row internally before calling leaf_node_insert
    bool insert(uint32_t key, const void* data, uint32_t size);
    std::unique_ptr<Cursor> find(uint32_t key);
 
    ~Table();
};
 

