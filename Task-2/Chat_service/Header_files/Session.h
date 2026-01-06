#pragma once
#include <iostream>
#include <vector>
#include <set>
#include <mutex>

class Session
{
private:
  int m_sessionId;
  static int m_staticSessionId;
  std::set<std::pair<int, int>> m_sessionHistory;
  std::set<int> m_users;
  mutable std::mutex session_mtx;

public:
  Session() : m_sessionId{m_staticSessionId++}
  {
  }

 
  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;
  Session(Session &&) = default;
  Session &operator=(Session &&) = default;

  const int getId() const;

  void addUser(const int user_id);

  void addMessage(const int message_id, const int sender_id);

  void deleteUser(const int user_id);

  void deleteMessage(const int message_id, const int sender_id);

  const std::set<std::pair<int, int>> &getSessionHistory() const;

  const std::set<int> &getUserList() const;
};
