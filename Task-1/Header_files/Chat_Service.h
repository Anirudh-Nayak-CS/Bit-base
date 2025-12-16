#pragma once
#include <iostream>
#include <string>
#include "Data_Manager.h"

class ChatService
{
private:
  Data_Manager& m_data_manager;

public:
  ChatService(Data_Manager& dm) : m_data_manager{dm}
  {
  }

  // User related logic
  int createUser(const std::string &name="", bool isOnline = true, UserRole role = UserRole::NONE);

  void deleteUser(const int user_id);

  // Message related logic

  int sendMessage(const int user_id,const int session_id,const std::string& content="");

  void deleteMessage(const int message_id,const int user_id,const int session_id);

  // Session related logic

  int addSession();

  void joinSession(const int user_id,const int session_id);

  void leaveSession(const int user_id,const int session_id);

  void deleteSession(const int session_id);

  const Data_Manager& getDataManager();
};