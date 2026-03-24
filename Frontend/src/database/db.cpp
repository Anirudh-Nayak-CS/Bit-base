#include "../../headers/database/db.h"

bool dbClass::createTable(const std::string &name,
                          const std::vector<std::string> &attributes) {

  // 1. Check if table already exists
  if (tables.find(name) != tables.end()) {
    return false;
  }

  // 2. Create the empty table
  Table *new_table = new Table(name);

  // 3. Let the Table class wire up its own columns!
  for (const auto &attribute : attributes) {
    new_table->add_column(attribute);
  }

  // 4. Save to database
  tables[name] = new_table;

  return true;
}

bool dbClass::deleteTable(const std::string &name) {

  if (tables.find(name) == tables.end())
    return false;

  delete tables[name];
  tables.erase(name);
  return true;
}

Table *dbClass::selectTable(const std::string &name) {
  if (tables.find(name) == tables.end())
    return nullptr;

  Table *selectedTable = tables[name];
  return selectedTable;
}