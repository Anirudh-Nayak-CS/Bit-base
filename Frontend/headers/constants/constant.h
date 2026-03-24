#pragma once
#define MAX_SIZE 100

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
  CMD_SYNTAX_ERROR,
  CMD_NEGATIVE_INT,
  CMD_STRING_TOO_LONG,
  CMD_ENTRY_NOT_FOUND,
  CMD_SUCCESS,
  CMD_FAILURE,
  CMD_OUT_OF_RANGE,
} Commandstatus;

typedef enum
{
  META_SUCCESS,
  META_FAILURE,
  META_UNRECOGNIZED
} Metastatus;

typedef enum
{
  MALE,
  FEMALE,
  OTHER,
  NOT_DEFINED
} gender;
