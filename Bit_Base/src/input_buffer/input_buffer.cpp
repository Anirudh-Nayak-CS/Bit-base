
#include "../../headers/input_buffer/input_buffer.h"
#include <stdexcept>

string inputBuffer::getInputBuffer()
{
  return m_buffer;
}

void inputBuffer::read_input()
{
  if (!std::getline(cin, m_buffer)) {
    throw std::runtime_error("Failed to read input from stdin");
  }
 
}
