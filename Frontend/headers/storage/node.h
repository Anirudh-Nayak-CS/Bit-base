#pragma once
#include <variant>
#include <string>


using Datatype = std::variant<int, long long, double, std::string, std::monostate, bool>;

struct Node
{

  Datatype data;
  Node *right;
  Node *left;
  Node *up;
  Node *down;

  Node(Datatype data) : data(data), right(nullptr), left(nullptr), up(nullptr), down(nullptr)
  {
  }
};