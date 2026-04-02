#pragma once
#include "../table/table.h"
#include "../Pager/pager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "../wal/wal.h"
#include "../transaction/transaction.h"

class db {


private:
    std::unique_ptr<WalManager>  wal_;
    std::unique_ptr<Transaction> current_txn_;
    uint64_t next_txn_id_ = 1;

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

    Transaction* begin_txn();
    void  pin_page(uint32_t page_num, const void* page_data, Pager* pager);
    bool         commit_txn(uint64_t txn_id);
    bool         rollback_txn(uint64_t txn_id);

    Transaction* current_txn() { return current_txn_.get(); }
};
