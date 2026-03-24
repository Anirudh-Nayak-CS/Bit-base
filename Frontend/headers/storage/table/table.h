#pragma once
#include "../Pager/pager.h"
#include <string>
#include <unordered_map>
#include <vector>



class Table {
public:
   uint32_t root_page_num;
   Pager* pager;
  };


