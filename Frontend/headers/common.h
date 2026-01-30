#pragma once
#define MAX_SIZE 100
#include "table.h"

typedef enum
{
  INSERT,
  SELECT,
  DELETE,
  UPDATE,
  CREATE_TABLE,
  DROP_TABLE,
  UNDEFINED
} SQLcommandType;

typedef enum
{
  SYNTAX_ERROR,
  NEGATIVE_INT,
  STRING_TOO_LONG,
  ENTRY_NOT_FOUND,
  SUCCESS,
  FAILURE,
} Commandstatus;
