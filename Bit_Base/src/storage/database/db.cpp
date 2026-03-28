#include "../../../headers/storage/database/db.h"
#include <sys/stat.h>
#include <fstream>
#include <sstream>

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

bool db::createTable(const std::string &name,
                     const std::vector<std::string> &attributes) {
  if (tables.count(name))
    return false;
  std::string filename = this->name + "/" + name + ".tbl";
  tables[name] = std::make_unique<Table>(name, attributes, filename);

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

    for (const auto &attr : table->attributes) {
      out << " " << attr;
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

    std::vector<std::string> attributes;
    std::string attr;

    while (ss >> attr) {
      attributes.push_back(attr);
    }

    std::string filename = name + "/" + tableName + ".tbl";

    tables[tableName] =
        std::make_unique<Table>(tableName, attributes, filename);
  }
}