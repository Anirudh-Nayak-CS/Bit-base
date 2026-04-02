// // test_sql.cpp  —  SQL command stress tests for BitBase
// //
// // Compile (from project root, adjust paths to match your layout):
// //   g++ -std=c++17 -g \
// //       test_sql.cpp \
// //       src/storage/database/db.cpp \
// //       src/storage/table/table.cpp \
// //       src/storage/Pager/pager.cpp \
// //       src/storage/b_plus_tree/b_plus_tree.cpp \
// //       src/storage/wal/wal.cpp \
// //       src/storage/transaction/transaction.cpp \
// //       src/tokenizer/tokenizer.cpp \
// //       src/parser/parser.cpp \
// //       src/vm/vm.cpp \
// //       src/cursor/cursor.cpp \
// //       -o test_sql
// //
// // Run:  ./test_sql
// // ─────────────────────────────────────────────────────────────────────────────

// #include <iostream>
// #include <cassert>
// #include <string>
// #include <vector>
// #include <filesystem>
// #include <random>
// #include <algorithm>
// #include <sstream>

// #include "headers/tokenizer/tokenizer.h"
// #include "headers/parser/parser.h"
// #include "headers/vm/vm.h"
// #include "headers/storage/database/db.h"
// #include "headers/statement/statement.h"
// #include "headers/constants/constant.h"
// #include "headers/cursor/cursor.h"
// #include "headers/storage/node/leaf_node.h"

// namespace fs = std::filesystem;

// // ─── colours ─────────────────────────────────────────────────────────────────
// #define GRN "\033[32m"
// #define RED "\033[31m"
// #define YEL "\033[33m"
// #define CYN "\033[36m"
// #define RST "\033[0m"

// // ─── harness ──────────────────────────────────────────────────────────────────
// static int tests_run    = 0;
// static int tests_passed = 0;
// static int tests_failed = 0;
// static int prev_failed  = 0;

// #define ASSERT(cond, msg)                                               \
//     do {                                                                \
//         if (!(cond)) {                                                  \
//             std::cerr << RED "  FAIL" RST " (line " << __LINE__        \
//                       << "): " << (msg) << "\n";                       \
//             tests_failed++;                                             \
//             return;                                                     \
//         }                                                               \
//     } while (0)

// #define RUN_TEST(fn)                                                    \
//     do {                                                                \
//         tests_run++;                                                    \
//         std::cout << YEL "[ RUN  ]" RST " " #fn "\n";                  \
//         fn();                                                           \
//         if (tests_failed == prev_failed) {                              \
//             std::cout << GRN "[ PASS ]" RST " " #fn "\n";              \
//             tests_passed++;                                             \
//         }                                                               \
//         prev_failed = tests_failed;                                     \
//     } while (0)

// // ─── helpers ──────────────────────────────────────────────────────────────────

// static void cleanup(const std::string& dbname) {
//     fs::remove_all(dbname);
//     fs::remove(dbname + ".schema");
//     fs::remove(dbname + ".wal");
// }

// // Run one SQL string through tokenizer → parser → vm.
// // Suppresses stdout during execution so test output stays clean.
// static ExecuteResult run_sql(db* database, const std::string& sql,
//                               Statement* out_stmt = nullptr) {
//     auto tokens = tokenize(sql);
//     Statement stmt;
//     Parser parser;
//     Commandstatus ps = parser.parse(tokens, stmt);
//     if (ps != CMD_SUCCESS) return (ExecuteResult)-1;   // parse error sentinel
//     if (out_stmt) *out_stmt = stmt;
//     VM vm(database);
//     // redirect cout to /dev/null so print output doesn't pollute test log
//     std::streambuf* old = std::cout.rdbuf();
//     std::ostringstream sink;
//     std::cout.rdbuf(sink.rdbuf());
//     ExecuteResult r = vm.execute(stmt);
//     std::cout.rdbuf(old);
//     return r;
// }

// // Count rows in a table by scanning
// static uint32_t count_rows(db* database, const std::string& tname) {
//     Table* table = database->getTable(tname);
//     if (!table) return 0;
//     uint32_t n = 0;
//     auto cursor = Cursor::table_start(table);
//     while (!cursor->end_of_table) { n++; cursor->cursor_advance(); }
//     return n;
// }

// // Check if a key exists
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
// //  CREATE TABLE tests
// // ─────────────────────────────────────────────────────────────────────────────

// static void test_create_table_basic() {
//     const std::string DB = "tsql_ct1";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     ExecuteResult r = run_sql(database,
//         "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     ASSERT(r == EXECUTE_SUCCESS, "basic CREATE TABLE must succeed");
//     ASSERT(database->getTable("users") != nullptr, "table must exist after CREATE");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_create_table_duplicate() {
//     const std::string DB = "tsql_ct2";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT)");
//     ExecuteResult r = run_sql(database,
//         "CREATE TABLE users (id INT PRIMARY KEY, age INT)");
//     ASSERT(r == EXECUTE_TABLE_EXISTS, "duplicate CREATE TABLE must return TABLE_EXISTS");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_create_table_no_primary_key() {
//     const std::string DB = "tsql_ct3";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     ExecuteResult r = run_sql(database,
//         "CREATE TABLE nopk (name TEXT, age INT)");
//     ASSERT(r == EXECUTE_SCHEMA_MISMATCH, "CREATE TABLE without PK must fail");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_create_table_multiple_pk() {
//     const std::string DB = "tsql_ct4";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     ExecuteResult r = run_sql(database,
//         "CREATE TABLE bad (id INT PRIMARY KEY, code INT PRIMARY KEY, name TEXT)");
//     ASSERT(r == EXECUTE_SCHEMA_MISMATCH, "multiple PKs must fail");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_create_table_text_primary_key() {
//     const std::string DB = "tsql_ct5";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     // TEXT primary key must be rejected (only INT allowed)
//     ExecuteResult r = run_sql(database,
//         "CREATE TABLE bad (name TEXT PRIMARY KEY, age INT)");
//     ASSERT(r == EXECUTE_TYPE_ERROR, "TEXT PRIMARY KEY must return TYPE_ERROR");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_create_many_tables() {
//     const std::string DB = "tsql_ct6";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     for (int i = 0; i < 20; i++) {
//         std::string sql = "CREATE TABLE t" + std::to_string(i) +
//                           " (id INT PRIMARY KEY, val INT)";
//         ExecuteResult r = run_sql(database, sql);
//         ASSERT(r == EXECUTE_SUCCESS,
//                "CREATE TABLE t" + std::to_string(i) + " must succeed");
//     }
//     ASSERT(database->tables.size() == 20, "must have 20 tables");

//     database->db_close(); delete database; cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  INSERT tests
// // ─────────────────────────────────────────────────────────────────────────────

// static void test_insert_single_row() {
//     const std::string DB = "tsql_ins1";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     ExecuteResult r = run_sql(database, "INSERT INTO users VALUES (1, \"Alice\", 30)");
//     ASSERT(r == EXECUTE_SUCCESS, "single insert must succeed");
//     ASSERT(count_rows(database, "users") == 1, "must have 1 row");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_insert_multi_row() {
//     const std::string DB = "tsql_ins2";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     ExecuteResult r = run_sql(database,
//         "INSERT INTO users VALUES (1,\"Alice\",30),(2,\"Bob\",25),(3,\"Carol\",22)");
//     ASSERT(r == EXECUTE_SUCCESS, "multi-row insert must succeed");
//     ASSERT(count_rows(database, "users") == 3, "must have 3 rows");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_insert_duplicate_key() {
//     const std::string DB = "tsql_ins3";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");
//     ExecuteResult r = run_sql(database, "INSERT INTO users VALUES (1,\"Bob\",25)");
//     ASSERT(r == EXECUTE_DUPLICATE_KEY, "duplicate key must fail");
//     ASSERT(count_rows(database, "users") == 1, "row count must stay 1");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_insert_multi_row_with_duplicate_rolls_back() {
//     const std::string DB = "tsql_ins4";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");

//     // row 1 already exists — batch must roll back completely
//     ExecuteResult r = run_sql(database,
//         "INSERT INTO users VALUES (2,\"Bob\",25),(3,\"Carol\",22),(1,\"Dup\",99)");
//     ASSERT(r == EXECUTE_DUPLICATE_KEY, "batch with dup must fail");
//     // rows 2 and 3 must NOT have been committed
//     ASSERT(count_rows(database, "users") == 1, "rollback must leave only original row");
//     ASSERT(!key_exists(database, "users", 2), "row 2 must not exist after rollback");
//     ASSERT(!key_exists(database, "users", 3), "row 3 must not exist after rollback");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_insert_wrong_column_count() {
//     const std::string DB = "tsql_ins5";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     // only 2 values for a 3-column table
//     ExecuteResult r = run_sql(database, "INSERT INTO users VALUES (1,\"Alice\")");
//     ASSERT(r == EXECUTE_SCHEMA_MISMATCH, "wrong column count must return SCHEMA_MISMATCH");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_insert_table_not_found() {
//     const std::string DB = "tsql_ins6";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     ExecuteResult r = run_sql(database,
//         "INSERT INTO nonexistent VALUES (1,\"x\",10)");
//     ASSERT(r == EXECUTE_TABLE_NOT_FOUND, "insert into missing table must fail");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_insert_stress_500_rows() {
//     const std::string DB = "tsql_ins7";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE data (id INT PRIMARY KEY, score INT)");

//     std::mt19937 rng(7);
//     std::unordered_set<int> inserted;

//     for (int i = 1; i <= 500; i++) {
//         std::string sql = "INSERT INTO data VALUES (" +
//                           std::to_string(i) + "," + std::to_string(i * 3) + ")";
//         ExecuteResult r = run_sql(database, sql);
//         ASSERT(r == EXECUTE_SUCCESS,
//                "stress insert " + std::to_string(i) + " must succeed");
//         inserted.insert(i);
//     }

//     ASSERT(count_rows(database, "data") == 500, "must have 500 rows after stress insert");

//     // spot-check 20 random keys
//     std::uniform_int_distribution<int> dist(1, 500);
//     for (int i = 0; i < 20; i++) {
//         int k = dist(rng);
//         ASSERT(key_exists(database, "data", k),
//                "key " + std::to_string(k) + " must exist");
//     }

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_insert_negative_id() {
//     const std::string DB = "tsql_ins8";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     // negative id — stoi will parse it, stored as uint32_t (wraps)
//     // just verify it doesn't crash and returns some result
//     ExecuteResult r = run_sql(database, "INSERT INTO users VALUES (-1,\"Ghost\",0)");
//     // not asserting success/fail — just that it doesn't crash
//     (void)r;

//     database->db_close(); delete database; cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  SELECT tests
// // ─────────────────────────────────────────────────────────────────────────────

// static void test_select_star() {
//     const std::string DB = "tsql_sel1";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");
//     run_sql(database, "INSERT INTO users VALUES (2,\"Bob\",25)");

//     ExecuteResult r = run_sql(database, "SELECT * FROM users");
//     ASSERT(r == EXECUTE_SUCCESS, "SELECT * must succeed");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_specific_columns() {
//     const std::string DB = "tsql_sel2";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");

//     ExecuteResult r = run_sql(database, "SELECT name,age FROM users");
//     ASSERT(r == EXECUTE_SUCCESS, "SELECT specific cols must succeed");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_invalid_column() {
//     const std::string DB = "tsql_sel3";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");

//     ExecuteResult r = run_sql(database, "SELECT salary FROM users");
//     ASSERT(r == EXECUTE_COLUMN_NOT_FOUND, "invalid column must return COLUMN_NOT_FOUND");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_table_not_found() {
//     const std::string DB = "tsql_sel4";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     ExecuteResult r = run_sql(database, "SELECT * FROM ghost");
//     ASSERT(r == EXECUTE_TABLE_NOT_FOUND, "SELECT from missing table must fail");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_where_eq() {
//     const std::string DB = "tsql_sel5";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     for (int i = 1; i <= 10; i++) {
//         run_sql(database, "INSERT INTO users VALUES (" +
//             std::to_string(i) + ",\"user" + std::to_string(i) + "\"," +
//             std::to_string(20 + i) + ")");
//     }

//     ExecuteResult r = run_sql(database, "SELECT * FROM users WHERE id = 5");
//     ASSERT(r == EXECUTE_SUCCESS, "WHERE id=5 must succeed");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_where_gt() {
//     const std::string DB = "tsql_sel6";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     for (int i = 1; i <= 10; i++)
//         run_sql(database, "INSERT INTO users VALUES (" + std::to_string(i) +
//             ",\"u\","+ std::to_string(18 + i) + ")");

//     ExecuteResult r = run_sql(database, "SELECT * FROM users WHERE age > 25");
//     ASSERT(r == EXECUTE_SUCCESS, "WHERE age>25 must succeed");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_where_invalid_col() {
//     const std::string DB = "tsql_sel7";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");

//     ExecuteResult r = run_sql(database, "SELECT * FROM users WHERE salary = 50000");
//     ASSERT(r == EXECUTE_COLUMN_NOT_FOUND,
//            "WHERE on nonexistent column must return COLUMN_NOT_FOUND");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_order_by_asc() {
//     const std::string DB = "tsql_sel8";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     // insert out of order
//     run_sql(database, "INSERT INTO users VALUES (3,\"Carol\",22)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",35)");
//     run_sql(database, "INSERT INTO users VALUES (2,\"Bob\",28)");

//     ExecuteResult r = run_sql(database, "SELECT * FROM users ORDER BY age ASC");
//     ASSERT(r == EXECUTE_SUCCESS, "ORDER BY age ASC must succeed");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_order_by_desc() {
//     const std::string DB = "tsql_sel9";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");
//     run_sql(database, "INSERT INTO users VALUES (2,\"Bob\",22)");
//     run_sql(database, "INSERT INTO users VALUES (3,\"Carol\",35)");

//     ExecuteResult r = run_sql(database, "SELECT * FROM users ORDER BY age DESC");
//     ASSERT(r == EXECUTE_SUCCESS, "ORDER BY age DESC must succeed");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_order_by_invalid_col() {
//     const std::string DB = "tsql_sel10";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");

//     ExecuteResult r = run_sql(database, "SELECT * FROM users ORDER BY salary");
//     ASSERT(r == EXECUTE_COLUMN_NOT_FOUND,
//            "ORDER BY nonexistent col must return COLUMN_NOT_FOUND");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_where_and_order_by() {
//     const std::string DB = "tsql_sel11";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE emp (id INT PRIMARY KEY, name TEXT, dept INT, salary INT)");
//     run_sql(database, "INSERT INTO emp VALUES (1,\"Ana\",2,70000)");
//     run_sql(database, "INSERT INTO emp VALUES (2,\"Ben\",1,50000)");
//     run_sql(database, "INSERT INTO emp VALUES (3,\"Cara\",2,90000)");
//     run_sql(database, "INSERT INTO emp VALUES (4,\"Dan\",1,60000)");
//     run_sql(database, "INSERT INTO emp VALUES (5,\"Eve\",2,80000)");

//     ExecuteResult r = run_sql(database,
//         "SELECT name,salary FROM emp WHERE dept = 2 ORDER BY salary DESC");
//     ASSERT(r == EXECUTE_SUCCESS, "WHERE + ORDER BY combined must succeed");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_select_empty_table() {
//     const std::string DB = "tsql_sel12";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE empty (id INT PRIMARY KEY, val INT)");
//     ExecuteResult r = run_sql(database, "SELECT * FROM empty");
//     ASSERT(r == EXECUTE_SUCCESS, "SELECT on empty table must succeed");

//     database->db_close(); delete database; cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  UPDATE tests
// // ─────────────────────────────────────────────────────────────────────────────

// static void test_update_single_col() {
//     const std::string DB = "tsql_upd1";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");
//     ExecuteResult r = run_sql(database,
//         "UPDATE users SET age = 31 WHERE id = 1");
//     ASSERT(r == EXECUTE_SUCCESS, "UPDATE single col must succeed");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_update_multiple_cols() {
//     const std::string DB = "tsql_upd2";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");
//     ExecuteResult r = run_sql(database,
//         "UPDATE users SET name = \"Alicia\", age = 31 WHERE id = 1");
//     ASSERT(r == EXECUTE_SUCCESS, "UPDATE multiple cols must succeed");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_update_no_where_updates_all() {
//     const std::string DB = "tsql_upd3";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     for (int i = 1; i <= 5; i++)
//         run_sql(database, "INSERT INTO users VALUES (" + std::to_string(i) +
//             ",\"user\",20)");

//     // no WHERE — all rows get updated
//     ExecuteResult r = run_sql(database, "UPDATE users SET age = 99");
//     ASSERT(r == EXECUTE_SUCCESS, "UPDATE without WHERE must succeed");
//     ASSERT(count_rows(database, "users") == 5, "row count must not change after update");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_update_nonexistent_key() {
//     const std::string DB = "tsql_upd4";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");

//     ExecuteResult r = run_sql(database,
//         "UPDATE users SET age = 99 WHERE id = 999");
//     ASSERT(r == EXECUTE_KEY_NOT_FOUND,
//            "UPDATE on missing key must return KEY_NOT_FOUND");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_update_invalid_column() {
//     const std::string DB = "tsql_upd5";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\",30)");

//     ExecuteResult r = run_sql(database,
//         "UPDATE users SET salary = 50000 WHERE id = 1");
//     ASSERT(r == EXECUTE_COLUMN_NOT_FOUND,
//            "UPDATE on nonexistent col must return COLUMN_NOT_FOUND");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_update_table_not_found() {
//     const std::string DB = "tsql_upd6";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     ExecuteResult r = run_sql(database,
//         "UPDATE ghost SET age = 1 WHERE id = 1");
//     ASSERT(r == EXECUTE_TABLE_NOT_FOUND,
//            "UPDATE on missing table must return TABLE_NOT_FOUND");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_update_stress() {
//     const std::string DB = "tsql_upd7";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE scores (id INT PRIMARY KEY, val INT)");
//     for (int i = 1; i <= 100; i++)
//         run_sql(database, "INSERT INTO scores VALUES (" +
//             std::to_string(i) + ",0)");

//     // update every row one by one
//     for (int i = 1; i <= 100; i++) {
//         ExecuteResult r = run_sql(database,
//             "UPDATE scores SET val = " + std::to_string(i * 10) +
//             " WHERE id = " + std::to_string(i));
//         ASSERT(r == EXECUTE_SUCCESS,
//                "stress update id=" + std::to_string(i) + " must succeed");
//     }

//     ASSERT(count_rows(database, "scores") == 100,
//            "row count must stay 100 after 100 updates");

//     database->db_close(); delete database; cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  DROP TABLE tests
// // ─────────────────────────────────────────────────────────────────────────────

// static void test_drop_table_basic() {
//     const std::string DB = "tsql_drop1";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT)");
//     ExecuteResult r = run_sql(database, "DROP TABLE users");
//     ASSERT(r == EXECUTE_SUCCESS, "DROP TABLE must succeed");
//     ASSERT(database->getTable("users") == nullptr, "table must not exist after DROP");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_drop_table_not_found() {
//     const std::string DB = "tsql_drop2";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     ExecuteResult r = run_sql(database, "DROP TABLE ghost");
//     ASSERT(r == EXECUTE_TABLE_NOT_FOUND, "DROP missing table must fail");

//     database->db_close(); delete database; cleanup(DB);
// }

// static void test_drop_then_recreate() {
//     const std::string DB = "tsql_drop3";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT)");
//     run_sql(database, "INSERT INTO users VALUES (1,\"Alice\")");
//     run_sql(database, "DROP TABLE users");

//     // recreate with different schema — must succeed
//     ExecuteResult r = run_sql(database,
//         "CREATE TABLE users (id INT PRIMARY KEY, email TEXT, score INT)");
//     ASSERT(r == EXECUTE_SUCCESS, "recreate after DROP must succeed");

//     // insert into new schema
//     ExecuteResult r2 = run_sql(database,
//         "INSERT INTO users VALUES (1,\"alice@x.com\",100)");
//     ASSERT(r2 == EXECUTE_SUCCESS, "insert into recreated table must succeed");
//     ASSERT(count_rows(database, "users") == 1, "must have 1 row");

//     database->db_close(); delete database; cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  Persistence tests — close + reopen
// // ─────────────────────────────────────────────────────────────────────────────

// static void test_persist_rows_survive_reopen() {
//     const std::string DB = "tsql_pers1";
//     cleanup(DB);

//     {
//         db* database = db::db_open(DB.c_str());
//         run_sql(database, "CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)");
//         for (int i = 1; i <= 50; i++)
//             run_sql(database, "INSERT INTO users VALUES (" + std::to_string(i) +
//                 ",\"user" + std::to_string(i) + "\"," + std::to_string(20+i) + ")");
//         database->db_close(); delete database;
//     }

//     {
//         db* database = db::db_open(DB.c_str());
//         ASSERT(count_rows(database, "users") == 50,
//                "50 rows must survive close+reopen");
//         for (int i = 1; i <= 50; i++)
//             ASSERT(key_exists(database, "users", i),
//                    "key " + std::to_string(i) + " must survive reopen");
//         database->db_close(); delete database;
//     }

//     cleanup(DB);
// }

// static void test_persist_schema_survives_reopen() {
//     const std::string DB = "tsql_pers2";
//     cleanup(DB);

//     {
//         db* database = db::db_open(DB.c_str());
//         run_sql(database, "CREATE TABLE products (id INT PRIMARY KEY, name TEXT, price INT, stock INT)");
//         database->db_close(); delete database;
//     }

//     {
//         db* database = db::db_open(DB.c_str());
//         Table* t = database->getTable("products");
//         ASSERT(t != nullptr, "table must exist after reopen");
//         ASSERT(t->schema.columns.size() == 4, "schema must have 4 columns after reopen");
//         database->db_close(); delete database;
//     }

//     cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  Mixed stress test — create, insert, select, update, drop all together
// // ─────────────────────────────────────────────────────────────────────────────

// static void test_mixed_stress() {
//     const std::string DB = "tsql_mix1";
//     cleanup(DB);
//     db* database = db::db_open(DB.c_str());

//     // Create two tables
//     run_sql(database,
//         "CREATE TABLE employees (id INT PRIMARY KEY, name TEXT, dept INT, salary INT)");
//     run_sql(database,
//         "CREATE TABLE departments (id INT PRIMARY KEY, dname TEXT, budget INT)");

//     // Populate departments
//     run_sql(database, "INSERT INTO departments VALUES (1,\"Engineering\",500000)");
//     run_sql(database, "INSERT INTO departments VALUES (2,\"Marketing\",300000)");
//     run_sql(database, "INSERT INTO departments VALUES (3,\"HR\",200000)");

//     // Populate employees — 200 rows
//     std::mt19937 rng(42);
//     std::uniform_int_distribution<int> dept_dist(1, 3);
//     std::uniform_int_distribution<int> sal_dist(40000, 120000);

//     for (int i = 1; i <= 200; i++) {
//         run_sql(database,
//             "INSERT INTO employees VALUES (" +
//             std::to_string(i) + ",\"emp" + std::to_string(i) + "\"," +
//             std::to_string(dept_dist(rng)) + "," +
//             std::to_string(sal_dist(rng)) + ")");
//     }

//     ASSERT(count_rows(database, "employees") == 200, "must have 200 employees");

//     // Try duplicate inserts — all must fail
//     for (int i = 1; i <= 10; i++) {
//         ExecuteResult r = run_sql(database,
//             "INSERT INTO employees VALUES (" + std::to_string(i) +
//             ",\"dup\",1,99999)");
//         ASSERT(r == EXECUTE_DUPLICATE_KEY,
//                "duplicate emp id=" + std::to_string(i) + " must fail");
//     }
//     ASSERT(count_rows(database, "employees") == 200,
//            "row count must stay 200 after failed duplicates");

//     // Updates
//     for (int i = 1; i <= 50; i++) {
//         run_sql(database,
//             "UPDATE employees SET salary = 100000 WHERE id = " + std::to_string(i));
//     }

//     // Selects with WHERE
//     ExecuteResult r1 = run_sql(database,
//         "SELECT name,salary FROM employees WHERE dept = 1");
//     ASSERT(r1 == EXECUTE_SUCCESS, "SELECT WHERE dept=1 must succeed");

//     ExecuteResult r2 = run_sql(database,
//         "SELECT * FROM employees WHERE salary > 100000 ORDER BY salary DESC");
//     ASSERT(r2 == EXECUTE_SUCCESS, "SELECT WHERE + ORDER BY must succeed");

//     // Drop one table and verify the other still works
//     run_sql(database, "DROP TABLE departments");
//     ASSERT(database->getTable("departments") == nullptr,
//            "departments must be gone after DROP");
//     ASSERT(count_rows(database, "employees") == 200,
//            "employees must be unaffected by DROP of other table");

//     // SELECT on dropped table must fail cleanly
//     ExecuteResult r3 = run_sql(database, "SELECT * FROM departments");
//     ASSERT(r3 == EXECUTE_TABLE_NOT_FOUND,
//            "SELECT on dropped table must return TABLE_NOT_FOUND");

//     database->db_close(); delete database; cleanup(DB);
// }

// // ─────────────────────────────────────────────────────────────────────────────
// //  main
// // ─────────────────────────────────────────────────────────────────────────────

// int main() {
//     std::cout << "\n" CYN "=== BitBase SQL Stress Tests ===" RST "\n\n";

//     // CREATE TABLE
//     std::cout << CYN "── CREATE TABLE ──" RST "\n";
//     RUN_TEST(test_create_table_basic);
//     RUN_TEST(test_create_table_duplicate);
//     RUN_TEST(test_create_table_no_primary_key);
//     RUN_TEST(test_create_table_multiple_pk);
//     RUN_TEST(test_create_table_text_primary_key);
//     RUN_TEST(test_create_many_tables);

//     // INSERT
//     std::cout << "\n" CYN "── INSERT ──" RST "\n";
//     RUN_TEST(test_insert_single_row);
//     RUN_TEST(test_insert_multi_row);
//     RUN_TEST(test_insert_duplicate_key);
//     RUN_TEST(test_insert_multi_row_with_duplicate_rolls_back);
//     RUN_TEST(test_insert_wrong_column_count);
//     RUN_TEST(test_insert_table_not_found);
//     RUN_TEST(test_insert_stress_500_rows);
//     RUN_TEST(test_insert_negative_id);

//     // SELECT
//     std::cout << "\n" CYN "── SELECT ──" RST "\n";
//     RUN_TEST(test_select_star);
//     RUN_TEST(test_select_specific_columns);
//     RUN_TEST(test_select_invalid_column);
//     RUN_TEST(test_select_table_not_found);
//     RUN_TEST(test_select_where_eq);
//     RUN_TEST(test_select_where_gt);
//     RUN_TEST(test_select_where_invalid_col);
//     RUN_TEST(test_select_order_by_asc);
//     RUN_TEST(test_select_order_by_desc);
//     RUN_TEST(test_select_order_by_invalid_col);
//     RUN_TEST(test_select_where_and_order_by);
//     RUN_TEST(test_select_empty_table);

//     // UPDATE
//     std::cout << "\n" CYN "── UPDATE ──" RST "\n";
//     RUN_TEST(test_update_single_col);
//     RUN_TEST(test_update_multiple_cols);
//     RUN_TEST(test_update_no_where_updates_all);
//     RUN_TEST(test_update_nonexistent_key);
//     RUN_TEST(test_update_invalid_column);
//     RUN_TEST(test_update_table_not_found);
//     RUN_TEST(test_update_stress);

//     // DROP TABLE
//     std::cout << "\n" CYN "── DROP TABLE ──" RST "\n";
//     RUN_TEST(test_drop_table_basic);
//     RUN_TEST(test_drop_table_not_found);
//     RUN_TEST(test_drop_then_recreate);

//     // PERSISTENCE
//     std::cout << "\n" CYN "── PERSISTENCE ──" RST "\n";
//     RUN_TEST(test_persist_rows_survive_reopen);
//     RUN_TEST(test_persist_schema_survives_reopen);

//     // MIXED
//     std::cout << "\n" CYN "── MIXED STRESS ──" RST "\n";
//     RUN_TEST(test_mixed_stress);

//     std::cout << "\n─────────────────────────────────────────\n";
//     std::cout << "Results: "
//               << GRN << tests_passed << " passed" RST "  "
//               << (tests_failed ? RED : "") << tests_failed << " failed" RST
//               << "  / " << tests_run << " total\n\n";

//     return tests_failed == 0 ? 0 : 1;
// }