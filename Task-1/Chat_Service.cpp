#include "Chat_Service.h"
#include <iostream>

int ChatService::createUser(const std::string &name, bool isOnline, UserRole role)
{

  User user(name, isOnline, role);
  m_data_manager.addUser(user);
  return user.getId();
}


void ChatService::deleteUser(const int user_id)
{

  m_data_manager.deleteUser(user_id);
}


int ChatService::sendMessage(int user_id, int session_id, const std::string &content)
{

  Message msg(content, user_id);
  m_data_manager.addMessage(msg, user_id, session_id);
  return msg.getId();
}


void ChatService::deleteMessage(const int message_id, const int user_id, const int session_id)
{
  m_data_manager.deleteMessage(message_id, user_id, session_id);
}


int ChatService::addSession()
{
  Session newSession;
  m_data_manager.addSession(newSession);
  return newSession.getId();
}


void ChatService::joinSession(const int user_id, const int session_id)
{
  m_data_manager.addUsertoSession(user_id, session_id);
}


void ChatService::leaveSession(const int user_id, const int session_id)
{
  m_data_manager.deleteUserfromSession(user_id, session_id);
}


void ChatService::deleteSession(const int session_id)
{
  m_data_manager.deleteSession(session_id);
}


const Data_Manager &ChatService::getDataManager()
{
  return m_data_manager;
}