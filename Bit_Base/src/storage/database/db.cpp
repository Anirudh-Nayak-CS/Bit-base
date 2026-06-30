#include "../../../headers/storage/database/db.h"
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <filesystem> 
#include <sstream>
#include <memory>

db *db::db_open(const char *filename) {
  
  mkdir(filename, 0755);
  db *database = new db(filename);
  database->wal_ = std::make_unique<WalManager>(
        std::string(filename) + ".wal");
  database->loadSchema();

  for (auto& [name, table] : database->tables)
      database->wal_->recover(table->pager);

  return database;
}

void db::db_close() {
     
    if (current_txn_) {
        std::cerr << "[db] open transaction rolled back on close\n";
    
        for (const auto& [page_num, data] : current_txn_->snapshots()) {
    for (auto& [tname, table] : tables) {
        if (page_num < table->pager->num_pages) {
            void* page = table->pager->get_page(page_num);
            std::memcpy(page, data.data(),
                        std::min(data.size(), (size_t)PAGE_SIZE));
            table->pager->mark_dirty(page_num);
        }
    }
}
        wal_->log_rollback(current_txn_->id());
        current_txn_.reset();
    }
  saveSchema();
  tables.clear();
}


//  Transaction management 
 
Transaction* db::begin_txn() {
 
    if (current_txn_) {
        std::cerr << "[db] warning: nested BEGIN  ignoring\n";
    
        return current_txn_.get();
    }
    current_txn_ = std::make_unique<Transaction>(next_txn_id_++);
    wal_->log_begin(current_txn_->id());
    return current_txn_.get();
}
 
// pin_page: called by vm.cpp BEFORE a page is mutated.

void db::pin_page(uint32_t page_num, const void* page_data, Pager*) {
    if (!current_txn_) return;
    if (current_txn_->has_snapshot(page_num)) return;
 
  
    current_txn_->pin_page(page_num, page_data, PAGE_SIZE);
 
}
 
bool db::commit_txn(uint64_t txn_id) {
    if (!current_txn_ || current_txn_->id() != txn_id) {
        std::cerr << "[db] commit_txn: no matching open transaction\n";
        current_txn_.reset();  
        return false;
    }
 

    for (const auto& [page_num, before_data] : current_txn_->snapshots()) {
        bool logged = false;
        for (auto& [tname, table] : tables) {
            if (page_num < table->pager->num_pages) {
                void* page = table->pager->get_page(page_num);
                wal_->log_write(txn_id, page_num, before_data.data(), page);
                logged = true;
                break;
            }
        }
        if (!logged) {
            std::cerr << "[db] warning: could not find pager for page " << page_num << "\n";
        }
    }

    wal_->log_commit(txn_id);
 

    for (auto& [tname, table] : tables)
        table->pager->flush_all_dirty();

    wal_->truncate();
 
    current_txn_->mark_committed();
    current_txn_.reset();
    return true;
}
 
bool db::rollback_txn(uint64_t txn_id) {
    if (!current_txn_ || current_txn_->id() != txn_id) {
        std::cerr << "[db] rollback_txn: no matching open transaction\n";
        return false;
    }

  for (const auto& [page_num, data] : current_txn_->snapshots()) {
    for (auto& [tname, table] : tables) {
        if (page_num < table->pager->num_pages) {
            void* page = table->pager->get_page(page_num);
            std::memcpy(page, data.data(),
                        std::min(data.size(), (size_t)PAGE_SIZE));
            table->pager->mark_dirty(page_num);
        }
    }
}

    for (auto& [tname, table] : tables)
        table->pager->flush_all_dirty();

    wal_->log_rollback(txn_id);
    wal_->truncate();
 
    current_txn_->mark_rolled_back();
    current_txn_.reset();
    return true;
}
 

bool db::createTable(const std::string &name, const Schema& schema) {
  if (tables.count(name))
    return false;
  std::string filename = this->name + "/" + name + ".tbl";
  tables[name] = std::make_unique<Table>(name, schema, filename);

  return true;
}

bool db::deleteTable(const std::string &name) {
   auto it = tables.find(name);
    if (it == tables.end())
        return false;

   
    std::string filepath = this->name + "/" + name + ".tbl";

    tables.erase(it);   

   
    std::filesystem::remove(filepath);

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
