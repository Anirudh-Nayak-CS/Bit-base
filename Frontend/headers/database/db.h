#pragma once
#include "../storage/table/table.h"
#include <string>
#include <unordered_map>
#include <vector>

class dbClass {
public:
  std::string name;
  std::unordered_map<std::string, Table *> tables;

  dbClass(const std::string &name) : name(name) {}
  bool createTable(const std::string &name,
                   const std::vector<std::string> &attributes);
  bool deleteTable(const std::string &name);
  Table *selectTable(const std::string &name);

  ~dbClass() {
    for (auto &pair : tables) {
      delete pair.second;
    }
    tables.clear();
  }
};