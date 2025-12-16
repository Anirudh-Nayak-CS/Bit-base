#include <iostream>
#include "Message.h"

int Message::m_static_messageId = 0;

const int Message::getId() const
{
  return m_messageId;
}

const int Message::getSenderId() const
{
  return m_senderId;
}

const std::string &Message::getContent() const
{
  return m_content;
}

void Message::setContent(const std::string &new_content)
{
  m_content = new_content;
}
