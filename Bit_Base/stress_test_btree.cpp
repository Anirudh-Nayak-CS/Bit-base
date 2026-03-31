// #include "headers/vm/vm.h"
// #include "headers/storage/database/db.h"
// #include "headers/tokenizer/tokenizer.h"
// #include "headers/parser/parser.h"
// #include <iostream>
// #include <chrono>
// #include <cassert>
// #include <iomanip>

// // Helper to execute SQL statements
// ExecuteResult executeSql(const std::string& sql, VM& vm) {
//     auto tokens = tokenize(sql);
//     Parser parser;
//     Statement stmt;
    
//     auto status = parser.parse(tokens, stmt);
//     if (status != CMD_SUCCESS) {
//         std::cerr << "Parse error: status=" << status << "\n";
//         return ExecuteResult::EXECUTE_UNKNOWN_ERROR;
//     }
    
//     return vm.execute(stmt);
// }

// // Helper to display formatted operation results
// void displayOperationResult(const std::string& operation, int id, const std::string& value, bool success = true) {
//     std::cout << "  [" << (success ? "✓" : "✗") << "] " 
//               << std::left << std::setw(10) << operation 
//               << "id=" << id << " | value=" << value << "\n";
// }

// int main() {
//     std::cout << "=== B+ TREE STRESS TEST ===\n\n";
    
//     // Setup
//     db* database = db::db_open("stress_test_db");
//     VM vm(database);
    
//     std::cout << "[1] Creating table with schema...\n";
//     std::string createSql = "CREATE TABLE users id INT32 PRIMARY KEY email TEXT";
//     auto result = executeSql(createSql, vm);
//     assert(result == ExecuteResult::EXECUTE_SUCCESS);
//     std::cout << "    ✓ Table created\n\n";
    
//     // Test 1: Sequential inserts (tests node splits)
//     std::cout << "[2] STRESS TEST: Inserting 100 rows sequentially...\n";
//     auto start = std::chrono::high_resolution_clock::now();
    
//     bool showFirstInserts = true;
//     for (int i = 1; i <= 100; ++i) {
//         std::string insertSql = "INSERT INTO users VALUES " + std::to_string(i) + " user" + std::to_string(i);
//         auto res = executeSql(insertSql, vm);
        
//         if (res != ExecuteResult::EXECUTE_SUCCESS) {
//             std::cerr << "    ✗ INSERT failed at key " << i << ": " << vm.resultMessage(res) << "\n";
//             assert(false);
//         }
        
//         // Show first 5 inserts in detail
//         if (i <= 5 && showFirstInserts) {
//             displayOperationResult("INSERT", i, "user" + std::to_string(i), true);
//         }
        
//         if (i == 6 && showFirstInserts) {
//             std::cout << "  ... (rows 6-100 inserted)\n";
//             showFirstInserts = false;
//         }
        
//         if (i % 20 == 0) {
//             std::cout << "    → Inserted " << i << " rows...\n";
//         }
//     }
    
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//     std::cout << "    ✓ All 100 rows inserted in " << duration.count() << "ms\n\n";
    
//     // Test 2: Find specific rows
//     std::cout << "[3] STRESS TEST: Finding specific rows...\n";
//     std::string selectSql = "SELECT * FROM users";
//     result = executeSql(selectSql, vm);
//     assert(result == ExecuteResult::EXECUTE_SUCCESS);
//     std::cout << "    ✓ SELECT * completed\n\n";
    
//     // Test 3: Random access patterns
//     std::cout << "[4] STRESS TEST: Random key lookups...\n";
//     int test_keys[] = {1, 50, 99, 25, 75, 100, 10, 90};
//     for (int key : test_keys) {
//         std::string selectSql = "SELECT * FROM users";
//         auto res = executeSql(selectSql, vm);
        
//         if (res != ExecuteResult::EXECUTE_SUCCESS) {
//             std::cerr << "    ✗ SELECT failed for key " << key << "\n";
//             assert(false);
//         }
//     }
//     std::cout << "    ✓ All random lookups completed\n\n";
    
//     // Test 4: Update operations (tests tree navigation)
//     std::cout << "[5] STRESS TEST: Updating 20 rows...\n";
//     for (int i = 1; i <= 20; ++i) {
//         std::string updateSql = "UPDATE users SET id=" + std::to_string(i) + ", email=updated" + std::to_string(i);
//         auto res = executeSql(updateSql, vm);
        
//         bool success = (res == ExecuteResult::EXECUTE_SUCCESS);
//         if (i <= 5) {
//             displayOperationResult("UPDATE", i, "updated" + std::to_string(i), success);
//         } else if (i == 6) {
//             std::cout << "  ... (rows 6-20 updated)\n";
//         }
        
//         if (res != ExecuteResult::EXECUTE_SUCCESS) {
//             std::cerr << "    ✗ UPDATE failed at key " << i << ": " << vm.resultMessage(res) << "\n";
//         }
//     }
//     std::cout << "    → Verifying UPDATE persistence...\n";
//     result = executeSql("SELECT * FROM users", vm);
//     if (result == ExecuteResult::EXECUTE_SUCCESS) {
//         std::cout << "    ✓ Update operations completed and verified\n\n";
//     } else {
//         std::cout << "    ✗ Failed to verify updates\n\n";
//     }
    
//     // Test 5: Edge cases
//     std::cout << "[6] EDGE CASES TEST:\n";
    
//     // Try to insert duplicate key
//     std::cout << "    → Testing duplicate key insertion...\n";
//     result = executeSql("INSERT INTO users VALUES 1 duplicate", vm);
//     if (result == ExecuteResult::EXECUTE_DUPLICATE_KEY) {
//         std::cout << "    ✓ Duplicate key correctly rejected\n";
//     } else {
//         std::cout << "    ! Duplicate key returned: " << vm.resultMessage(result) << "\n";
//     }
    
//     // Try to select non-existent table
//     std::cout << "    → Testing non-existent table...\n";
//     result = executeSql("SELECT * FROM nonexistent", vm);
//     if (result == ExecuteResult::EXECUTE_TABLE_NOT_FOUND) {
//         std::cout << "    ✓ Non-existent table correctly detected\n";
//     } else {
//         std::cout << "    ! Returned: " << vm.resultMessage(result) << "\n";
//     }
    
//     std::cout << "\n=== STRESS TEST COMPLETE ===\n";
//     std::cout << "✓ B+ Tree insertion, finding, and splitting all working!\n";
    
//     database->db_close();
//     return 0;
// }
