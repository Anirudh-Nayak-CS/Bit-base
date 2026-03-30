#include "../../headers/parser/parser.h"

#include <cctype>

#include <cstring>

bool Parser::ieq(const std::string & a,
  const std::string & b) {
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (std::tolower((unsigned char) a[i]) != std::tolower((unsigned char) b[i]))
      return false;
  return true;
}

bool Parser::match(const std::vector < Token > & t, size_t i, TokenType type,
  const std::string & value) const {
  if (i >= t.size())
    return false;
  if (t[i].type != type)
    return false;
  if (!value.empty() && !ieq(t[i].value, value))
    return false;
  return true;
}

Commandstatus Parser::parse(const std::vector < Token > & tokens, Statement & out) {
  if (tokens.empty() || tokens[0].type == TokenType::END)
    return CMD_SYNTAX_ERROR;
  if (tokens[0].type != TokenType::KEYWORD)
    return CMD_SYNTAX_ERROR;

  const std::string & first = tokens[0].value;

  if (ieq(first, "INSERT"))
    return parseInsert(tokens, out);
  else if (ieq(first, "SELECT"))
    return parseSelect(tokens, out);
  else if (ieq(first, "UPDATE"))
    return parseUpdate(tokens, out);
  else if (ieq(first, "CREATE"))
    return parseCreateTable(tokens, out);
  else if (ieq(first, "DROP"))
    return parseDropTable(tokens, out);

  return CMD_SYNTAX_ERROR;
}

//  INSERT INTO <table> VALUES <v1> <v2> ... <vN>

Commandstatus Parser::parseInsert(const std::vector < Token > & t, Statement & out) {
  // INSERT INTO table VALUES + at least 1 value
  if (t.size() < 5)
    return CMD_SYNTAX_ERROR;

  if (!match(t, 1, TokenType::KEYWORD, "INTO"))
    return CMD_SYNTAX_ERROR;
  if (!match(t, 2, TokenType::IDENTIFIER))
    return CMD_SYNTAX_ERROR;
  if (!match(t, 3, TokenType::KEYWORD, "VALUES"))
    return CMD_SYNTAX_ERROR;

  out.type = INSERT;
  out.table_name = t[2].value;
  out.insert_values.clear();

  // the VM will validate count and types against the schema
  for (size_t i = 4; i < t.size(); ++i) {
    if (t[i].type == TokenType::END)
      break;
    // Accept NUMBER, STRING (quoted), or IDENTIFIER (unquoted strings)
    if (t[i].type != TokenType::NUMBER && t[i].type != TokenType::STRING && t[i].type != TokenType::IDENTIFIER)
      return CMD_SYNTAX_ERROR;
    out.insert_values.push_back(t[i].value);
  }

  if (out.insert_values.empty())
    return CMD_SYNTAX_ERROR;

  return CMD_SUCCESS;
}

//  SELECT * FROM <table> [WHERE id = <n>]

Commandstatus Parser::parseSelect(const std::vector < Token > & t, Statement & out) {
  if (t.size() < 4)
    return CMD_SYNTAX_ERROR;

  out.type = SELECT;
  out.select_cols.clear();

  // collect column names between SELECT and FROM
  // if the first token after SELECT is '*', treat as SELECT ALL (empty list)
  size_t i = 1;
  if (match(t, i, TokenType::IDENTIFIER, "*")) {
    // SELECT * — leave select_cols empty, skip the *
    i++;
  } else {
    // collect named columns until we hit FROM
    while (i < t.size() && !match(t, i, TokenType::KEYWORD, "FROM")) {
      if (t[i].type == TokenType::END)
        return CMD_SYNTAX_ERROR;
      if (t[i].type != TokenType::IDENTIFIER)
        return CMD_SYNTAX_ERROR;
      out.select_cols.push_back(t[i].value);
      i++;
    }
  }

  if (!match(t, i, TokenType::KEYWORD, "FROM"))
    return CMD_SYNTAX_ERROR;
  i++; // skip FROM

  if (!match(t, i, TokenType::IDENTIFIER))
    return CMD_SYNTAX_ERROR;
  out.table_name = t[i].value;
  i++; // skip table name

  return CMD_SUCCESS;
}

//  UPDATE <table> SET <col1>=<val1> [, <col2>=<val2> ...] WHERE id = <n>

Commandstatus Parser::parseUpdate(const std::vector < Token > & t, Statement & out) {
  if (t.size() < 7)
    return CMD_SYNTAX_ERROR;

  if (!match(t, 1, TokenType::IDENTIFIER))
    return CMD_SYNTAX_ERROR; // table
  if (!match(t, 2, TokenType::KEYWORD, "SET"))
    return CMD_SYNTAX_ERROR;

  out.type = UPDATE;
  out.table_name = t[1].value;
  out.assignments.clear();

  size_t i = 3;


  while (i < t.size()) {
    if (t[i].type == TokenType::END)
      break;  // Normal end of assignments

    // col
    if (!match(t, i, TokenType::IDENTIFIER))
      return CMD_SYNTAX_ERROR;
    std::string col = t[i].value;
    i++;

  
    if (i >= t.size())
      return CMD_SYNTAX_ERROR;
    if (t[i].value != "=")
      return CMD_SYNTAX_ERROR;
    i++;

    // value
    if (i >= t.size())
      return CMD_SYNTAX_ERROR;
    // Accept NUMBER, STRING (quoted), or IDENTIFIER (unquoted strings)
    if (t[i].type != TokenType::STRING && t[i].type != TokenType::NUMBER && t[i].type != TokenType::IDENTIFIER)
      return CMD_SYNTAX_ERROR;

    out.assignments.push_back({
      col,
      t[i].value
    });
    i++;
    
    // Skip comma if present (for separating multiple assignments)
    if (i < t.size() && t[i].value == ",") {
      i++;
    }
  }

  if (out.assignments.empty())
    return CMD_SYNTAX_ERROR;

  return CMD_SUCCESS;
}

//  CREATE TABLE <name> <col1> <TYPE1> [PRIMARY KEY] <col2> <TYPE2> ...

Commandstatus Parser::parseCreateTable(const std::vector < Token > & t,
  Statement & out) {
  // minimum: CREATE TABLE name col TYPE  (6 tokens)
  if (t.size() < 6)
    return CMD_SYNTAX_ERROR;

  if (!match(t, 1, TokenType::KEYWORD, "TABLE"))
    return CMD_SYNTAX_ERROR;
  if (!match(t, 2, TokenType::IDENTIFIER))
    return CMD_SYNTAX_ERROR;

  out.type = CREATE_TABLE;
  out.table_name = t[2].value;
  out.columns.clear();

  // parse col TYPE [PRIMARY KEY] pairs
  for (size_t i = 3; i < t.size(); ) {
    if (t[i].type == TokenType::END)
      break;

    // column name
    if (t[i].type != TokenType::IDENTIFIER)
      return CMD_SYNTAX_ERROR;
    std::string col_name = t[i].value;
    i++;

    if (i >= t.size())
      return CMD_SYNTAX_ERROR;

    // type keyword — stored as KEYWORD or IDENTIFIER depending on tokenizer
    if (t[i].type != TokenType::KEYWORD && t[i].type != TokenType::IDENTIFIER)
      return CMD_SYNTAX_ERROR;

    DataType dtype;
    std::string type_str = t[i].value;
    // uppercase for comparison
    for (char & c: type_str)
      c = std::toupper((unsigned char) c);

    if (!parseDataType(type_str, dtype))
      return CMD_SYNTAX_ERROR;
    i++;

    // Check for PRIMARY KEY marker
    bool is_pk = false;
    if (i < t.size() && match(t, i, TokenType::KEYWORD, "PRIMARY")) {
      if (i + 1 < t.size() && match(t, i + 1, TokenType::KEYWORD, "KEY")) {
        is_pk = true;
        i += 2;  
      }
    }

    out.columns.push_back({
      col_name,
      dtype,
      is_pk
    });
  }

  if (out.columns.empty())
    return CMD_SYNTAX_ERROR;

  return CMD_SUCCESS;
}

//  DROP TABLE <name>

Commandstatus Parser::parseDropTable(const std::vector < Token > & t,
  Statement & out) {
  if (t.size() < 3)
    return CMD_SYNTAX_ERROR;

  if (!match(t, 1, TokenType::KEYWORD, "TABLE"))
    return CMD_SYNTAX_ERROR;
  if (!match(t, 2, TokenType::IDENTIFIER))
    return CMD_SYNTAX_ERROR;

  out.type = DROP_TABLE;
  out.table_name = t[2].value;

  return CMD_SUCCESS;
}