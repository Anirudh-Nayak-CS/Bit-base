#include "../../headers/vm/vm.h"
#include "../../headers/constants/constant.h"
#include "../../headers/storage/node/leaf_node.h"
#include "../../headers/schema/schema.h"
#include "../../headers/storage/row/row_serialize.h"
#include <iostream>
#include <cstring>

ExecuteResult VM::execute(const Statement& stmt) {
    switch (stmt.type) {
        case INSERT:       return executeInsert(stmt);
        case SELECT:       return executeSelect(stmt);
        case UPDATE:       return executeUpdate(stmt);
        case CREATE_TABLE: return executeCreateTable(stmt);
        case DROP_TABLE:   return executeDropTable(stmt);
        default:           return ExecuteResult::EXECUTE_UNKNOWN_ERROR;
    }
}


//  INSERT

// helper: insert a single row of raw string values into the table
static ExecuteResult insertOneRow(Table* table,
                                  const Schema& schema,
                                  const std::vector<std::string>& values) {
    if (values.size() != schema.columns.size())
        return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;

    Row row;
    try {
        int pk_idx = schema.primaryKeyIndex();
        if (pk_idx == -1) return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;

        row.id = static_cast<uint32_t>(std::stoi(values[pk_idx]));

        for (size_t i = 0; i < schema.columns.size(); ++i)
            row.fields.push_back(parseValue(values[i], schema.columns[i].type));
    } catch (...) {
        return ExecuteResult::EXECUTE_TYPE_ERROR;
    }

    auto bytes = serializeRow(row);
    if (bytes.size() > LEAF_NODE_VALUE_SIZE)
        return ExecuteResult::EXECUTE_ROW_TOO_LARGE;

    table->insert(row.id, bytes.data(), static_cast<uint32_t>(bytes.size()));
    return ExecuteResult::EXECUTE_SUCCESS;
}

ExecuteResult VM::executeInsert(const Statement& stmt) {
    Table* table = db_->getTable(stmt.table_name);
    if (!table) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

    const Schema& schema = table->schema;

    // multi-row path (parser always populates multi_insert_rows)
    if (!stmt.multi_insert_rows.empty()) {
        for (const auto& row_vals : stmt.multi_insert_rows) {
            ExecuteResult r = insertOneRow(table, schema, row_vals);
            if (r != ExecuteResult::EXECUTE_SUCCESS) return r;
        }
        return ExecuteResult::EXECUTE_SUCCESS;
    }

    // fallback  single-row path via insert_values
    return insertOneRow(table, schema, stmt.insert_values);
}


//  SELECT

ExecuteResult VM::executeSelect(const Statement& stmt) {
    Table* table = db_->getTable(stmt.table_name);
    if (!table) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

    const Schema& schema = table->schema;

    for (const auto& col : stmt.select_cols)
        if (schema.indexOf(col) == -1) return ExecuteResult::EXECUTE_COLUMN_NOT_FOUND;

    // print header
    if (stmt.select_cols.empty()) {
        for (const auto& col : schema.columns) std::cout << col.name << "\t";
    } else {
        for (const auto& col : stmt.select_cols) std::cout << col << "\t";
    }
    std::cout << "\n" << std::string(40, '-') << "\n";

    std::unique_ptr<Cursor> cursor = Cursor::table_start(table);

    while (!cursor->end_of_table) {
        void* raw = cursor->cursor_value();
        Row   row = deserializeRow(raw, schema);

        if (stmt.select_cols.empty()) {
            printRow(row, schema);
        } else {
            for (const auto& col_name : stmt.select_cols) {
                int idx = schema.indexOf(col_name);
                std::visit([](auto&& v){ std::cout << v << "\t"; },
                           row.fields[idx]);
            }
            std::cout << "\n";
        }

        cursor->cursor_advance();
    }

    return ExecuteResult::EXECUTE_SUCCESS;
}


//  UPDATE

ExecuteResult VM::executeUpdate(const Statement& stmt) {
    Table* table = db_->getTable(stmt.table_name);
    if (!table) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

    const Schema& schema = table->schema;

    if (stmt.assignments.empty()) return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;

    int pk_idx = schema.primaryKeyIndex();
    if (pk_idx == -1) return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;

    uint32_t target_id = 0;
    bool found_pk = false;

    for (const auto& a : stmt.assignments) {
        int idx = schema.indexOf(a.col);
        if (idx == -1) return ExecuteResult::EXECUTE_COLUMN_NOT_FOUND;

        if (idx == pk_idx) {
            try {
                target_id = static_cast<uint32_t>(std::stoi(a.raw_value));
                found_pk = true;
            } catch (...) {
                return ExecuteResult::EXECUTE_TYPE_ERROR;
            }
        }
    }

    if (!found_pk) return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;

    auto cursor = table->find(target_id);
    if (cursor->end_of_table) return ExecuteResult::EXECUTE_KEY_NOT_FOUND;

    void* cell = cursor->cursor_value();
    Row   row  = deserializeRow(cell, schema);
    if (row.id != target_id) return ExecuteResult::EXECUTE_KEY_NOT_FOUND;

    for (const auto& a : stmt.assignments) {
        int idx = schema.indexOf(a.col);
        if (idx == -1) return ExecuteResult::EXECUTE_COLUMN_NOT_FOUND;
        try {
            row.fields[idx] = parseValue(a.raw_value, schema.columns[idx].type);
        } catch (...) {
            return ExecuteResult::EXECUTE_TYPE_ERROR;
        }
    }

    auto bytes = serializeRow(row);
    std::memset(cell, 0, LEAF_NODE_VALUE_SIZE);
    std::memcpy(cell, bytes.data(), bytes.size());

    return ExecuteResult::EXECUTE_SUCCESS;
}


//  CREATE TABLE

ExecuteResult VM::executeCreateTable(const Statement& stmt) {
    if (db_->getTable(stmt.table_name)) return ExecuteResult::EXECUTE_TABLE_EXISTS;

    Schema schema;
    schema.columns = stmt.columns;

    if (!db_->createTable(stmt.table_name, schema))
        return ExecuteResult::EXECUTE_UNKNOWN_ERROR;

    db_->saveSchema();
    std::cout << "Table '" << stmt.table_name << "' created.\n";
    return ExecuteResult::EXECUTE_SUCCESS;
}


//  DROP TABLE

ExecuteResult VM::executeDropTable(const Statement& stmt) {
    if (!db_->getTable(stmt.table_name)) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

    db_->deleteTable(stmt.table_name);
    db_->saveSchema();
    std::cout << "Table '" << stmt.table_name << "' dropped.\n";
    return ExecuteResult::EXECUTE_SUCCESS;
}


//  helpers

void VM::printRow(const Row& row, const Schema& schema) const {
    for (const auto& field : row.fields) {
        std::visit([](auto&& v){ std::cout << v << "\t"; }, field);
    }
    std::cout << "\n";
}

std::string VM::resultMessage(ExecuteResult r) {
    switch (r) {
        case ExecuteResult::EXECUTE_SUCCESS:          return "OK";
        case ExecuteResult::EXECUTE_DUPLICATE_KEY:    return "Error: duplicate primary key";
        case ExecuteResult::EXECUTE_TABLE_NOT_FOUND:  return "Error: table not found";
        case ExecuteResult::EXECUTE_TABLE_EXISTS:     return "Error: table already exists";
        case ExecuteResult::EXECUTE_SCHEMA_MISMATCH:  return "Error: wrong number of values for schema";
        case ExecuteResult::EXECUTE_TYPE_ERROR:       return "Error: value cannot be cast to column type";
        case ExecuteResult::EXECUTE_COLUMN_NOT_FOUND: return "Error: column not found";
        case ExecuteResult::EXECUTE_KEY_NOT_FOUND:    return "Error: no row with that id";
        case ExecuteResult::EXECUTE_ROW_TOO_LARGE:    return "Error: serialized row exceeds MAX_ROW_SIZE (512 bytes)";
        default:                                      return "Error: unknown";
    }
}