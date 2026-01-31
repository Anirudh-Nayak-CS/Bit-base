#pragma once
#include <bits/stdc++.h>
#include "common.h"
struct Row
{

  char username[32];
  char email[255];
  int age=0;
  gender sex=NOT_DEFINED;
  bool is_active=false;
  char created_at[80];
  char updated_at[80];

  Row(char *user_name, char *_email)
  {

    strncpy(username, user_name, 32);
    strncpy(email, _email, 255);
    username[31] = '\0';
    email[254] = '\0';
  }
  Row()
  {
    username[0] = '\0';
    email[0] = '\0';
    created_at[0]='\0';
    updated_at[0]='\0';
    
  }
};

extern std::map<uint32_t, Row *> table;