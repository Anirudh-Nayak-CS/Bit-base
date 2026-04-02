#include "../../headers/parser/parser.h"

#include <cctype>
#include <cstring>

bool Parser::ieq(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
            return false;
    return true;
}

bool Parser::match(const std::vector<Token>& t, size_t i, TokenType type,
                   const std::string& value) const {
    if (i >= t.size()) return false;
    if (t[i].type != type) return false;
    if (!value.empty() && !ieq(t[i].value, value)) return false;
    return true;
}

Commandstatus Parser::parse(const std::vector<Token>& tokens, Statement& out) {
    if (tokens.empty() || tokens[0].type == TokenType::END)
        return CMD_SYNTAX_ERROR;
    if (tokens[0].type != TokenType::KEYWORD)
        return CMD_SYNTAX_ERROR;

    const std::string& first = tokens[0].value;

    if      (ieq(first, "INSERT")) return parseInsert(tokens, out);
    else if (ieq(first, "SELECT")) return parseSelect(tokens, out);
    else if (ieq(first, "UPDATE")) return parseUpdate(tokens, out);
    else if (ieq(first, "CREATE")) return parseCreateTable(tokens, out);
    else if (ieq(first, "DROP"))   return parseDropTable(tokens, out);

    return CMD_SYNTAX_ERROR;
}

// INSERT INTO <table> VALUES (<v1>, <v2>, ...), (<v1>, <v2>, ...), ...

Commandstatus Parser::parseInsert(const std::vector<Token>& t, Statement& out) {
    // INSERT INTO table VALUES ( ...
    if (t.size() < 6) return CMD_SYNTAX_ERROR;

    if (!match(t, 1, TokenType::KEYWORD,    "INTO"))   return CMD_SYNTAX_ERROR;
    if (!match(t, 2, TokenType::IDENTIFIER))            return CMD_SYNTAX_ERROR;
    if (!match(t, 3, TokenType::KEYWORD,    "VALUES"))  return CMD_SYNTAX_ERROR;

    out.type       = INSERT;
    out.table_name = t[2].value;
    out.insert_values.clear();
    out.multi_insert_rows.clear();

    size_t i = 4;

    // parse one or more row tuples: ( v1, v2, ... ) [, ( ... )]
    while (i < t.size() && t[i].type != TokenType::END) {
        if (!match(t, i, TokenType::PUNCTUATION, "(")) return CMD_SYNTAX_ERROR;
        i++; // skip (

        std::vector<std::string> row_vals;

        while (i < t.size()) {
            if (match(t, i, TokenType::PUNCTUATION, ")")) { i++; break; }

            // value
            if (t[i].type != TokenType::NUMBER &&
                t[i].type != TokenType::STRING  &&
                t[i].type != TokenType::IDENTIFIER)
                return CMD_SYNTAX_ERROR;

            row_vals.push_back(t[i].value);
            i++;

            // optional comma between values
            if (match(t, i, TokenType::PUNCTUATION, ",")) i++;
        }

        if (row_vals.empty()) return CMD_SYNTAX_ERROR;
        out.multi_insert_rows.push_back(row_vals);

        // optional comma between row tuples
        if (match(t, i, TokenType::PUNCTUATION, ",")) i++;
    }

    if (out.multi_insert_rows.empty()) return CMD_SYNTAX_ERROR;

    // for single-row insert keep insert_values populated for VM compatibility
    if (out.multi_insert_rows.size() == 1)
        out.insert_values = out.multi_insert_rows[0];

    return CMD_SUCCESS;
}

// SELECT * FROM <table>
// SELECT COL1,COL2 FROM <table> WHERE ... 
// SELECT COL1,COL2 FROM <table> ORDER BY  <param> <DESC/ASC>

Commandstatus Parser::parseSelect(const std::vector<Token>& t, Statement& out) {
    if (t.size() < 4) return CMD_SYNTAX_ERROR;

    out.type = SELECT;
    out.select_cols.clear();
    out.where    = {};
    out.order_by = {};

    size_t i = 1;

    // Column list or *
    if (match(t, i, TokenType::IDENTIFIER, "*")) {
        i++;
    } else {
        while (i < t.size() && !match(t, i, TokenType::KEYWORD, "FROM")) {
            if (t[i].type == TokenType::END)        return CMD_SYNTAX_ERROR;
            if (t[i].type != TokenType::IDENTIFIER) return CMD_SYNTAX_ERROR;
            out.select_cols.push_back(t[i].value);
            i++;
            if (match(t, i, TokenType::PUNCTUATION, ",")) i++;
        }
    }

    // FROM <table>
    if (!match(t, i, TokenType::KEYWORD, "FROM")) return CMD_SYNTAX_ERROR;
    i++;
    if (!match(t, i, TokenType::IDENTIFIER)) return CMD_SYNTAX_ERROR;
    out.table_name = t[i].value;
    i++;

    //  WHERE <col> <op> <value>
    if (i < t.size() && match(t, i, TokenType::KEYWORD, "WHERE")) {
        i++;
        if (!match(t, i, TokenType::IDENTIFIER)) return CMD_SYNTAX_ERROR;
        out.where.col = t[i].value;
        i++;

        // operator: = < > <= >=
        if (i >= t.size()) return CMD_SYNTAX_ERROR;
        std::string op = t[i].value;
        if (op != "=" && op != "<" && op != ">" && op != "<=" && op != ">=")
            return CMD_SYNTAX_ERROR;
        out.where.op = op;
        i++;

        if (i >= t.size()) return CMD_SYNTAX_ERROR;
        if (t[i].type != TokenType::NUMBER   &&
            t[i].type != TokenType::STRING   &&
            t[i].type != TokenType::IDENTIFIER)
            return CMD_SYNTAX_ERROR;
        out.where.value  = t[i].value;
        out.where.active = true;
        i++;
    }

    //  ORDER BY <col> [ASC|DESC]
    if (i < t.size() && match(t, i, TokenType::KEYWORD, "ORDER")) {
        i++;
        if (!match(t, i, TokenType::KEYWORD, "BY") &&
            !match(t, i, TokenType::IDENTIFIER, "BY"))
            return CMD_SYNTAX_ERROR;
        i++;

        if (!match(t, i, TokenType::IDENTIFIER)) return CMD_SYNTAX_ERROR;
        out.order_by.col    = t[i].value;
        out.order_by.active = true;
        i++;

     
        if (i < t.size() && t[i].type == TokenType::IDENTIFIER) {
            std::string dir = t[i].value;
            for (char& c : dir) c = std::toupper((unsigned char)c);
            if (dir == "DESC") { out.order_by.descending = true;  i++; }
            else if (dir == "ASC")  {                              i++; }
        }
    }

    return CMD_SUCCESS;
}



// UPDATE <table>  SET col1=val1 [, col2=val2 ...]  WHERE <Primary_key>=<col_no>

Commandstatus Parser::parseUpdate(const std::vector<Token>& t, Statement& out) {
    if (t.size() < 6) return CMD_SYNTAX_ERROR;

    if (!match(t, 1, TokenType::IDENTIFIER))     return CMD_SYNTAX_ERROR;
    if (!match(t, 2, TokenType::KEYWORD, "SET")) return CMD_SYNTAX_ERROR;

    out.type       = UPDATE;
    out.table_name = t[1].value;
    out.assignments.clear();
    out.where = {};

    size_t i = 3;

    // SET col1=val1 [, col2=val2 ...]  — stop at WHERE or END
    while (i < t.size() && t[i].type != TokenType::END) {
        if (match(t, i, TokenType::KEYWORD, "WHERE")) break;

        if (!match(t, i, TokenType::IDENTIFIER)) return CMD_SYNTAX_ERROR;
        std::string col = t[i].value;
        i++;

        if (i >= t.size() || t[i].value != "=") return CMD_SYNTAX_ERROR;
        i++;

        if (i >= t.size()) return CMD_SYNTAX_ERROR;
        if (t[i].type != TokenType::STRING   &&
            t[i].type != TokenType::NUMBER   &&
            t[i].type != TokenType::IDENTIFIER)
            return CMD_SYNTAX_ERROR;

        out.assignments.push_back({col, t[i].value});
        i++;

        if (match(t, i, TokenType::PUNCTUATION, ",")) i++;
    }

    if (out.assignments.empty()) return CMD_SYNTAX_ERROR;

    // Optional WHERE <col> <op> <value>
    if (i < t.size() && match(t, i, TokenType::KEYWORD, "WHERE")) {
        i++;
        if (!match(t, i, TokenType::IDENTIFIER)) return CMD_SYNTAX_ERROR;
        out.where.col = t[i].value;
        i++;

        if (i >= t.size()) return CMD_SYNTAX_ERROR;
        std::string op = t[i].value;
        if (op != "=" && op != "<" && op != ">" && op != "<=" && op != ">=")
            return CMD_SYNTAX_ERROR;
        out.where.op = op;
        i++;

        if (i >= t.size()) return CMD_SYNTAX_ERROR;
        out.where.value  = t[i].value;
        out.where.active = true;
    }

    return CMD_SUCCESS;
}

// CREATE TABLE <name> (<col1> <TYPE1> [PRIMARY KEY], <col2> <TYPE2>, ...)

Commandstatus Parser::parseCreateTable(const std::vector<Token>& t, Statement& out) {
    // CREATE TABLE name ( col TYPE [PRIMARY KEY], ... )
    if (t.size() < 6) return CMD_SYNTAX_ERROR;

    if (!match(t, 1, TokenType::KEYWORD,    "TABLE")) return CMD_SYNTAX_ERROR;
    if (!match(t, 2, TokenType::IDENTIFIER))           return CMD_SYNTAX_ERROR;
    if (!match(t, 3, TokenType::PUNCTUATION, "("))     return CMD_SYNTAX_ERROR;

    out.type       = CREATE_TABLE;
    out.table_name = t[2].value;
    out.columns.clear();

    size_t i = 4;

    while (i < t.size()) {
        if (match(t, i, TokenType::PUNCTUATION, ")")) { i++; break; }
        if (t[i].type == TokenType::END) return CMD_SYNTAX_ERROR;

        // column name
        if (t[i].type != TokenType::IDENTIFIER) return CMD_SYNTAX_ERROR;
        std::string col_name = t[i].value;
        i++;

        if (i >= t.size()) return CMD_SYNTAX_ERROR;

        // type
        if (t[i].type != TokenType::KEYWORD && t[i].type != TokenType::IDENTIFIER)
            return CMD_SYNTAX_ERROR;

        std::string type_str = t[i].value;
        for (char& c : type_str) c = std::toupper((unsigned char)c);

        DataType dtype;
        if (!parseDataType(type_str, dtype)) return CMD_SYNTAX_ERROR;
        i++;

        // optional PRIMARY KEY
        bool is_pk = false;
        if (match(t, i, TokenType::KEYWORD, "PRIMARY") &&
            match(t, i+1, TokenType::KEYWORD, "KEY")) {
            is_pk = true;
            i += 2;
        }

        out.columns.push_back({col_name, dtype, is_pk});

        // optional comma between column definitions
        if (match(t, i, TokenType::PUNCTUATION, ",")) i++;
    }

    if (out.columns.empty()) return CMD_SYNTAX_ERROR;
    return CMD_SUCCESS;
}

// DROP TABLE <name>

Commandstatus Parser::parseDropTable(const std::vector<Token>& t, Statement& out) {
    if (t.size() < 3) return CMD_SYNTAX_ERROR;

    if (!match(t, 1, TokenType::KEYWORD,    "TABLE")) return CMD_SYNTAX_ERROR;
    if (!match(t, 2, TokenType::IDENTIFIER))           return CMD_SYNTAX_ERROR;

    out.type       = DROP_TABLE;
    out.table_name = t[2].value;
    return CMD_SUCCESS;
}