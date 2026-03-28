#include "../../headers/meta/meta.h"
#include <iostream>

Metastatus MetaCommandHandler::handleMetaCommands(const std::string &meta_command)
{
  if (meta_command == ".exit")
  {
    std::cout << "EXITED" << std::endl;
    return Metastatus::META_FAILURE;
  }
  else if (meta_command == ".tables")
  {
    std::cout << "Users" << std::endl;
    return Metastatus::  META_SUCCESS;
  }
  else if (meta_command == ".help")
  {
    std::cout << "--- Meta Commands ---" << std::endl;
    std::cout << "  .exit      : Exit the database REPL" << std::endl;
    std::cout << "  .help      : Print this message" << std::endl;
    std::cout << "  .tables    : List all tables" << std::endl;

    std::cout << "\n--- SQL Commands ---" << std::endl;
    std::cout << "  insert <id> <username> <email> <age> <gender> : Add a new row" << std::endl;
    std::cout << "  select                                        : Print all rows" << std::endl;
    std::cout << "  update <id> <username> <email> <age> <gender> : Modify an existing row" << std::endl;
    std::cout << "  delete <id>                                   : Remove a row" << std::endl;
    std::cout << "  create table <name>                           : Create a new table" << std::endl;
    std::cout << "  drop table <name>                             : Delete a table" << std::endl;

    return Metastatus::META_SUCCESS;
  }
  else
  {
    std::cout << "Unrecognized command .cmd" << std::endl;
    return Metastatus::META_UNRECOGNIZED;
  }
}