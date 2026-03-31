#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

enum class DataType : uint8_t {
  INT32 = 0,
  DOUBLE = 1,
  BOOL = 2,
  TEXT = 3,
};

struct Column {
  std::string name;
  DataType type;
  bool is_primary_key = false;  
};

struct Schema {
  std::vector<Column> columns;

  // find primary key column index (defaults to first column if none explicitly marked)
  int primaryKeyIndex() const {
    for (size_t i = 0; i < columns.size(); ++i)
      if (columns[i].is_primary_key)
        return static_cast<int>(i);
    // Default to first column if no explicit primary key is marked
    return -1;
  }

  //  find column index by name (-1 if not found)
  int indexOf(const std::string &name) const {
    for (size_t i = 0; i < columns.size(); ++i)
      if (columns[i].name == name)
        return static_cast<int>(i);
    return -1;
  }
};

using Value = std::variant<int32_t, double, bool, std::string>;

struct Row {
  uint32_t id = 0;
  std::vector<Value> fields;
};

//   parse a raw string → Value for a given DataType

inline Value parseValue(const std::string &raw, DataType type) {
  switch (type) {
  case DataType::INT32:
    return static_cast<int32_t>(std::stoi(raw));
  case DataType::DOUBLE:
    return std::stod(raw);
  case DataType::BOOL:
    return (raw == "true" || raw == "1");
  case DataType::TEXT:
    return raw;
  }
  return raw; // unreachable
}

// parse a SQL type keyword string → DataType

inline bool parseDataType(const std::string &s, DataType &out) {
  if (s == "INT" || s == "INT32") {
    out = DataType::INT32;
    return true;
  }
  if (s == "DOUBLE" || s == "FLOAT") {
    out = DataType::DOUBLE;
    return true;
  }
  if (s == "BOOL" || s == "BOOLEAN") {
    out = DataType::BOOL;
    return true;
  }
  if (s == "TEXT" || s == "STRING") {
    out = DataType::TEXT;
    return true;
  }
  return false;
}