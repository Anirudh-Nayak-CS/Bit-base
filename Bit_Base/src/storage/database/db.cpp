#include "../../../headers/storage/database/db.h"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <memory>

db *db::db_open(const char *filename) {
  
  mkdir(filename, 0755);
  db *database = new db(filename);
  database->loadSchema();
  return database;
}

void db::db_close() {
  saveSchema();
  tables.clear();
}

bool db::createTable(const std::string &name, const Schema& schema) {
  if (tables.count(name))
    return false;
  std::string filename = this->name + "/" + name + ".tbl";
  tables[name] = std::make_unique<Table>(name, schema, filename);

  return true;
}

bool db::deleteTable(const std::string &name) {
  if (!tables.count(name))
    return false;

  tables.erase(name);

  return true;
}

Table *db::getTable(const std::string &name) {
  if (!tables.count(name))
    return nullptr;
  return tables[name].get();
}

void db::saveSchema() {
  std::ofstream out(name + ".schema");

  for (auto &[tableName, table] : tables) {
    out << tableName;

    for (const auto &col : table->schema.columns) {
      out << " " << col.name << ":" << static_cast<int>(col.type);
      if (col.is_primary_key) out << ":PK";
    }

    out << "\n";
  }
}

void db::loadSchema() {
  std::ifstream in(name + ".schema");
  if (!in.is_open())
    return;

  std::string line;

  while (std::getline(in, line)) {
    std::stringstream ss(line);

    std::string tableName;
    ss >> tableName;

    Schema schema;
    std::string colspec;

    while (ss >> colspec) {
      // Parse colspec: "name:type" or "name:type:PK"
      size_t pos1 = colspec.find(':');
      if (pos1 == std::string::npos) continue;
      
      std::string col_name = colspec.substr(0, pos1);
      std::string type_str = colspec.substr(pos1 + 1);
      
      bool is_pk = false;
      size_t pos2 = type_str.find(':');
      if (pos2 != std::string::npos) {
        if (type_str.substr(pos2 + 1) == "PK") is_pk = true;
        type_str = type_str.substr(0, pos2);
      }
      
      DataType dtype = static_cast<DataType>(std::stoi(type_str));
      schema.columns.push_back({col_name, dtype, is_pk});
    }

    std::string filename = name + "/" + tableName + ".tbl";

    tables[tableName] =
        std::make_unique<Table>(tableName, schema, filename);
  }
}