#pragma once
#include <iostream>
#include <ctime>

class Message
{
private:
  std::string m_content;
  int m_messageId;
  int m_senderId;
  time_t m_sendTime;
  static int m_static_messageId;

public:
  Message(const std::string content="",const int senderId=-1) : m_content{content}, m_messageId{m_static_messageId++},m_senderId{senderId}
  {
    time(&m_sendTime);
  }

  const int getId() const;

  const int getSenderId() const;

  const std::string &getContent() const;

  void setContent(const std::string &new_content);
};
