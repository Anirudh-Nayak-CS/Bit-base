#pragma once

#include "../constants/constant.h"
#include "../schema/schema.h"
#include <string>
#include <vector>

// One col = value assignment, used by UPDATE
struct Assignment {
  std::string col;
  std::string raw_value;
};

struct Statement {

  SQLcommandType type = UNDEFINED;

  // table targeted by any command
  std::string table_name;

  // INSERT
  // raw string values in schema column order as typed by the user Vm will cast
  // each one to the correct DataType via the  Schema helper functions
  std::vector<std::string> insert_values;
 

  // INSERT — multi-row: each inner vector is one row's values
  std::vector<std::vector<std::string>> multi_insert_rows;

  // SELECT
  std::vector<std::string> select_cols;

  //  UPDATE
  // parser stores raw strings; VM resolves each to the column's DataType
  std::vector<Assignment> assignments;

  //  CREATE TABLE

  std::vector<Column> columns;
};