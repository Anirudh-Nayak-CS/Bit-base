#include <iostream>
#include "Data_Manager.h"


const User &Data_Manager::getUser(const int user_id) const
{

  return *user_map.at(user_id);
}


const Message &Data_Manager::getMessage(const int message_id) const
{

  return *message_map.at(message_id);
}


const Session &Data_Manager::getSession(const int session_id) const
{

  return *session_map.at(session_id);
}


void Data_Manager::addUser(const User &user)
{
  const int userid = user.getId();
  user_map.emplace(userid,std::make_unique<User>(user));
}


void Data_Manager::addUsertoSession(const int user_id, const int session_id)
{
  auto it = session_map.find(session_id);
  if (it == session_map.end())
    return;
  Session &currSession = *session_map[session_id];
  auto it2 = user_map.find(user_id);
  if (it2 == user_map.end())
    return;
  User &user = *user_map.at(user_id);
  user.setSessionId(session_id);
  currSession.addUser(user_id);
}


void Data_Manager::addMessage(const Message &msg, const int user_id, const int session_id)
{
  const int msgId = msg.getId();
  message_map.emplace(msgId,std::make_unique<Message>(msg));
  auto it = session_map.find(session_id);
  if (it == session_map.end())
    return;
  Session &currSession = *session_map.at(session_id);
  currSession.addMessage(msgId, user_id);
}


void Data_Manager::addSession(const Session &session)
{
  const int sessionid = session.getId();
  session_map.emplace(sessionid,std::make_unique<Session>(session));
}


void Data_Manager::deleteUserfromSession(const int user_id, const int session_id)
{
  auto it = session_map.find(session_id);
  if (it == session_map.end())
    return;
  Session &currSession = *it->second;

  currSession.deleteUser(user_id);
}


void Data_Manager::deleteUser(const int user_id)
{
  auto it = user_map.find(user_id);
  if (it == user_map.end())
    return;

  User &user = *it->second;
  int session_id = user.getSessionId();
  if (session_id != -1)
  {
    deleteUserfromSession(user_id, session_id);
  }

  user_map.erase(user_id);
}


void Data_Manager::deleteMessage(const int message_id, const int user_id, const int session_id)
{
  if (message_map.find(message_id) == message_map.end())
    return;
  message_map.erase(message_id);

  if (session_map.find(session_id) == session_map.end())
    return;
  Session &session = *session_map.at(session_id);
  session.deleteMessage(message_id, user_id);
}


void Data_Manager::deleteSession(const int session_id)
{
  if (session_map.find(session_id) == session_map.end())
    return;
  session_map.erase(session_id);
}
