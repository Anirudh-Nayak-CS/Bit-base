#pragma once
#include "common.h"
using namespace std;
#include "table.h"

typedef struct
{
  SQLcommandType type;
  uint32_t id;
  Row row_data;
} stmt;

SQLcommandType getCommandType(const string &cmd);

bool handle_meta_commands(string meta_command);

Commandstatus handle_insert(string sql_command, stmt &statement);

Commandstatus handle_update(string sql_command, stmt &statement);

Commandstatus handle_delete(string sql_command, stmt &statement);

Commandstatus handle_select(string sql_command, stmt &statement);

Commandstatus handle_create(string sql_command, stmt &statement);

Commandstatus handle_drop(string sql_command, stmt &statement);

Commandstatus handle_SQL_Commands(string sql_command, stmt &statement);

void get_current_time(char* buffer,size_t size);