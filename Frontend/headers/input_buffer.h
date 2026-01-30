#pragma once
#include <bits/stdc++.h>
using namespace std;
class inputBuffer
{
private:
  string m_buffer;

public:
  inputBuffer() {}

  string getInputBuffer();

  void read_input();
};
