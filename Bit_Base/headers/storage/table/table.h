#pragma once
#include "../Pager/pager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "../row/row.h"
#include "../../cursor/cursor.h"

class Table {
public:
  std::string name;
  std::vector<std::string> attributes;
  uint32_t root_page_num;
  Pager* pager;


   Table(const std::string& name,
      const std::vector<std::string>& attrs,
      const std::string& filename);
  

  // core B+ tree operations
  void insert(uint32_t key, const Row& value);
  std::unique_ptr<Cursor> find(uint32_t key);

   ~Table();
  };


