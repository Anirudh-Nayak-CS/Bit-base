#include <iostream>
#include <unordered_map>
#include <memory>
#include "User.h"
#include "Session.h"
#include "Message.h"
#include<mutex>

class Data_Manager
{

  std::unordered_map<int, std::unique_ptr<User>> user_map;
  std::unordered_map<int, std::unique_ptr<Session>> session_map;
  std::unordered_map<int,std::unique_ptr<Message>> message_map;
  mutable std::recursive_mutex data_mtx;


public:
  Data_Manager()
  {
  }

  const User &getUser(const int user_id) const;

  const Message &getMessage(const int message_id) const;

  const Session &getSession(const int session_id) const;

  void addUser(const User &user);

  void addUsertoSession(const int user_id,const int session_id);

  void addMessage(const Message &msg,const int user_id,const int session_id);

  int createSession();

  void deleteUser(const int user_id);

  void deleteUserfromSession(const int user_id,const int session_id);

  void deleteMessage(const int message_id,const int user_id,const int session_id);

  void deleteSession(const int session_id);
};
