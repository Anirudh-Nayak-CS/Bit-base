#include <iostream>
#include <vector>
#include "Chat_Service.h"

int main()
{
    std::cout << "=== Starting Chat System ===\n";

    // 1. Initialize the Service
    Data_Manager initial_dm;
    ChatService chatApp(initial_dm);

    // 2. Creating users
    int userAlice = chatApp.createUser("Alice", true);
    int userBob = chatApp.createUser("Bob", true);
    int userAliceDup = chatApp.createUser("Alice", false); // duplicate name
    std::cout << "Created Users: Alice (ID: " << userAlice << "), Bob (ID: " << userBob << "), AliceDup (ID: " << userAliceDup << ")\n";

    // 3. Create Sessions
    int sessionID = chatApp.addSession();
    int sessionID2 = chatApp.addSession();
    std::cout << "Created Sessions: " << sessionID << ", " << sessionID2 << "\n";

    // 4. Users join sessions (including invalid cases)
    chatApp.joinSession(userAlice, sessionID);
    chatApp.joinSession(userBob, sessionID);
    chatApp.joinSession(userAliceDup, sessionID2);
    std::cout << "Alice and Bob joined session " << sessionID << ". AliceDup joined session " << sessionID2 << ".\n";

    // Try joining with invalid user/session
    chatApp.joinSession(999, sessionID); // invalid user
    chatApp.joinSession(userAlice, 888); // invalid session

    // 5. Send Messages (including edge cases)
    chatApp.sendMessage(userAlice, sessionID, "Hello Bob! Can you hear me?");
    chatApp.sendMessage(userBob, sessionID, "Hey Alice! Yes, loud and clear.");
    chatApp.sendMessage(userAlice, sessionID, "Great. This C++ chat system is working!");
    chatApp.sendMessage(userAliceDup, sessionID2, "Hi, I'm the other Alice!");

    // Try sending from user not in session
    chatApp.sendMessage(userBob, sessionID2, "Should not be delivered");
    // Try sending to non-existent session
    chatApp.sendMessage(userAlice, 888, "No such session");

    std::cout << "--- Messages Sent ---\n\n";

    // 6. Delete a user and try to access
    chatApp.deleteUser(userAliceDup);
    std::cout << "Deleted AliceDup (ID: " << userAliceDup << ")\n";
    // Try to send message as deleted user
    chatApp.sendMessage(userAliceDup, sessionID2, "Should not work");

    // 7. Delete a session and try to use it
    chatApp.deleteSession(sessionID2);
    std::cout << "Deleted session " << sessionID2 << "\n";
    chatApp.sendMessage(userAlice, sessionID2, "Session deleted");

    // 8. Try deleting non-existent message
    chatApp.deleteMessage(999, userAlice, sessionID);

    // 9. Create and delete users/sessions in a loop
    std::vector<int> tempUsers;
    for (int i = 0; i < 5; ++i) {
        int uid = chatApp.createUser("TempUser" + std::to_string(i), true);
        tempUsers.push_back(uid);
    }
    int tempSession = chatApp.addSession();
    for (int uid : tempUsers) chatApp.joinSession(uid, tempSession);
    for (int uid : tempUsers) chatApp.deleteUser(uid);
    chatApp.deleteSession(tempSession);


    // 11. Try deleting the same user and session twice
    chatApp.deleteUser(userAliceDup); // already deleted
    chatApp.deleteSession(sessionID2); // already deleted

    // 12. Try leaving a session not joined
    chatApp.leaveSession(userAliceDup, sessionID); // user deleted
    chatApp.leaveSession(userBob, sessionID2); // session deleted
    chatApp.leaveSession(999, sessionID); // invalid user
    chatApp.leaveSession(userAlice, 888); // invalid session
   
 
    // 13. User joins, leaves, and rejoins
    int userCharlie = chatApp.createUser("Charlie", true);
    chatApp.joinSession(userCharlie, sessionID);
    chatApp.leaveSession(userCharlie, sessionID);
   
    chatApp.joinSession(userCharlie, sessionID);
    chatApp.sendMessage(userCharlie, sessionID, "I'm back!");
  
    // 14. Try sending a message after leaving
    chatApp.leaveSession(userCharlie, sessionID);
    chatApp.sendMessage(userCharlie, sessionID, "Should not be delivered");
   

    // 15. Try deleting a user in a session, then send message
    int userDave = chatApp.createUser("Dave", true);
    chatApp.joinSession(userDave, sessionID);
    chatApp.deleteUser(userDave);
    chatApp.sendMessage(userDave, sessionID, "Should not work");

    // 16. Try creating a session, deleting it, and joining
    int tempSession2 = chatApp.addSession();
    chatApp.deleteSession(tempSession2);
    chatApp.joinSession(userAlice, tempSession2);

    // 17. Try deleting a session with users in it
    int userEve = chatApp.createUser("Eve", true);
    int sessionEve = chatApp.addSession();
    chatApp.joinSession(userEve, sessionEve);
    chatApp.deleteSession(sessionEve);
    chatApp.sendMessage(userEve, sessionEve, "Should not work");

    std::cout << "=== CHAT HISTORY (Session " << sessionID << ") ===\n";
    const Data_Manager& dm = chatApp.getDataManager();
    const Session& currentSession = dm.getSession(sessionID);
    const auto& history = currentSession.getSessionHistory();
    for (const auto& entry : history) {
        int msgId = entry.first;
        int senderId = entry.second;
        std::string senderName = dm.getUser(senderId).getName();
        std::string content = dm.getMessage(msgId).getContent();
        std::cout << "[" << senderName << "]: " << content << "\n";
    }
    std::cout << "\n=== End of Chat ===\n";
    return 0;
}