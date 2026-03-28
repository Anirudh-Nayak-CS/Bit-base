#include "headers/input_buffer.h"
#include "headers/compiler.h"
#include "headers/virtual_machine.h"
#include "headers/storage/database/db.h"

void print_prompt() { cout << "db > "; }

int main()
{
  db *database = db::db_open("db");
  inputBuffer *input_buffer = new inputBuffer();
  
  try {
    while (true)
    {
      print_prompt();

      try {
        input_buffer->read_input();
      } catch (const std::runtime_error& e) {
        cout << "EOF reached - exiting database" << endl;
        break;
      }
      
      string buffer = input_buffer->getInputBuffer();
      if (buffer.empty())
      {
        continue;
      }
      if (buffer[0] == '.')
      {
        bool meta_result = handle_meta_commands(buffer);
        switch (meta_result)
        {
        case true:
          cout << "Successfully handled the meta-command" << endl;
          continue;
        case false:
          cout << "Error while handling meta-command" << endl;
          continue;
        }
      }
      stmt statement;
      Commandstatus sql_result = handle_SQL_Commands(buffer, statement, database);
      switch (sql_result)
      {
      case Commandstatus::CMD_SUCCESS:
        execute_statement(statement);
        continue;
      case Commandstatus::CMD_STRING_TOO_LONG:
        cout << "Error: Input string is too long (Username < 32 chars, Email < 255)." << endl;
        break;

      case Commandstatus::CMD_NEGATIVE_INT:
        cout << "Error: ID cannot be negative." << endl;
        break;

      case Commandstatus::CMD_SYNTAX_ERROR:
        cout << "Error: Syntax error. Could not parse command." << endl;
        break;

      case Commandstatus::CMD_OUT_OF_RANGE:
        cout << "Error: Age was out of range (age > 0 and age < 100)" << endl;
        break;
      default:
        cout << "Error: Unknown error while handling SQL command." << endl;
        break;
      }
    }
  } catch (const std::exception& e) {
    cout << "Error: " << e.what() << endl;
  }
  
  cout << "Closing database..." << endl;
  database->db_close();
  delete database;
  delete input_buffer;
  return EXIT_SUCCESS;
}