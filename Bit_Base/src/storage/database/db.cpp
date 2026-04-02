#include "../../../headers/storage/database/db.h"
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <memory>

db *db::db_open(const char *filename) {
  
  mkdir(filename, 0755);
  db *database = new db(filename);
  database->wal_ = std::make_unique<WalManager>(
        std::string(filename) + ".wal");
  database->loadSchema();
    // Replay any committed-but-not-flushed txns from a prior crash
  for (auto& [name, table] : database->tables) {
        Pager* pager = table->pager;
        database->wal_->recover(pager);
    }
  return database;
}

void db::db_close() {
       // Roll back any open transaction that was never committed
    if (current_txn_) {
        std::cerr << "[db] open transaction rolled back on close\n";
        // Restore all before-images
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
    // Only one active txn at a time (single-writer model)
    if (current_txn_) {
        std::cerr << "[db] warning: nested BEGIN  ignoring\n";
    
        return current_txn_.get();
    }
    current_txn_ = std::make_unique<Transaction>(next_txn_id_++);
    wal_->log_begin(current_txn_->id());
    return current_txn_.get();
}
 
// pin_page: called by vm.cpp BEFORE a page is mutated.
// Saves the before-image to both the in-memory transaction and the WAL.
void db::pin_page(uint32_t page_num, const void* page_data, Pager* pager) {
    if (!current_txn_) return;
    if (current_txn_->has_snapshot(page_num)) return;
 
    // Save in-memory snapshot (for fast rollback without re-reading the WAL)
    current_txn_->pin_page(page_num, page_data, PAGE_SIZE);
 
    // Log the before-image to disk (WAL guarantee)
    // after-image pointer is unused in our UNDO-only scheme
    wal_->log_write(current_txn_->id(), page_num, page_data, nullptr);
}
 
bool db::commit_txn(uint64_t txn_id) {
    if (!current_txn_ || current_txn_->id() != txn_id) {
        std::cerr << "[db] commit_txn: no matching open transaction\n";
        current_txn_.reset();  
        return false;
    }
 
    // Write COMMIT record and fsync the WAL *before* touching data pages.
  
    wal_->log_commit(txn_id);   // log_commit calls flush() internally
 
    // Flush every dirty page to the .tbl file now that the commit is durable
    for (auto& [tname, table] : tables)
        table->pager->flush_all_dirty();
 
    //  Truncate the WAL – the data file is now consistent
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
 
    // Restore every page to its before-image (in-memory snapshots).
    // We iterate all tables and check which page numbers belong to each pager.
    // Because BitBase is single-table-per-file, page numbers are per-pager,
    // so we match by checking num_pages bounds.
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
 
    // Flush restored pages so the .tbl file is consistent too
    for (auto& [tname, table] : tables)
        table->pager->flush_all_dirty();
 
    // Log ROLLBACK and truncate WAL
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