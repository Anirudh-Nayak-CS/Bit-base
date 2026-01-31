#include<bits/stdc++.h>
#include "virtual_machine.h"
using namespace std;

void execute_statement(const stmt &statement)
{

  switch (statement.type)
  {
  case INSERT:
    cout << "INSERT command recognized.\n";
    break;
  case DELETE:
    cout << "DELETE command recognized.\n";
    break;
  case UPDATE:
    cout << "UPDATE command recognized.\n";
    break;
  case DROP_TABLE:
    cout << "DROP TABLE command recognized.\n";
    break;
  case CREATE_TABLE:
    cout << "CREATE TABLE command recognized.\n";
    break;
  case SELECT:
    cout << "SELECT command recognized.\n";
    break;
  default:
    cout << "Unknown or invalid SQL command.\n";
  }
}