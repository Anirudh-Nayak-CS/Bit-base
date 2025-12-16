#include <iostream>
#include "Session.h"

int Session::m_staticSessionId = 0;

const int Session::getId() const
{
  return m_sessionId;
}

void Session::addUser(const int user_id)
{

  m_users.insert(user_id);
}

void Session::deleteUser(const int user_id)
{
  m_users.erase(user_id);
}

void Session::addMessage(const int message_id, const int sender_id)
{

  m_sessionHistory.insert({message_id, sender_id});
}

void Session::deleteMessage(const int message_id, const int sender_id)
{

  m_sessionHistory.erase({message_id, sender_id});
}

const std::set<std::pair<int,int>>& Session::getSessionHistory() const {
    return m_sessionHistory;
}

const std::set<int>&  Session::getUserList() const {
    return m_users;
 }