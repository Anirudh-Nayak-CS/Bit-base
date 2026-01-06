#include <iostream>
#include "../Header_files/Data_Manager.h"
#include<mutex>

const User &Data_Manager::getUser(const int user_id) const
{
  std::lock_guard< std::recursive_mutex> lock(data_mtx);
  auto it2 = user_map.find(user_id);
  if (it2 == user_map.end())
  {
    throw std::out_of_range("User_id is invalid\n");
  }
  return *user_map.at(user_id);
}

const Message &Data_Manager::getMessage(const int message_id) const
{
  std::lock_guard< std::recursive_mutex> lock(data_mtx);
  auto it2 = message_map.find(message_id);
  if (it2 == message_map.end())
  {
    throw std::out_of_range("Message_id is invalid\n");
  }
  return *message_map.at(message_id);
}

const Session &Data_Manager::getSession(const int session_id) const
{
  std::lock_guard< std::recursive_mutex> lock(data_mtx);
  auto it2 = session_map.find(session_id);
  if (it2 == session_map.end())
  {
    throw std::out_of_range("Session_id is invalid\n");
  }
  return *session_map.at(session_id);
}

void Data_Manager::addUser(const User &user)
{
  std::lock_guard< std::recursive_mutex> lock(data_mtx);
  const int userid = user.getId();
  user_map.emplace(userid, std::make_unique<User>(user));
}

void Data_Manager::addUsertoSession(const int user_id, const int session_id)
{
  std::lock_guard< std::recursive_mutex> lock(data_mtx);
  auto it = session_map.find(session_id);
  if (it == session_map.end())
  {
    std::cout << "Session not found\n";
    return;
  }

  Session &currSession = *session_map[session_id];
  auto it2 = user_map.find(user_id);
  if (it2 == user_map.end())
  {
    std::cout << "User not found\n";
    return;
  }

  User &user = *user_map.at(user_id);

  int user_session_id = user.getSessionId();

  if (user_session_id != -1)
  {
    std::cout << "User is already a  part of session\n";
    return;
  }
  user.setSessionId(session_id);
  currSession.addUser(user_id);
}

void Data_Manager::addMessage(const Message &msg, const int user_id, const int session_id)
{
  std::lock_guard< std::recursive_mutex> lock(data_mtx);
  const int msgId = msg.getId();
  message_map.emplace(msgId, std::make_unique<Message>(msg));
  auto it2 = user_map.find(user_id);
  if (it2 == user_map.end())
  {
    std::cout << "User_id is invalid\n";
    return;
  }
  auto it = session_map.find(session_id);
  if (it == session_map.end())
  {
    std::cout << "Session not found\n";
    return;
  }

  int user_session_id = (*it2->second).getSessionId();

  if (user_session_id == -1)
  {
    std::cout << "User must be part of a session to send message.\n";
    return;
  }

  Session &currSession = *session_map.at(session_id);
  currSession.addMessage(msgId, user_id);
}

int Data_Manager::createSession()
{
 std::lock_guard< std::recursive_mutex> lock(data_mtx);


  auto new_session = std::make_unique<Session>();

  
  int sessionid = new_session->getId();

  
  session_map.emplace(sessionid, std::move(new_session));

  std::cout << "Session created with ID: " << sessionid << "\n";
  return sessionid;
}

void Data_Manager::deleteUserfromSession(const int user_id, const int session_id)
{
  std::lock_guard< std::recursive_mutex> lock(data_mtx);
  auto it2 = user_map.find(user_id);
  if (it2 == user_map.end())
  {
    std::cout << "User_id is invalid\n";
    return;
  }
  auto it = session_map.find(session_id);
  if (it == session_map.end())
  {
    std::cout << "Session not found\n";
    return;
  }
  User &user = *user_map.at(user_id);

  int user_session_id = user.getSessionId();
  if (user_session_id == -1)
  {
    std::cout << "User isn't part of any session\n";
    return;
  }

  Session &currSession = *it->second;
  user.setSessionId(-1);

  currSession.deleteUser(user_id);
}

void Data_Manager::deleteUser(const int user_id)
{
  std::lock_guard< std::recursive_mutex> lock(data_mtx);
  auto it = user_map.find(user_id);
  if (it == user_map.end())
  {
    std::cout << "User not found\n";
    return;
  }

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
  std::lock_guard< std::recursive_mutex> lock(data_mtx);
  if (message_map.find(message_id) == message_map.end())
  {
    std::cout << "Message not found\n";
    return;
  }

  message_map.erase(message_id);

  auto it2 = user_map.find(user_id);
  if (it2 == user_map.end())
  {
    std::cout << "User_id is invalid\n";
    return;
  }

  if (session_map.find(session_id) == session_map.end())
  {
    std::cout << "Session not found\n";
    return;
  }

  int user_session_id = (*it2->second).getSessionId();

  if (user_session_id == -1)
  {
    std::cout << "User must be part of a session to delete a message.\n";
    return;
  }

  Session &session = *session_map.at(session_id);
  session.deleteMessage(message_id, user_id);
}

void Data_Manager::deleteSession(const int session_id)
{
  std::lock_guard< std::recursive_mutex> lock(data_mtx);
  if (session_map.find(session_id) == session_map.end())
  {
    std::cout << "Session not found\n";
    return;
  }

  session_map.erase(session_id);
}
