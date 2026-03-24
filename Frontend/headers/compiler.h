#pragma once
#include "constants/constant.h"
#include "storage/row/row.h"
#include "storage/table/table.h"

typedef struct
{
  SQLcommandType type;
  uint32_t id;
  Row row_data;
} stmt;

SQLcommandType getCommandType(const std::string &cmd);

bool handle_meta_commands(std::string meta_command);

Commandstatus handle_insert(std::string sql_command, stmt &statement);

Commandstatus handle_update(std::string sql_command, stmt &statement);

Commandstatus handle_delete(std::string sql_command, stmt &statement);

Commandstatus handle_select(std::string sql_command, stmt &statement);

Commandstatus handle_create(std::string sql_command, stmt &statement);

Commandstatus handle_drop(std::string sql_command, stmt &statement);

Commandstatus handle_SQL_Commands(std::string sql_command, stmt &statement);

void get_current_time(char* buffer,size_t size);