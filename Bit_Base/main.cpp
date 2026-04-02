#include <iostream>
#include <string>
#include "headers/tokenizer/tokenizer.h"
#include "headers/parser/parser.h"
#include "headers/vm/vm.h"
#include "headers/storage/database/db.h"
#include "headers/statement/statement.h"
#include "headers/constants/constant.h"

static void printPrompt() {
    std::cout << "bitbase> ";
}

static bool handleMeta(const std::string& input, db* database) {
    if (input == ".exit") {
        database->db_close();
        std::cout << "Bye.\n";
        exit(0);
    }
    if (input == ".tables") {
        for (auto& [name, _] : database->tables)
            std::cout << "  " << name << "\n";
        return true;
    }
    if (input == ".help") {
        std::cout <<
            "\nMeta commands:\n\n"
            "  .exit                                        exit\n"
            "  .tables                                      list tables\n"
            "  .help                                        this message\n"
            "\n\nSQL:\n\n"
            "  INSERT INTO <table> VALUES (<v1>, <v2>, ...), (<v1>, <v2>, ...)\n"
            "  SELECT * FROM <table>\n"
            "  SELECT col1,col2 FROM <table> WHERE condition\n"
            "  SELECT col1,col2 FROM <table> ORDER BY  <col> DESC/ASC\n"
            "  UPDATE <table>  SET col1=val1 [, col2=val2 ...]  WHERE <Primary_key>=<col_no>\n"
            "  CREATE TABLE <name> (<col1> <TYPE1> [PRIMARY KEY], <col2> <TYPE2>, ...)\n"
            "  DROP TABLE <name>\n";
        return true;
    }
    std::cout << "Unknown meta command: " << input << "\n";
    return false;
}

int main(int argc, char* argv[]) {
    const char* dbname = (argc >= 2) ? argv[1] : "bitbase_db";
    db* database = db::db_open(dbname);

    std::cout << "BitBase   (db: " << dbname << ")\n";
    std::cout << "Type .help for commands.\n\n";

    std::string line;
    while (true) {
        printPrompt();
        if (!std::getline(std::cin, line)) break;

        // trim leading/trailing whitespace
        size_t s = line.find_first_not_of(" \t\r\n");
        if (s == std::string::npos) continue;
        line = line.substr(s);
        size_t e = line.find_last_not_of(" \t\r\n");
        if (e != std::string::npos) line = line.substr(0, e + 1);

        if (line.empty()) continue;

        // meta commands
        if (line[0] == '.') {
            handleMeta(line, database);
            continue;
        }

        // tokenize
        auto tokens = tokenize(line);

        // parse
        Statement out;
        Parser parser;
        Commandstatus result = parser.parse(tokens, out);
        if (result != CMD_SUCCESS) {
            std::cout << "Parse error\n";
            continue;
        }

        // execute
        VM vm(database);
        ExecuteResult exec_result = vm.execute(out);

        if (exec_result != EXECUTE_SUCCESS) {
            std::cout << VM::resultMessage(exec_result) << "\n";
        } else if (out.type == INSERT) {
            size_t count = out.multi_insert_rows.empty() ? 1 : out.multi_insert_rows.size();
            std::cout << count << " row" << (count == 1 ? "" : "s") << " inserted.\n";
        } else if (out.type == UPDATE) {
            std::cout << "Row(s) updated.\n";
        }
    }

    database->db_close();
    return 0;
}