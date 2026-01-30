
#include "compiler.h"
using namespace std;

SQLcommandType getCommandType(const std::string &cmd)
{
  if (cmd == "insert")
    return SQLcommandType::INSERT;
  if (cmd == "select")
    return SQLcommandType::SELECT;
  if (cmd == "delete")
    return SQLcommandType::DELETE;
  if (cmd == "update")
    return SQLcommandType::UPDATE;
  if (cmd == "create table")
    return SQLcommandType::CREATE_TABLE;
  if (cmd == "drop table")
    return SQLcommandType::DROP_TABLE;
  return SQLcommandType::UNDEFINED;
}

bool handle_meta_commands(string meta_command)
{
  if (meta_command == ".exit")
  {
    cout << "EXITED" << endl;
    exit(EXIT_SUCCESS);
  }
  else if (meta_command == ".tables")
  {
    cout << "TABLES" << endl;
    return true;
  }
  else if (meta_command == ".help")
  {
    cout << "--- Meta Commands ---" << endl;
    cout << "  .exit      : Exit the database REPL" << endl;
    cout << "  .help      : Print this message" << endl;
    cout << "  .tables    : List all tables" << endl;

    cout << "\n--- SQL Commands ---" << endl;
    cout << "  insert <id> <username> <email> : Add a new row" << endl;
    cout << "  select                         : Print all rows" << endl;
    cout << "  update <id> <username> <email> : Modify an existing row" << endl;
    cout << "  delete <id>                    : Remove a row" << endl;
    cout << "  create table <name>            : Create a new table" << endl;
    cout << "  drop table <name>              : Delete a table" << endl;
    return true;
  }
  else
  {
    cout << "Unrecognized command .cmd" << endl;
    return false;
  }
}

Commandstatus handle_insert(string sql_command, stmt &statement)
{
  stringstream ss(sql_command);
  string words;
  if (!(ss >> words))
  {
    return SYNTAX_ERROR;
  }
  int id;
  string username;
  string email;

  // id
  if (!(ss >> id))
  {
    return SYNTAX_ERROR;
  }
  if (id < 0)
  {

    return NEGATIVE_INT;
  }

  // username
  if (!(ss >> username))
  {
    return SYNTAX_ERROR;
  }
  if (username.length() > 32)
  {

    return STRING_TOO_LONG;
  }

  // email
  if (!(ss >> email))
  {
    return SYNTAX_ERROR;
  }
  if (email.length() > 255)
  {

    return STRING_TOO_LONG;
  }
  uint32_t final_id = static_cast<uint32_t>(id);
  char final_username[32];
  char final_email[255];

  strncpy(final_username, username.c_str(), sizeof(final_username));
  strncpy(final_email, email.c_str(), sizeof(final_email));
  final_username[sizeof(final_username) - 1] = '\0';
  final_email[sizeof(final_email) - 1] = '\0';

  statement.row_data = Row(final_username, final_email);
  statement.id = final_id;
  statement.type = INSERT;

  cout << " insertion parsed." << endl;
  return SUCCESS;
}
Commandstatus handle_update(string sql_command, stmt &statement)
{
  stringstream ss(sql_command);
  string words;
  if (!(ss >> words))
  {
    return SYNTAX_ERROR;
  }
  int id;
  string username;
  string email;

  // id
  if (!(ss >> id))
  {
    return SYNTAX_ERROR;
  }
  if (id < 0)
  {

    return NEGATIVE_INT;
  }

  // username
  if (!(ss >> username))
  {

    return SYNTAX_ERROR;
  }
  if (username.length() > 32)
  {

    return STRING_TOO_LONG;
  }

  // email
  if (!(ss >> email))
  {
    return SYNTAX_ERROR;
  }
  if (email.length() > 255)
  {

    return STRING_TOO_LONG;
  }
  uint32_t final_id = static_cast<uint32_t>(id);
  char final_username[32];
  char final_email[255];

  strncpy(final_username, username.c_str(), sizeof(final_username));
  strncpy(final_email, email.c_str(), sizeof(final_email));
  final_username[sizeof(final_username) - 1] = '\0';
  final_email[sizeof(final_email) - 1] = '\0';

  statement.row_data = Row(final_username, final_email);
  statement.id = final_id;
  statement.type = UPDATE;

  cout << "updation parsed." << endl;
  return SUCCESS;
}

Commandstatus handle_delete(string sql_command, stmt &statement)
{
  stringstream ss(sql_command);
  string words;

  if (!(ss >> words))
  {
    return SYNTAX_ERROR;
  }
  int id;

  // id
  if (!(ss >> id))
  {
    return SYNTAX_ERROR;
  }
  if (id < 0)
  {
    return NEGATIVE_INT;
  }

  string leftover;
  if (ss >> leftover)
  {
    return SYNTAX_ERROR;
  }

  uint32_t final_id = static_cast<uint32_t>(id);

  statement.id = final_id;
  statement.type = DELETE;

  cout << "deletion parsed." << endl;
  return SUCCESS;
}

Commandstatus handle_select(string sql_command, stmt &statement)
{
  stringstream ss(sql_command);
  string words;
  if (!(ss >> words))
  {
    return SYNTAX_ERROR;
  }
  string leftover;
  if (ss >> leftover)
  {
    return SYNTAX_ERROR;
  }

  statement.type = SELECT;

  cout << "selection parsed." << endl;
  return SUCCESS;
}

Commandstatus handle_create(string sql_command, stmt &statement)
{
  stringstream ss(sql_command);
  string first_word;
  if (!(ss >> first_word))
  {
    return SYNTAX_ERROR;
  }

  string second_word;
  if (!(ss >> second_word))
  {
    return SYNTAX_ERROR;
  }
  string leftover;
  if ((ss >> leftover))
  {
    return SYNTAX_ERROR;
  }

  statement.type = CREATE_TABLE;

  cout << "create table parsed." << endl;
  return SUCCESS;
}

Commandstatus handle_drop(string sql_command, stmt &statement)
{
  stringstream ss(sql_command);
  string words;
  string first_word;
  if (!(ss >> first_word))
  {
    return SYNTAX_ERROR;
  }

  string second_word;
  if (!(ss >> second_word))
  {
    return SYNTAX_ERROR;
  }
  string leftover;
  if ((ss >> leftover))
  {
    return SYNTAX_ERROR;
  }

  statement.type = DROP_TABLE;

  cout << "drop table parsed." << endl;
  return SUCCESS;
}

Commandstatus handle_SQL_Commands(string sql_command, stmt &statement)
{
  stringstream ss(sql_command);
  string first_word;
  if (!(ss >> first_word))
  {
    return SYNTAX_ERROR;
  }
  string final_command = first_word;
  string second_word;
  if (first_word == "drop" || first_word == "create")
  {
    if (ss >> second_word && second_word == "table")
    {
      final_command += " " + second_word;
    }
    else {
      return SYNTAX_ERROR;
    }
  }

  SQLcommandType sql_command_type = getCommandType(final_command);
  Commandstatus result;

  switch (sql_command_type)
  {
  case INSERT:
    result = handle_insert(sql_command, statement);
    break;
  case DELETE:
    result = handle_delete(sql_command, statement);
    break;
  case UPDATE:
    result = handle_update(sql_command, statement);
    break;
  case DROP_TABLE:
    result = handle_drop(sql_command, statement);
    break;
  case CREATE_TABLE:
    result = handle_create(sql_command, statement);
    break;
  case SELECT:
    result = handle_select(sql_command, statement);
    break;
  default:
    result = FAILURE;
  }

  return result;
}
