#pragma once
#include "../table/table.h"
#include "../Pager/pager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
 
class db {
public:
    std::string name;
    std::unordered_map<std::string, std::unique_ptr<Table>> tables;
 
    db(const std::string& name) : name(name) {}
 
    static db* db_open(const char* filename);
    void db_close();
 
    // schema (tables) 
    bool   createTable(const std::string& name, const Schema& schema);
    bool   deleteTable(const std::string& name);
    Table* getTable   (const std::string& name);
 
    // persistence — saves/loads column names
    void saveSchema();
    void loadSchema();
};
