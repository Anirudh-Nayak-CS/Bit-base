#include "../../headers/vm/vm.h"
#include "../../headers/constants/constant.h"
#include "../../headers/storage/node/leaf_node.h"
#include "../../headers/schema/schema.h"
#include "../../headers/storage/row/row_serialize.h"
#include <iostream>
#include <cstring>
#include <algorithm>

// Helper: compare a FieldValue against a raw string using an operator.
// Returns true if the row passes the WHERE filter.
static bool matchesWhere(const Value& field,
                          const std::string& op,
                          const std::string& raw_value,
                          DataType dtype) {
    if (dtype == DataType::INT32) {
        int32_t row_val  = std::get<int32_t>(field);
        int32_t cmp_val;
        try { cmp_val = std::stoi(raw_value); } catch (...) { return false; }
        if (op == "=")  return row_val == cmp_val;
        if (op == "<")  return row_val <  cmp_val;
        if (op == ">")  return row_val >  cmp_val;
        if (op == "<=") return row_val <= cmp_val;
        if (op == ">=") return row_val >= cmp_val;
    } else {
        // TEXT comparison
        std::string row_str = std::get<std::string>(field);
        if (op == "=")  return row_str == raw_value;
        if (op == "<")  return row_str <  raw_value;
        if (op == ">")  return row_str >  raw_value;
        if (op == "<=") return row_str <= raw_value;
        if (op == ">=") return row_str >= raw_value;
    }
    return false;
}



ExecuteResult VM::execute(const Statement & stmt) {
  switch (stmt.type) {
  case INSERT:
    return executeInsert(stmt);
  case SELECT:
    return executeSelect(stmt);
  case UPDATE:
    return executeUpdate(stmt);
  case CREATE_TABLE:
    return executeCreateTable(stmt);
  case DROP_TABLE:
    return executeDropTable(stmt);
  default:
    return ExecuteResult::EXECUTE_UNKNOWN_ERROR;
  }
}

static void pin_cursor_page(db* database, Cursor* cursor) {
    Pager* pager = cursor->table->pager;
    uint32_t pnum = cursor->page_num;
    void* page = pager->get_page(pnum);
    database->pin_page(pnum, page, pager);
}

//  INSERT

// helper: insert a single row of raw string values into the table
static ExecuteResult insertOneRow(db* database,Table * table,
  const Schema & schema,
    const std::vector < std::string > & values) {
  if (values.size() != schema.columns.size())
    return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;

  int pk_idx = schema.primaryKeyIndex();
  if (pk_idx == -1) {
    std::cerr << "Error: table has no PRIMARY KEY column.\n" <<
      "Hint: CREATE TABLE t (id INT PRIMARY KEY, ...)\n";
    return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;
  }
  if (schema.columns[pk_idx].type != DataType::INT32) {
    std::cerr << "Error: PRIMARY KEY column must be INT.\n";
    return ExecuteResult::EXECUTE_TYPE_ERROR;
  }

  Row row;
  try {
    row.id = static_cast < uint32_t > (std::stoi(values[pk_idx]));

    for (size_t i = 0; i < schema.columns.size(); ++i)
      row.fields.push_back(parseValue(values[i], schema.columns[i].type));

  } catch (...) {
    std::cerr << "Error: could not parse one or more values — " <<
      "check types match the schema.\n";
    return ExecuteResult::EXECUTE_TYPE_ERROR;
  }

  auto bytes = serializeRow(row);
  if (bytes.size() > LEAF_NODE_VALUE_SIZE)
    return ExecuteResult::EXECUTE_ROW_TOO_LARGE;

    // Find the insertion position first (read-only)
  auto cursor = table->find(row.id);
 
    
  pin_cursor_page(database, cursor.get());  

  bool ok = table -> insert(row.id, bytes.data(), static_cast < uint32_t > (bytes.size()));
  if (!ok) return ExecuteResult::EXECUTE_DUPLICATE_KEY;
  return ExecuteResult::EXECUTE_SUCCESS;

}

//INSERT 

ExecuteResult VM::executeInsert(const Statement& stmt) {
    Table* table = db_->getTable(stmt.table_name);
    if (!table) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

    const Schema& schema = table->schema;

    Transaction* txn = db_->begin_txn();
    ExecuteResult result = ExecuteResult::EXECUTE_SUCCESS;

    if (!stmt.multi_insert_rows.empty()) {
        for (const auto& row_vals : stmt.multi_insert_rows) {
            result = insertOneRow(db_, table, schema, row_vals);
            if (result != ExecuteResult::EXECUTE_SUCCESS) break;
        }
    } else {
        result = insertOneRow(db_, table, schema, stmt.insert_values);
    }

    // single commit/rollback path for both single and multi-row
    if (result == ExecuteResult::EXECUTE_SUCCESS)
        db_->commit_txn(txn->id());
    else
        db_->rollback_txn(txn->id());

    return result;
}

//  SELECT
ExecuteResult VM::executeSelect(const Statement& stmt) {
    Table* table = db_->getTable(stmt.table_name);
    if (!table) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

    const Schema& schema = table->schema;

    // Validate requested columns exist
    for (const auto& col : stmt.select_cols)
        if (schema.indexOf(col) == -1) return ExecuteResult::EXECUTE_COLUMN_NOT_FOUND;

    // Validate WHERE column exists
    int where_idx = -1;
    if (stmt.where.active) {
        where_idx = schema.indexOf(stmt.where.col);
        if (where_idx == -1) return ExecuteResult::EXECUTE_COLUMN_NOT_FOUND;
    }

    // Validate ORDER BY column exists
    int order_idx = -1;
    if (stmt.order_by.active) {
        order_idx = schema.indexOf(stmt.order_by.col);
        if (order_idx == -1) return ExecuteResult::EXECUTE_COLUMN_NOT_FOUND;
    }

    //scan all rows
    std::vector<Row> results;
    auto cursor = Cursor::table_start(table);

    while (!cursor->end_of_table) {
        void* raw = cursor->cursor_value();
        Row row   = deserializeRow(raw, schema);

        // WHERE filter
        if (stmt.where.active) {
            bool pass = matchesWhere(
                row.fields[where_idx],
                stmt.where.op,
                stmt.where.value,
                schema.columns[where_idx].type);
            if (!pass) { cursor->cursor_advance(); continue; }
        }

        results.push_back(std::move(row));
        cursor->cursor_advance();
    }

    //  ORDER BY 
    if (stmt.order_by.active) {
        bool desc = stmt.order_by.descending;
        DataType otype = schema.columns[order_idx].type;

        std::sort(results.begin(), results.end(),
            [&](const Row& a, const Row& b) {
                if (otype == DataType::INT32) {
                    int32_t va = std::get<int32_t>(a.fields[order_idx]);
                    int32_t vb = std::get<int32_t>(b.fields[order_idx]);
                    return desc ? va > vb : va < vb;
                } else {
                    const std::string& va = std::get<std::string>(a.fields[order_idx]);
                    const std::string& vb = std::get<std::string>(b.fields[order_idx]);
                    return desc ? va > vb : va < vb;
                }
            });
    }

    // print header
    if (stmt.select_cols.empty()) {
        for (const auto& col : schema.columns) std::cout << col.name << "\t";
    } else {
        for (const auto& col : stmt.select_cols) std::cout << col << "\t";
    }
    std::cout << "\n" << std::string(40, '-') << "\n";

    // print rows
    for (const auto& row : results) {
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
    }

    return ExecuteResult::EXECUTE_SUCCESS;
}

//  UPDATE

ExecuteResult VM::executeUpdate(const Statement& stmt) {
    Table* table = db_->getTable(stmt.table_name);
    if (!table) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

    const Schema& schema = table->schema;

    if (stmt.assignments.empty()) return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;

    // Validate all SET columns exist
    for (const auto& a : stmt.assignments)
        if (schema.indexOf(a.col) == -1) return ExecuteResult::EXECUTE_COLUMN_NOT_FOUND;

    // Validate WHERE column exists
    int where_idx = -1;
    if (stmt.where.active) {
        where_idx = schema.indexOf(stmt.where.col);
        if (where_idx == -1) return ExecuteResult::EXECUTE_COLUMN_NOT_FOUND;
    }

    Transaction* txn = db_->begin_txn();
    ExecuteResult result = ExecuteResult::EXECUTE_SUCCESS;
    int updated = 0;

    auto cursor = Cursor::table_start(table);

    while (!cursor->end_of_table) {
        void* cell = cursor->cursor_value();
        Row row    = deserializeRow(cell, schema);

        // WHERE filter — skip rows that don't match
        if (stmt.where.active) {
            bool pass = matchesWhere(
                row.fields[where_idx],
                stmt.where.op,
                stmt.where.value,
                schema.columns[where_idx].type);
            if (!pass) { cursor->cursor_advance(); continue; }
        }

        // Pin this page before first write
        pin_cursor_page(db_, cursor.get());

        // Apply SET assignments
        for (const auto& a : stmt.assignments) {
            int idx = schema.indexOf(a.col);
            try {
                row.fields[idx] = parseValue(a.raw_value, schema.columns[idx].type);
            } catch (...) {
                result = ExecuteResult::EXECUTE_TYPE_ERROR;
                break;
            }
        }

        if (result != ExecuteResult::EXECUTE_SUCCESS) break;

        auto bytes = serializeRow(row);
        std::memset(cell, 0, LEAF_NODE_VALUE_SIZE);
        std::memcpy(cell, bytes.data(), bytes.size());
        updated++;

        cursor->cursor_advance();
    }

    if (result == ExecuteResult::EXECUTE_SUCCESS)
        db_->commit_txn(txn->id());
    else
        db_->rollback_txn(txn->id());

    if (result == ExecuteResult::EXECUTE_SUCCESS && updated == 0
        && stmt.where.active)
        return ExecuteResult::EXECUTE_KEY_NOT_FOUND;

    return result;
}

//  CREATE TABLE

ExecuteResult VM::executeCreateTable(const Statement & stmt) {
  if (db_ -> getTable(stmt.table_name)) return ExecuteResult::EXECUTE_TABLE_EXISTS;

  // validating exactly one PRIMARY KEY column, and it must be INT32
  int pk_count = 0;
  for (const auto & col: stmt.columns) {
    if (col.is_primary_key) {
      pk_count++;
      if (col.type != DataType::INT32) {
        std::cerr << "Error: PRIMARY KEY column '" << col.name <<
          "' must be INT.\n";
        return ExecuteResult::EXECUTE_TYPE_ERROR;
      }
    }
  }
  if (pk_count == 0) {
    std::cerr << "Error: no PRIMARY KEY defined.\n" <<
      "Hint: CREATE TABLE t (id INT PRIMARY KEY, col1 TEXT, ...)\n";
    return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;
  }
  if (pk_count > 1) {
    std::cerr << "Error: only one PRIMARY KEY allowed.\n";
    return ExecuteResult::EXECUTE_SCHEMA_MISMATCH;
  }

  Schema schema;
  schema.columns = stmt.columns;

  if (!db_ -> createTable(stmt.table_name, schema))
    return ExecuteResult::EXECUTE_UNKNOWN_ERROR;

  db_ -> saveSchema();
  std::cout << "Table '" << stmt.table_name << "' created.\n";
  return ExecuteResult::EXECUTE_SUCCESS;
}

//  DROP TABLE

ExecuteResult VM::executeDropTable(const Statement & stmt) {
  if (!db_ -> getTable(stmt.table_name)) return ExecuteResult::EXECUTE_TABLE_NOT_FOUND;

  db_ -> deleteTable(stmt.table_name);
  db_ -> saveSchema();
  std::cout << "Table '" << stmt.table_name << "' dropped.\n";
  return ExecuteResult::EXECUTE_SUCCESS;
}

//  helpers

void VM::printRow(const Row & row,
  const Schema & schema) const {
  for (const auto & field: row.fields) {
    std::visit([](auto && v) {
      std::cout << v << "\t";
    }, field);
  }
  std::cout << "\n";
}

std::string VM::resultMessage(ExecuteResult r) {
  switch (r) {
  case ExecuteResult::EXECUTE_SUCCESS:
    return "OK";
  case ExecuteResult::EXECUTE_DUPLICATE_KEY:
    return "Error: duplicate primary key";
  case ExecuteResult::EXECUTE_TABLE_NOT_FOUND:
    return "Error: table not found";
  case ExecuteResult::EXECUTE_TABLE_EXISTS:
    return "Error: table already exists";
  case ExecuteResult::EXECUTE_SCHEMA_MISMATCH:
    return "Error: wrong number of values for schema";
  case ExecuteResult::EXECUTE_TYPE_ERROR:
    return "Error: value cannot be cast to column type";
  case ExecuteResult::EXECUTE_COLUMN_NOT_FOUND:
    return "Error: column not found";
  case ExecuteResult::EXECUTE_KEY_NOT_FOUND:
    return "Error: no row with that id";
  case ExecuteResult::EXECUTE_ROW_TOO_LARGE:
    return "Error: serialized row exceeds MAX_ROW_SIZE (512 bytes)";
  default:
    return "Error: unknown";
  }
}