#pragma once
#include <iostream>
#include <string>

enum class UserRole
{
  ADMIN,
  NONE
};

class User
{
private:
  std::string m_name;
  bool m_isOnline;
  UserRole m_role = UserRole::NONE;
  int m_id;
  static int m_static_id;
  int m_session_id=-1;

public:
  User(const std::string &name = "", bool status = true, UserRole role = UserRole::NONE) : m_name{name}, m_isOnline{status}, m_role{role}, m_id{m_static_id++}
  {
  }

  const std::string &getName() const;

  void setName(const std::string& name);

  const int getId() const;

  bool getStatus() const;

  void toggleStatus();

  UserRole getRole() const;

  const int getSessionId() const;

  void setSessionId(const int session_id);
};
