#pragma once
#include "../constants/constant.h"
#include "../statement/statement.h"
#include "../tokenizer/tokenizer.h"
#include <vector>

//  Parser  it Converts a flat token list into a Statement.

class Parser {
public:
  Commandstatus parse(const std::vector<Token> &tokens, Statement &out);

private:
  Commandstatus parseInsert(const std::vector<Token> &t, Statement &out);
  Commandstatus parseSelect(const std::vector<Token> &t, Statement &out);
  Commandstatus parseUpdate(const std::vector<Token> &t, Statement &out);
  Commandstatus parseCreateTable(const std::vector<Token> &t, Statement &out);
  Commandstatus parseDropTable(const std::vector<Token> &t, Statement &out);

  // true if token i exists, matches type, and (if given) value matches
  // case-insensitively
  bool match(const std::vector<Token> &t, size_t i, TokenType type,
             const std::string &value = "") const;

  // case-insensitive string compare
  static bool ieq(const std::string &a, const std::string &b);
};