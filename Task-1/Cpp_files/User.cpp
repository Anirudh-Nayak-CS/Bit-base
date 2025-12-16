#include <iostream>
#include <string>
#include "User.h"

int User::m_static_id = 1;

const std::string &User::getName() const
{
  return m_name;
}

void User::setName(const std::string& name)
{
  m_name = name;
}

const int User::getId() const
{
  return m_id;
}

bool User::getStatus() const
{
  return m_isOnline;
}

void User::toggleStatus()
{
  m_isOnline = !m_isOnline;
}

UserRole User::getRole() const
{
  return m_role;
}

const int User::getSessionId() const
{
  return m_session_id;
}

void User::setSessionId(const int session_id) {
  m_session_id=session_id;
}