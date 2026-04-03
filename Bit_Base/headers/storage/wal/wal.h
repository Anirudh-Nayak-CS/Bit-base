// headers/storage/wal/wal.h
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_set>

enum class WalRecordType : uint8_t {
    BEGIN    = 1,   // transaction started
    WRITE    = 2,   // page was modified
    COMMIT   = 3,   // transaction successfully finished
    ROLLBACK = 4,  // transaction was aborted
};

struct WalRecord {
    WalRecordType type;
    uint64_t      transaction_id;
    uint32_t      page_num;          // meaningful for WRITE records
    std::vector<uint8_t> before_image; // before image of page (Data)
};

class WalManager {
public:
    explicit WalManager(const std::string& wal_path);
    ~WalManager();

    void log_begin(uint64_t transaction_id);
    void log_write(uint64_t transaction_id, uint32_t page_num,
                   const void* before, const void* after);
    void log_commit(uint64_t transaction_id);
    void log_rollback(uint64_t transaction_id);

    // Called at db_open: replay any committed-but-unflushed transactions,
    // discard any incomplete ones.
    void recover(class Pager* pager);

    void flush();   // fsync the log file

    void truncate();

private:
    std::fstream file_;
    std::string  path_;
    void write_record(const WalRecord& r);
    std::vector<WalRecord> read_all();

    // Cache used by recover() so multi-table recovery reads the WAL only once.
    std::vector<WalRecord>       recover_cache_;
    std::unordered_set<uint64_t> recover_committed_;
};