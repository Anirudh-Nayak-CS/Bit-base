

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include "headers/tokenizer/tokenizer.h"
#include "headers/parser/parser.h"
#include "headers/vm/vm.h"
#include "headers/storage/database/db.h"
#include "headers/statement/statement.h"
#include "headers/constants/constant.h"

static const std::string BENCH_DB = "bench_tmp_db";


static void cleanupDb() {
    std::string cmd = "rm -rf " + BENCH_DB + " " + BENCH_DB + ".schema " + BENCH_DB + ".wal 2>/dev/null";
    std::system(cmd.c_str());
}


static bool execSQL(db* database, const std::string& sql) {
    auto tokens = tokenize(sql);
    Statement stmt;
    Parser parser;
    if (parser.parse(tokens, stmt) != CMD_SUCCESS) return false;

    VM vm(database);
    return vm.execute(stmt) == EXECUTE_SUCCESS;
}

class ScopedOutputSilencer {
public:
    ScopedOutputSilencer()
        : cout_old_(std::cout.rdbuf(nullptr)),
          cerr_old_(std::cerr.rdbuf(nullptr)) {}

    ~ScopedOutputSilencer() {
        std::cout.rdbuf(cout_old_);
        std::cerr.rdbuf(cerr_old_);
    }

private:
    std::streambuf* cout_old_;
    std::streambuf* cerr_old_;
};

struct BenchResult {
    std::string label;
    int n;
    double total_ms;
    double per_op_us;
};


static BenchResult measure(const std::string& label, int n, std::function<void()> fn) {
    auto t0 = std::chrono::high_resolution_clock::now();
    {
        ScopedOutputSilencer quiet;
        fn();
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return {label, n, ms, (ms * 1000.0) / n};
}


static void printHeader() {
    std::cout << std::left  << std::setw(34) << "Operation"
              << std::right << std::setw(8)  << "N"
              << std::right << std::setw(12) << "Total (ms)"
              << std::right << std::setw(14) << "Per-op (µs)\n";
    std::cout << std::string(72, '-') << "\n";
}


static void printRow(const BenchResult& r) {
    std::cout << std::left  << std::setw(34) << r.label
              << std::right << std::setw(8)  << r.n
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << r.total_ms
              << std::right << std::setw(14) << std::fixed << std::setprecision(2) << r.per_op_us << "\n";
}


static void printGraph(const std::vector<BenchResult>& results) {
    if (results.empty()) return;

    double max_ms = 0;
    for (const auto& r : results) max_ms = std::max(max_ms, r.total_ms);

    std::cout << "\nPerformance Graph (Total time in ms):\n";
    for (const auto& r : results) {
        int bar_len = static_cast<int>((r.total_ms / max_ms) * 60.0);
        if (bar_len < 1) bar_len = 1;
        std::cout << std::left << std::setw(32) << r.label.substr(0, 31)
                  << " |" << std::string(bar_len, '#')
                  << " " << std::fixed << std::setprecision(1) << r.total_ms << " ms\n";
    }
    std::cout << "\n";
}


static void exportToCSV(const std::vector<std::vector<BenchResult>>& allResults) {
    std::ofstream csv("benchmark_results.csv");
    if (!csv.is_open()) {
        std::cerr << "Warning: Could not write benchmark_results.csv\n";
        return;
    }

    csv << "N,Operation,Total_ms,Per_op_us\n";
    for (const auto& suite : allResults) {
        for (const auto& r : suite) {
            csv << r.n << "," 
                << "\"" << r.label << "\"," 
                << r.total_ms << "," 
                << r.per_op_us << "\n";
        }
    }
    csv.close();
    std::cout << "✓ Results saved to benchmark_results.csv\n";
}


static std::string nameFor(int i) {
    static const char* names[] = {"Alice", "Bob", "Carol", "Dave", "Eve", "Frank"};
    std::ostringstream oss;
    oss << "'" << names[i % 6] << i << "'";
    return oss.str();
}


static std::vector<BenchResult> runSuite(int N) {
    std::vector<BenchResult> results;
    cleanupDb();
    db* database = db::db_open(BENCH_DB.c_str());


    execSQL(database, "CREATE TABLE bench (id INT PRIMARY KEY, name TEXT, score INT)");


    results.push_back(measure("INSERT (sequential)", N, [&]() {
        for (int i = 1; i <= N; ++i) {
            std::ostringstream sql;
            sql << "INSERT INTO bench VALUES (" << i << ", " << nameFor(i) << ", " << (i * 3 % 1000) << ")";
            execSQL(database, sql.str());
        }
    }));

   
    results.push_back(measure("SELECT * (full scan)", N, [&]() {
        for (int i = 0; i < N; ++i) {
            execSQL(database, "SELECT * FROM bench");
        }
    }));


    results.push_back(measure("SELECT WHERE id= (point lookup)", N, [&]() {
        for (int i = 1; i <= N; ++i) {
            std::ostringstream sql;
            sql << "SELECT * FROM bench WHERE id = " << i;
            execSQL(database, sql.str());
        }
    }));

 
    results.push_back(measure("UPDATE WHERE id= (point)", N, [&]() {
        for (int i = 1; i <= N; ++i) {
            std::ostringstream sql;
            sql << "UPDATE bench SET score = " << (i * 7 % 1000) << " WHERE id = " << i;
            execSQL(database, sql.str());
        }
    }));


    results.push_back(measure("DELETE WHERE id= (point)", N, [&]() {
    for (int i = 1; i <= N; ++i) {
        std::ostringstream sql;
        sql << "DELETE FROM bench WHERE id = " << i;
        execSQL(database, sql.str());
    }
    }));


    execSQL(database, "CREATE TABLE bench_rev (id INT PRIMARY KEY, val TEXT)");
    results.push_back(measure("INSERT reverse order (splits)", N, [&]() {
        for (int i = N; i >= 1; --i) {
            std::ostringstream sql;
            sql << "INSERT INTO bench_rev VALUES (" << i << ", 'val" << i << "')";
            execSQL(database, sql.str());
        }
    }));

    database->db_close();
    cleanupDb();
    return results;
}

int main(int argc, char* argv[]) {
    std::vector<int> sizes = {100, 1000, 5000, 10000};
    if (argc >= 2) {
        sizes.clear();
        for (int i = 1; i < argc; ++i) {
            int n = std::atoi(argv[i]);
            if (n > 0) sizes.push_back(n);
        }
    }

    

    std::vector<std::vector<BenchResult>> allResults;

    for (int N : sizes) {
        std::cout << "══════════ N = " << N << " ══════════\n";
        printHeader();

        // Suppress B+tree debug logs
        std::streambuf* cerr_old = std::cerr.rdbuf(nullptr);
        auto results = runSuite(N);
        std::cerr.rdbuf(cerr_old);

        for (const auto& r : results)
            printRow(r);

        printGraph(results);
        allResults.push_back(results);
    }

    // CSV Export
    exportToCSV(allResults);

    std::cout << "\nBenchmark completed!\n";
    std::cout << "→ Results saved to: benchmark_results.csv\n";
    

    return 0;
}
