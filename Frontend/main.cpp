#include "input_buffer.h"
#include "compiler.h"
#include "virtual_machine.h"

void print_prompt() { cout << "db > "; }

int main()
{
  inputBuffer *input_buffer = new inputBuffer();
  while (true)
  {
    print_prompt();
    input_buffer->read_input();
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
    Commandstatus sql_result = handle_SQL_Commands(buffer, statement);
    switch (sql_result)
    {
    case SUCCESS:
      execute_statement(statement);
      continue;
    case STRING_TOO_LONG:
      cout << "Error: Input string is too long (Username < 32 chars, Email < 255)." << endl;
      break;

    case NEGATIVE_INT:
      cout << "Error: ID cannot be negative." << endl;
      break;

    case SYNTAX_ERROR:
      cout << "Error: Syntax error. Could not parse command." << endl;
      break;

    case OUT_OF_RANGE:
      cout << "Error: Age was out of range (age > 0 and age < 100)" << endl;
      break;
    default:
      cout << "Error: Unknown error while handling SQL command." << endl;
      break;
        }
  }
}