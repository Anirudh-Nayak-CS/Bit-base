#include "../../../headers/storage/wal/wal.h"
#include "../../../headers/storage/Pager/pager.h"   // adjust path to match your pager header
#include <cstring>
#include <iostream>
#include <stdexcept>

// helpers 

static void write_u8 (std::fstream& f, uint8_t  v){ f.write(reinterpret_cast<char*>(&v), 1); }
static void write_u32(std::fstream& f, uint32_t v){ f.write(reinterpret_cast<char*>(&v), 4); }
static void write_u64(std::fstream& f, uint64_t v){ f.write(reinterpret_cast<char*>(&v), 8); }

static uint8_t  read_u8 (std::fstream& f){ uint8_t  v; f.read(reinterpret_cast<char*>(&v),1); return v; }
static uint32_t read_u32(std::fstream& f){ uint32_t v; f.read(reinterpret_cast<char*>(&v),4); return v; }
static uint64_t read_u64(std::fstream& f){ uint64_t v; f.read(reinterpret_cast<char*>(&v),8); return v; }


WalManager::WalManager(const std::string& wal_path) : path_(wal_path) {
    // Open for reading AND writing; create if not present.
    file_.open(wal_path, std::ios::in | std::ios::out |
                          std::ios::binary | std::ios::app);
    if (!file_.is_open()) {
        // File doesn't exist yet – create it then reopen r/w
        std::ofstream create(wal_path, std::ios::binary);
        create.close();
        file_.open(wal_path, std::ios::in | std::ios::out | std::ios::binary);
    }
    if (!file_.is_open())
        throw std::runtime_error("WAL: cannot open " + wal_path);
}

WalManager::~WalManager() {
    if (file_.is_open()) file_.close();
}



void WalManager::write_record(const WalRecord& r) {
    file_.seekp(0, std::ios::end);

    write_u8 (file_, static_cast<uint8_t>(r.type));
    write_u64(file_, r.transaction_id);

    if (r.type == WalRecordType::WRITE) {
        write_u32(file_, r.page_num);
        write_u32(file_, static_cast<uint32_t>(r.before_image.size()));
        file_.write(reinterpret_cast<const char*>(r.before_image.data()),
                    static_cast<std::streamsize>(r.before_image.size()));
    }
}

// ── public logging API ────────────────────────────────────────────────────────

void WalManager::log_begin(uint64_t txn_id) {
    WalRecord r;
    r.type   = WalRecordType::BEGIN;
    r.transaction_id = txn_id;
    write_record(r);
    flush();
}

void WalManager::log_write(uint64_t txn_id, uint32_t page_num,
                            const void* before, const void* /*after*/) {
    WalRecord r;
    r.type     = WalRecordType::WRITE;
    r.transaction_id  = txn_id;
    r.page_num = page_num;
    // We store the before-image so rollback can restore the page.
    // (after-image is implicit – it's whatever is in the pager cache.)
    r.before_image.resize(PAGE_SIZE);
    std::memcpy(r.before_image.data(), before, PAGE_SIZE);
    write_record(r);
    // Note: no flush here – we flush in bulk on commit for performance.
  
}

void WalManager::log_commit(uint64_t txn_id) {
    WalRecord r;
    r.type   = WalRecordType::COMMIT;
    r.transaction_id = txn_id;
    write_record(r);
    flush();   // WAL record MUST hit disk before page data does
    // Truncate immediately after commit: the pager will flush committed pages
    // to disk on db_close, so the WAL is no longer needed for recovery.
    truncate();
}

void WalManager::log_rollback(uint64_t txn_id) {
    WalRecord r;
    r.type   = WalRecordType::ROLLBACK;
    r.transaction_id = txn_id;
    write_record(r);
    flush();
}

void WalManager::flush() {
    file_.flush();
}


// Reads every record from the beginning of the WAL.

std::vector<WalRecord> WalManager::read_all() {
    std::vector<WalRecord> records;
    file_.clear();
    file_.seekg(0, std::ios::beg);

    while (true) {
        // Try to read type byte
        uint8_t type_byte;
        file_.read(reinterpret_cast<char*>(&type_byte), 1);
        if (file_.eof() || file_.fail()) break;

        WalRecord r;
        r.type   = static_cast<WalRecordType>(type_byte);
        r.transaction_id = read_u64(file_);
        if (file_.fail()) break;

        if (r.type == WalRecordType::WRITE) {
            r.page_num = read_u32(file_);
            uint32_t data_size = read_u32(file_);
            if (file_.fail()) break;
            r.before_image.resize(data_size);
            file_.read(reinterpret_cast<char*>(r.before_image.data()),
                       static_cast<std::streamsize>(data_size));
            if (file_.fail()) break;
        }

        records.push_back(std::move(r));
    }

    return records;
}

//  recover 
// Called once per table at db_open.

// This handles the multi-table case correctly: all tables share one WAL file,
// so we must not truncate it after processing the first table.

void WalManager::recover(Pager* pager) {
    // Read records from disk only once; reuse the cache for subsequent calls.
    if (recover_cache_.empty()) {
        recover_cache_ = read_all();
        if (recover_cache_.empty()) return;   // WAL is empty — nothing to do

        // Build the committed-txn set once (shared across all pager calls).
        for (const auto& r : recover_cache_)
            if (r.type == WalRecordType::COMMIT)
                recover_committed_.insert(r.transaction_id);

        // Truncate now: we have the records in memory and the file is no
        // longer needed. Subsequent recover() calls use recover_cache_.
        truncate();
    }

    if (recover_cache_.empty()) return;

    // Apply undo for this pager's pages.
    bool did_undo = false;
    for (const auto& r : recover_cache_) {
        if (r.type != WalRecordType::WRITE) continue;
        if (recover_committed_.count(r.transaction_id)) continue;  // committed

        void* page = pager->get_page(r.page_num);
        std::memcpy(page, r.before_image.data(),
                    std::min(r.before_image.size(), (size_t)PAGE_SIZE));
        pager->mark_dirty(r.page_num);
        did_undo = true;

        std::cerr << "[WAL recovery] undid partial write on page "
                  << r.page_num << " for txn " << r.transaction_id << "\n";
    }

    if (did_undo)
        pager->flush_all_dirty();
}


// Wipe the WAL file after a successful commit flush or recovery.

void WalManager::truncate() {
    file_.close();
    // Reopen with trunc to zero the file
    file_.open(path_, std::ios::in | std::ios::out |
                       std::ios::binary | std::ios::trunc);
    if (!file_.is_open())
        std::cerr << "[WAL] warning: could not truncate " << path_ << "\n";
}