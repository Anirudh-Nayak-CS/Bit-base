// headers/storage/transaction/transaction.h
#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <memory>

struct PageSnapshot {
    uint32_t             page_num;
    std::vector<uint8_t> data;   
};

class Transaction {
public:
    explicit Transaction(uint64_t id) : id_(id) {}

    uint64_t id() const { return id_; }

    // Save a before-image the first time a page is dirtied in this transaction. bcs we only need original state for rollback, fills snapshots
    void pin_page(uint32_t page_num, const void* page_data, size_t page_size);

    // True if we already snapshotted this page (stored original state of page).
    bool has_snapshot(uint32_t page_num) const;
    
    //to store initial state of all pages that got modified
    const std::unordered_map<uint32_t,  std::vector<uint8_t>>& snapshots() const { return snapshots_; }
   
    // tracking what happened during transaction
    void mark_committed()  { committed_  = true; }
    void mark_rolled_back(){ rolled_back_= true; }
    bool committed()   const { return committed_;   }
    bool rolled_back() const { return rolled_back_; }

private:
    uint64_t id_;
    std::unordered_map<uint32_t,  std::vector<uint8_t>> snapshots_;
    bool committed_   = false;
    bool rolled_back_ = false;
};