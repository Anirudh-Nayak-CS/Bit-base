#include <iostream>
#include <vector>
#include "Chat_Service.h"

int main()
{
    std::cout << "=== Starting Chat System ===\n";

    // 1. Initialize the Service
    // We pass an empty Data_Manager to initialize the service
    Data_Manager initial_dm;
    ChatService chatApp(initial_dm);

    // 2. Create Users
    // Alice and Bob are standard users
    int userAlice = chatApp.createUser("Alice", true);
    int userBob = chatApp.createUser("Bob", true);
    
    std::cout << "Created Users: Alice (ID: " << userAlice << "), Bob (ID: " << userBob << ")\n";

    // 3. Create a Session (Chat Room)
    int sessionID = chatApp.addSession();
    std::cout << "Created Session (ID: " << sessionID << ")\n";

    // 4. Users join the session
    chatApp.joinSession(userAlice, sessionID);
    chatApp.joinSession(userBob, sessionID);
    std::cout << "Alice and Bob joined the session.\n\n";

    // 5. Send Messages (Simulating a conversation)
    chatApp.sendMessage( userAlice, sessionID,"Hello Bob! Can you hear me?");
    chatApp.sendMessage(userBob, sessionID,"Hey Alice! Yes, loud and clear.");
    chatApp.sendMessage(userAlice, sessionID,"Great. This C++ chat system is working!");
    
    std::cout << "--- Messages Sent ---\n\n";

    // 6. DISPLAY THE CHAT ( The "View" Logic )
    std::cout << "=== CHAT HISTORY (Session " << sessionID << ") ===\n";

    // A. Get access to the read-only data
    const Data_Manager& dm = chatApp.getDataManager();
    const Session& currentSession = dm.getSession(sessionID);
    
    // B. Get the list of messages in this session
    // m_sessionHistory is a set of pairs {messageId, senderId}
    const auto& history = currentSession.getSessionHistory();

    // C. Loop through history and print details
    for (const auto& entry : history) {
        int msgId = entry.first;
        int senderId = entry.second;

        // Retrieve the actual objects using the IDs
        std::string senderName = dm.getUser(senderId).getName();
        std::string content = dm.getMessage(msgId).getContent();

        std::cout << "[" << senderName << "]: " << content << "\n";
    }

    std::cout << "\n=== End of Chat ===\n";

    return 0;
}