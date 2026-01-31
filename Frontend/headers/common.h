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
  SYNTAX_ERROR,
  NEGATIVE_INT,
  STRING_TOO_LONG,
  ENTRY_NOT_FOUND,
  SUCCESS,
  FAILURE,
  OUT_OF_RANGE,
} Commandstatus;

typedef enum {
 MALE,
 FEMALE,
 OTHER,
 NOT_DEFINED,
}gender;
