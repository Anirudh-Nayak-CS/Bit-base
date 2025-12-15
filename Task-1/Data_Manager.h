#include <iostream>
#include <map>
#include "User.h"
#include "Session.h"
#include "Message.h"

class Data_Manager
{

  std::map<int, User> user_map;
  std::map<int, Session> session_map;
  std::map<int, Message> message_map;

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

  void addSession(const Session &session);

  void deleteUser(const int user_id);

  void deleteUserfromSession(const int user_id,const int session_id);

  void deleteMessage(const int message_id,const int user_id,const int session_id);

  void deleteSession(const int session_id);
};
