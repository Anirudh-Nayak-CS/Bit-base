#pragma once

#include "../constants/constant.h"
#include "../statement/statement.h"
#include "../storage/database/db.h"
#include <string>

class VM {
public:
  explicit VM(db *database) : db_(database) {}

  ExecuteResult execute(const Statement &stmt);

  static std::string resultMessage(ExecuteResult r);

private:
  db *db_;

  ExecuteResult executeInsert(const Statement &stmt);
  ExecuteResult executeSelect(const Statement &stmt);
  ExecuteResult executeUpdate(const Statement &stmt);
  ExecuteResult executeDelete(const Statement& stmt);
  ExecuteResult executeCreateTable(const Statement &stmt);
  ExecuteResult executeDropTable(const Statement &stmt);

  void printRow(const Row &row, const Schema &schema) const;
};
