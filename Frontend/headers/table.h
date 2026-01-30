#include <bits/stdc++.h>
#pragma once
struct Row
{

  char username[32];
  char email[255];

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
  }
};

extern std::map<uint32_t, Row *> table;