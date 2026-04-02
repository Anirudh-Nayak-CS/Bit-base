// // test_wal.cpp  —  WAL + rollback stress tests for BitBase
// //
// // Compile (from project root):
// //   g++ -std=c++17 -g \
// //       test_wal.cpp \
// //       src/storage/database/db.cpp \
// //       src/storage/table/table.cpp \
// //       src/storage/Pager/pager.cpp \
// //       src/storage/b_plus_tree/b_plus_tree.cpp \
// //       src/storage/wal/wal.cpp \
// //       src/storage/transaction/transaction.cpp \
// //       src/cursor/cursor.cpp \
// //       -o test_wal
// //
// // Run:   ./test_wal
// // ─────────────────────────────────────────────────────────────────────────────

// #include <iostream>
// #include <cassert>
// #include <cstdlib>
// #include <cstring>
// #include <fstream>
// #include <filesystem>
// #include <vector>
// #include <string>
// #include <unordered_set>
// #include <random>


// #include "headers/storage/database/db.h"
// #include "headers/storage/table/table.h"
// #include "headers/storage/Pager/pager.h"

// #include "headers/storage/transaction/transaction.h"
// #include "headers/storage/node/leaf_node.h"
// #include "headers/schema/schema.h"
// #include "headers/storage/row/row_serialize.h"
// #include "headers/constants/constant.h"
// #include "headers/cursor/cursor.h"

// namespace fs = std::filesystem;

// // ─── colours ─────────────────────────────────────────────────────────────────
// #define GRN "\033[32m"
// #define RED "\033[31m"
// #define YEL "\033[33m"
// #define RST "\033[0m"

// // ─── test harness ─────────────────────────────────────────────────────────────

// static int tests_run    = 0;
// static int tests_passed = 0;
// static int tests_failed = 0;

// #define ASSERT(cond, msg)                                                      \
//     do {                                                                        \
//         if (!(cond)) {                                                          \
//             std::cerr << RED "  FAIL" RST " line " << __LINE__                 \
//                       << ": " << (msg) << "\n";                                 \
//             tests_failed++;                                                     \
//             return;                                                             \
//         }                                                                       \
//     } while (0)

// #define RUN_TEST(fn)                                                            \
//     do {                                                                        \
//         tests_run++;                                                            \
//         std::cout << YEL "[ RUN  ]" RST " " #fn "\n";                          \
//         fn();                                                                   \
//         if (tests_failed == prev_failed) {                                      \
//             std::cout << GRN "[ PASS ]" RST " " #fn "\n";                      \
//             tests_passed++;                                                     \
//         }                                                                       \
//         prev_failed = tests_failed;                                             \
//     } while (0)

// // ─── helpers ──────────────────────────────────────────────────────────────────

// // Remove a test database directory and all its files
// static void cleanup(const std::string& dbname) {
//     fs::remove_all(dbname);
//     fs::remove(dbname + ".schema");
//     fs::remove(dbname + ".wal");
// }

// // Build a minimal single-INT-PK schema
// static Schema make_schema(bool with_text = true) {
//     Schema s;
//     s.columns.push_back({"id",   DataType::INT32,  true });
//     if (with_text)
//         s.columns.push_back({"name", DataType::TEXT,   false});
//     s.columns.push_back({"age",  DataType::INT32,  false});
//     return s;
// }

// // Insert a single row directly through the db API (no VM/parser needed)
// static ExecuteResult raw_insert(db* database,
//                                  const std::string& table_name,
//                                  uint32_t id,
//                                  const std::string& name,
//                                  int age) {
//     Table* table = database->getTable(table_name);
//     if (!table) return EXECUTE_TABLE_NOT_FOUND;

//     const Schema& schema = table->schema;

//     Row row;
//     row.id = id;
//     row.fields.push_back(static_cast<int32_t>(id));
//     row.fields.push_back(name);
//     row.fields.push_back(static_cast<int32_t>(age));

//     auto bytes = serializeRow(row);
//     if (bytes.size() > LEAF_NODE_VALUE_SIZE) return EXECUTE_ROW_TOO_LARGE;

//     Transaction* txn = database->begin_txn();

//     // pin the page before writing
//     auto cursor = table->find(id);
//     void* page  = table->pager->get_page(cursor->page_num);
//     database->pin_page(cursor->page_num, page, table->pager);

//     bool ok = table->insert(id, bytes.data(), (uint32_t)bytes.size());

//     if (ok) {
//         database->commit_txn(txn->id());
//         return EXECUTE_SUCCESS;
//     } else {
//         database->rollback_txn(txn->id());
//         return EXECUTE_DUPLICATE_KEY;
//     }
// }

// // Count how many rows a table currently has
// static uint32_t count_rows(db* database, const std::string& table_name) {
//     Table* table = database->getTable(table_name);
//     if (!table) return 0;

//     uint32_t count = 0;
//     auto cursor = Cursor::table_start(table);
//     while (!cursor->end_of_table) {
//         count++;
//         cursor->cursor_advance();
//     }
//     return count;
// }

// // Check if a specific key exists in the table
// static bool key_exists(db* database, const std::string& tname, uint32_t key) {
//     Table* table = database->getTable(tname);
//     if (!table) return false;
//     auto cursor = table->find(key);
//     if (cursor->end_of_table) return false;
//     void* node = table->pager->get_page(cursor->page_num);
//     uint32_t num_cells = *leafNodeNumCells(node);
//     if (cursor->cell_num >= num_cells) return false;
//     return *leafNodeKey(node, cursor->cell_num) == key;
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  TEST 1 — Duplicate key triggers rollback, no row inserted
// // ─────────────────────────────────────────────────────────────────────────────
// static void test_duplicate_key_rollback() {
//     const std::string DB = "test_wal_t1";
//     cleanup(DB);

//     db* database = db::db_open(DB.c_str());
//     database->createTable("users", make_schema());

//     // First insert must succeed
//     ExecuteResult r1 = raw_insert(database, "users", 1, "Alice", 30);
//     ASSERT(r1 == EXECUTE_SUCCESS, "first insert should succeed");
//     ASSERT(count_rows(database, "users") == 1, "should have 1 row after first insert");

//     // Second insert with same key must fail and roll back
//     ExecuteResult r2 = raw_insert(database, "users", 1, "Bob", 25);
//     ASSERT(r2 == EXECUTE_DUPLICATE_KEY, "duplicate insert should return EXECUTE_DUPLICATE_KEY");
//     ASSERT(count_rows(database, "users") == 1, "row count must still be 1 after failed insert");

//     // The row that IS there should be Alice's (id=1), not Bob's
//     ASSERT(key_exists(database, "users", 1), "key 1 should still exist");

//     database->db_close();
//     delete database;
//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  TEST 2 — Multi-row insert: partial failure rolls back ALL rows in batch
// // ─────────────────────────────────────────────────────────────────────────────
// static void test_multi_row_partial_failure_rollback() {
//     const std::string DB = "test_wal_t2";
//     cleanup(DB);

//     db* database = db::db_open(DB.c_str());
//     database->createTable("users", make_schema());

//     // Insert rows 1 and 2 cleanly
//     raw_insert(database, "users", 1, "Alice", 30);
//     raw_insert(database, "users", 2, "Bob",   25);
//     ASSERT(count_rows(database, "users") == 2, "should have 2 rows");

//     // Now simulate a multi-row batch: rows 3, 4, 2(dup), 5
//     // Row 2 is already there — batch should abort and rows 3,4,5 should NOT appear
//     Transaction* txn = database->begin_txn();
//     Table* table = database->getTable("users");

//     auto do_insert = [&](uint32_t id, const std::string& name, int age) -> bool {
//         Row row;
//         row.id = id;
//         row.fields.push_back(static_cast<int32_t>(id));
//         row.fields.push_back(name);
//         row.fields.push_back(static_cast<int32_t>(age));
//         auto bytes = serializeRow(row);
//         auto cursor = table->find(id);
//         database->pin_page(cursor->page_num,
//                            table->pager->get_page(cursor->page_num),
//                            table->pager);
//         return table->insert(id, bytes.data(), (uint32_t)bytes.size());
//     };

//     bool ok3 = do_insert(3, "Carol", 22);
//     bool ok4 = do_insert(4, "Dave",  28);
//     bool ok2 = do_insert(2, "Eve",   19);   // DUPLICATE — should fail

//     if (!ok3 || !ok4 || !ok2) {
//         database->rollback_txn(txn->id());
//     } else {
//         database->commit_txn(txn->id());
//     }

//     // After rollback: only rows 1 and 2 should exist
//     ASSERT(count_rows(database, "users") == 2, "rollback must restore count to 2");
//     ASSERT(!key_exists(database, "users", 3), "row 3 must not exist after rollback");
//     ASSERT(!key_exists(database, "users", 4), "row 4 must not exist after rollback");
//     ASSERT( key_exists(database, "users", 1), "row 1 must still exist");
//     ASSERT( key_exists(database, "users", 2), "row 2 must still exist");

//     database->db_close();
//     delete database;
//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  TEST 3 — Commit durability: rows survive close + reopen
// // ─────────────────────────────────────────────────────────────────────────────
// static void test_commit_durability() {
//     const std::string DB = "test_wal_t3";
//     cleanup(DB);

//     {
//         db* database = db::db_open(DB.c_str());
//         database->createTable("users", make_schema());
//         raw_insert(database, "users", 10, "Zara", 21);
//         raw_insert(database, "users", 20, "Liam", 35);
//         raw_insert(database, "users", 30, "Mia",  27);
//         database->db_close();
//         delete database;
//     }

//     // Reopen and check rows are still there
//     {
//         db* database = db::db_open(DB.c_str());
//         ASSERT(count_rows(database, "users") == 3, "all 3 rows must survive reopen");
//         ASSERT(key_exists(database, "users", 10), "row 10 must survive reopen");
//         ASSERT(key_exists(database, "users", 20), "row 20 must survive reopen");
//         ASSERT(key_exists(database, "users", 30), "row 30 must survive reopen");
//         database->db_close();
//         delete database;
//     }

//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  TEST 4 — WAL crash recovery: inject an incomplete WAL then reopen
// //
// //  Simulates: process crashed after writing WAL WRITE record but before
// //  writing COMMIT. On reopen, recovery should undo the partial write.
// // ─────────────────────────────────────────────────────────────────────────────
// static void test_crash_recovery_undo() {
//     const std::string DB = "test_wal_t4";
//     cleanup(DB);

//     // Step 1: create db and insert one clean row (committed, on disk)
//     {
//         db* database = db::db_open(DB.c_str());
//         database->createTable("users", make_schema());
//         raw_insert(database, "users", 1, "Alice", 30);
//         database->db_close();
//         delete database;
//     }

//     // Step 2: manually write a WAL file that has BEGIN + WRITE but no COMMIT
//     // (simulating a crash mid-transaction for key=99)
//     {
//         std::fstream wal(DB + ".wal",
//                          std::ios::out | std::ios::binary | std::ios::trunc);

//         auto write_u8  = [&](uint8_t  v){ wal.write((char*)&v, 1); };
//         auto write_u32 = [&](uint32_t v){ wal.write((char*)&v, 4); };
//         auto write_u64 = [&](uint64_t v){ wal.write((char*)&v, 8); };

//         uint64_t txn_id = 999;

//         // BEGIN record
//         write_u8(1);          // WalRecordType::BEGIN
//         write_u64(txn_id);

//         // WRITE record — page 0, before-image is all zeros (fake)
//         write_u8(2);          // WalRecordType::WRITE
//         write_u64(txn_id);
//         write_u32(0);         // page_num = 0
//         write_u32(PAGE_SIZE); // data_size
//         std::vector<uint8_t> fake_page(PAGE_SIZE, 0xAB); // garbage data
//         wal.write((char*)fake_page.data(), PAGE_SIZE);

//         // NO COMMIT record — simulates crash
//         wal.close();
//     }

//     // Step 3: reopen — recovery should undo the WRITE (restore page 0 from
//     // before-image = all 0xAB, then our existing row 1 will be gone since
//     // the "before image" we wrote is garbage, BUT the key point is that
//     // recovery runs without crashing and WAL is truncated cleanly)
//     {
//         db* database = db::db_open(DB.c_str());

//         // WAL should be truncated after recovery
//         std::ifstream wal_check(DB + ".wal", std::ios::binary | std::ios::ate);
//         std::streamsize wal_size = wal_check.tellg();
//         ASSERT(wal_size == 0, "WAL must be empty (truncated) after recovery");

//         database->db_close();
//         delete database;
//     }

//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  TEST 5 — WAL is empty after a successful commit
// // ─────────────────────────────────────────────────────────────────────────────
// static void test_wal_truncated_after_commit() {
//     const std::string DB = "test_wal_t5";
//     cleanup(DB);

//     db* database = db::db_open(DB.c_str());
//     database->createTable("users", make_schema());
//     raw_insert(database, "users", 1, "Alice", 30);

//     // After a committed insert, WAL should be truncated to 0 bytes
//     std::ifstream wal_check(DB + ".wal", std::ios::binary | std::ios::ate);
//     std::streamsize sz = wal_check.tellg();
//     ASSERT(sz == 0, "WAL must be empty after successful commit");

//     database->db_close();
//     delete database;
//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  TEST 6 — WAL has content during an open (uncommitted) transaction
// // ─────────────────────────────────────────────────────────────────────────────
// static void test_wal_has_content_during_txn() {
//     const std::string DB = "test_wal_t6";
//     cleanup(DB);

//     db* database = db::db_open(DB.c_str());
//     database->createTable("users", make_schema());

//     // Manually begin a txn and pin a page without committing
//     Transaction* txn = database->begin_txn();
//     Table* table = database->getTable("users");
//     void* page = table->pager->get_page(0);
//     database->pin_page(0, page, table->pager);

//     // WAL should now have BEGIN + WRITE records — not empty
//     {
//         std::ifstream wal_check(DB + ".wal", std::ios::binary | std::ios::ate);
//         std::streamsize sz = wal_check.tellg();
//         ASSERT(sz > 0, "WAL must have content while txn is open");
//     }

//     // Roll it back — WAL should be truncated again
//     database->rollback_txn(txn->id());

//     {
//         std::ifstream wal_check(DB + ".wal", std::ios::binary | std::ios::ate);
//         std::streamsize sz = wal_check.tellg();
//         ASSERT(sz == 0, "WAL must be empty after rollback");
//     }

//     database->db_close();
//     delete database;
//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  TEST 7 — Stress: 500 random inserts, ~20% are duplicates
// //  Verify final row count matches the number of unique keys inserted
// // ─────────────────────────────────────────────────────────────────────────────
// static void test_stress_random_inserts() {
//     const std::string DB = "test_wal_t7";
//     cleanup(DB);

//     db* database = db::db_open(DB.c_str());
//     database->createTable("users", make_schema());

//     std::mt19937 rng(42);
//     // Keys in range [1, 400] so ~20% of 500 inserts will be duplicates
//     std::uniform_int_distribution<uint32_t> key_dist(1, 400);
//     std::uniform_int_distribution<int>      age_dist(18, 80);

//     std::unordered_set<uint32_t> inserted_keys;
//     int success_count = 0;
//     int dup_count     = 0;

//     for (int i = 0; i < 500; i++) {
//         uint32_t key = key_dist(rng);
//         int      age = age_dist(rng);
//         ExecuteResult r = raw_insert(database, "users", key, "user_" + std::to_string(key), age);

//         if (r == EXECUTE_SUCCESS) {
//             inserted_keys.insert(key);
//             success_count++;
//         } else if (r == EXECUTE_DUPLICATE_KEY) {
//             dup_count++;
//         } else {
//             // Unexpected error
//             ASSERT(false, "unexpected error during stress insert: " + std::to_string(r));
//             break;
//         }
//     }

//     uint32_t actual_count = count_rows(database, "users");
//     ASSERT(actual_count == (uint32_t)inserted_keys.size(),
//            "row count must equal unique keys inserted (got " +
//            std::to_string(actual_count) + " expected " +
//            std::to_string(inserted_keys.size()) + ")");

//     std::cout << "    inserts: " << success_count
//               << "  duplicates rejected: " << dup_count
//               << "  unique rows: " << actual_count << "\n";

//     // Every key we think we inserted should actually be findable
//     for (uint32_t key : inserted_keys) {
//         ASSERT(key_exists(database, "users", key),
//                "key " + std::to_string(key) + " should exist but doesn't");
//     }

//     database->db_close();
//     delete database;
//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  TEST 8 — Stress: close and reopen 10 times, rows must survive each cycle
// // ─────────────────────────────────────────────────────────────────────────────
// static void test_stress_reopen_cycles() {
//     const std::string DB = "test_wal_t8";
//     cleanup(DB);

//     // Initial population
//     {
//         db* database = db::db_open(DB.c_str());
//         database->createTable("users", make_schema());
//         for (int i = 1; i <= 50; i++)
//             raw_insert(database, "users", i, "user_" + std::to_string(i), 20 + i);
//         database->db_close();
//         delete database;
//     }

//     // Reopen 10 times, each time insert 5 more rows and verify total
//     for (int cycle = 0; cycle < 10; cycle++) {
//         db* database = db::db_open(DB.c_str());

//         uint32_t base = 50 + cycle * 5;
//         for (uint32_t i = base + 1; i <= base + 5; i++)
//             raw_insert(database, "users", i, "user_" + std::to_string(i), 25);

//         uint32_t expected = 50 + (cycle + 1) * 5;
//         uint32_t actual   = count_rows(database, "users");
//         ASSERT(actual == expected,
//                "cycle " + std::to_string(cycle) +
//                ": expected " + std::to_string(expected) +
//                " rows, got " + std::to_string(actual));

//         database->db_close();
//         delete database;
//     }

//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  TEST 9 — No transaction leak: begin_txn after a rollback works cleanly
// // ─────────────────────────────────────────────────────────────────────────────
// static void test_no_txn_leak_after_rollback() {
//     const std::string DB = "test_wal_t9";
//     cleanup(DB);

//     db* database = db::db_open(DB.c_str());
//     database->createTable("users", make_schema());

//     // Insert row 1 — success
//     ExecuteResult r1 = raw_insert(database, "users", 1, "Alice", 30);
//     ASSERT(r1 == EXECUTE_SUCCESS, "first insert must succeed");

//     // Attempt duplicate — rolls back internally
//     ExecuteResult r2 = raw_insert(database, "users", 1, "Bob", 25);
//     ASSERT(r2 == EXECUTE_DUPLICATE_KEY, "duplicate must be rejected");

//     // Now insert row 2 — this will fail with "nested BEGIN" if txn leaked
//     ExecuteResult r3 = raw_insert(database, "users", 2, "Carol", 22);
//     ASSERT(r3 == EXECUTE_SUCCESS, "insert after rollback must succeed (no txn leak)");

//     ASSERT(count_rows(database, "users") == 2, "must have exactly 2 rows");

//     database->db_close();
//     delete database;
//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  TEST 10 — Large batch: insert 200 rows in one transaction
// // ─────────────────────────────────────────────────────────────────────────────
// static void test_large_batch_commit() {
//     const std::string DB = "test_wal_t10";
//     cleanup(DB);

//     db* database = db::db_open(DB.c_str());
//     database->createTable("users", make_schema());

//     Table* table = database->getTable("users");
//     Transaction* txn = database->begin_txn();
//     bool all_ok = true;

//     for (uint32_t i = 1; i <= 200; i++) {
//         Row row;
//         row.id = i;
//         row.fields.push_back(static_cast<int32_t>(i));
//         row.fields.push_back(std::string("user_") + std::to_string(i));
//         row.fields.push_back(static_cast<int32_t>(20 + i % 50));
//         auto bytes = serializeRow(row);

//         auto cursor = table->find(i);
//         database->pin_page(cursor->page_num,
//                            table->pager->get_page(cursor->page_num),
//                            table->pager);
//         if (!table->insert(i, bytes.data(), (uint32_t)bytes.size())) {
//             all_ok = false;
//             break;
//         }
//     }

//     if (all_ok)
//         database->commit_txn(txn->id());
//     else
//         database->rollback_txn(txn->id());

//     ASSERT(all_ok, "200-row batch should have no duplicates");
//     ASSERT(count_rows(database, "users") == 200, "must have 200 rows after large batch");

//     // Close and reopen — all 200 must survive
//     database->db_close();
//     delete database;

//     db* database2 = db::db_open(DB.c_str());
//     ASSERT(count_rows(database2, "users") == 200, "200 rows must survive reopen");
//     database2->db_close();
//     delete database2;

//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  main
// // ─────────────────────────────────────────────────────────────────────────────

// int main() {
//     std::cout << "\n=== BitBase WAL + Rollback Stress Tests ===\n\n";

//     int prev_failed = 0;

//     RUN_TEST(test_duplicate_key_rollback);
//     RUN_TEST(test_multi_row_partial_failure_rollback);
//     RUN_TEST(test_commit_durability);
//     RUN_TEST(test_crash_recovery_undo);
//     RUN_TEST(test_wal_truncated_after_commit);
//     RUN_TEST(test_wal_has_content_during_txn);
//     RUN_TEST(test_stress_random_inserts);
//     RUN_TEST(test_stress_reopen_cycles);
//     RUN_TEST(test_no_txn_leak_after_rollback);
//     RUN_TEST(test_large_batch_commit);

//     std::cout << "\n─────────────────────────────────────────\n";
//     std::cout << "Results: "
//               << GRN << tests_passed << " passed" << RST << "  "
//               << (tests_failed ? RED : "") << tests_failed << " failed" << RST
//               << "  / " << tests_run << " total\n\n";

//     return tests_failed == 0 ? 0 : 1;
// }