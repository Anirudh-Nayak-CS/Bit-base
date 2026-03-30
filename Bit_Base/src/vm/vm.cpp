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

ExecuteResult VM::executeInsert(const Statement& stmt) {
    Table* table = db_->getTable(stmt.table_name);
    if (!table) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

    const Schema& schema = table->schema;

    if (stmt.insert_values.size() != schema.columns.size())
        return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;

    // build typed Row
    Row row;
    try {
        // Find the primary key column from schema
        int pk_idx = schema.primaryKeyIndex();
        if (pk_idx == -1) return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;
        
        row.id = static_cast<uint32_t>(std::stoi(stmt.insert_values[pk_idx]));

        for (size_t i = 0; i < schema.columns.size(); ++i)
            row.fields.push_back(parseValue(stmt.insert_values[i],
                                            schema.columns[i].type));
    } catch (...) {
        return ExecuteResult::EXECUTE_TYPE_ERROR;
    }

    // VM serializes here — Table receives flat bytes, knows nothing about Row
    auto bytes = serializeRow(row);

    if (bytes.size() > LEAF_NODE_VALUE_SIZE)
        return ExecuteResult::EXECUTE_ROW_TOO_LARGE;

    table->insert(row.id, bytes.data(), static_cast<uint32_t>(bytes.size()));
    return ExecuteResult::EXECUTE_SUCCESS;
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

    std::unique_ptr<Cursor> cursor =  Cursor::table_start(table);

    while (!cursor->end_of_table) {
        // VM deserializes raw bytes → typed Row using the Schema
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
//  deserialize → patch fields → re-serialize into same cell slot


ExecuteResult VM::executeUpdate(const Statement& stmt) {
    Table* table = db_->getTable(stmt.table_name);
    if (!table) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

    const Schema& schema = table->schema;

    if (stmt.assignments.empty()) return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;

    // Find the primary key column from schema (defaults to first column if none marked)
    int pk_idx = schema.primaryKeyIndex();
    if (pk_idx == -1) return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;

    // Extract ID from the assignment that updates the primary key column
    uint32_t target_id = 0;
    bool found_pk = false;

    for (const auto& a : stmt.assignments) {
        int idx = schema.indexOf(a.col);
        if (idx == -1) return ExecuteResult::EXECUTE_COLUMN_NOT_FOUND;
        
        // If this assignment is for the primary key column, extract the ID
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

    // Apply all assignments to row fields
    for (const auto& a : stmt.assignments) {
        int idx = schema.indexOf(a.col);
        if (idx == -1) return ExecuteResult::EXECUTE_COLUMN_NOT_FOUND;
        try {
            row.fields[idx] = parseValue(a.raw_value, schema.columns[idx].type);
        } catch (...) {
            return ExecuteResult::EXECUTE_TYPE_ERROR;
        }
    }

    // re-serialize patched row back into the same cell slot in-place
    auto bytes = serializeRow(row);
    std::memset(cell, 0, LEAF_NODE_VALUE_SIZE);
    std::memcpy(cell, bytes.data(), bytes.size());

    return ExecuteResult::EXECUTE_SUCCESS;
}

//create table

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

// drop table 

ExecuteResult VM::executeDropTable(const Statement& stmt) {
    if (!db_->getTable(stmt.table_name)) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

    db_->deleteTable(stmt.table_name);
    db_->saveSchema();
    std::cout << "Table '" << stmt.table_name << "' dropped.\n";
    return ExecuteResult::EXECUTE_SUCCESS;
}

//helpers

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
        default:                              return "Error: unknown";
    }
}