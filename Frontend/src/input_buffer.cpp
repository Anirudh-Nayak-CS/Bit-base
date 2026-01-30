
#include "input_buffer.h"

string inputBuffer::getInputBuffer()
{
  return m_buffer;
}

void inputBuffer::read_input()
{

  std::getline(cin, m_buffer);

  if (m_buffer.empty())
  {
    cout << "Error reading input" << endl;
    exit(EXIT_SUCCESS);
  }
}
