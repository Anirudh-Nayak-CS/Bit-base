# 💬 C++ Memory-Based Chat System

A high-performance, in-memory backend for a real-time chat application, built from scratch in C++. This project demonstrates modern backend architecture patterns, including **Service-Layer abstraction**, **Normalized Data Models**, and **Resource Management** using smart pointers.

## 🏗️ Architecture

This project follows a **Service-Manager-Model** architecture, strictly separating business logic from data storage.

### Core Components

1.  **ChatService (The API Layer):**
    * Acts as the public interface for the application.
    * Handles "Business Logic" (e.g., ensuring a user exists before joining a session).
    * **Design Choice:** Decoupled from data storage details; it simply commands the Manager.

2.  **Data_Manager (The Storage Layer):**
    * Acts as an **In-Memory Database**.
    * Manages the lifecycle of all objects (`Users`, `Sessions`, `Messages`).
    * **Optimization:** Uses `std::map` for `O(log n)` lookups by ID.

3.  **The Models (User, Session, Message):**
    * **User:** Represents a client with role-based access (`ADMIN`/`NONE`).
    * **Message:** Immutable data structure containing content, timestamp, and sender ID.
    * **Session:** Represents a chat room. Crucially, it **does not store message strings**. It stores only `MessageIDs`, keeping the memory footprint low.

## 🧠 Key Technical Highlights

### 1. Normalized Data Structure
Instead of storing heavy message strings inside every Session, this project uses a **Normalized Database approach**.
* **Storage:** The `Data_Manager` holds the single "Master Copy" of every message.
* **Reference:** The `Session` class only stores a lightweight list of IDs: `std::set<std::pair<int, int>>`.
* **Benefit:** Reduces memory redundancy and ensures data consistency.

### 2. Separation of Concerns (SoC)
* The `User` class doesn't know about `Session`.
* The `Session` class doesn't know about `ChatService`.
* This prevents "Circular Dependency" issues and makes the code modular and testable.

### 3. Smart Resource Management
* Uses `std::unique_ptr` for memory safety in the entry point.
* Implements **Const Correctness** throughout getters (`const std::string&`) to prevent unnecessary data copying and accidental state mutation.

## 🚀 Getting Started

### Prerequisites
* A C++ Compiler (GCC, Clang, or MSVC) that supports C++14 or later.

### Installation & Run

1.  **Clone the repository**
    ```bash
    git clone [https://github.com/yourusername/cpp-chat-system.git](https://github.com/yourusername/cpp-chat-system.git)
    cd cpp-chat-system
    ```

2.  **Compile the source code**
    ```bash
    g++ *.cpp -o chat_app
    ```

3.  **Run the application**
    ```bash
    ./chat_app
    ```

### Sample Output
```text
=== Starting Chat System ===
Created Users: Alice (ID: 1), Bob (ID: 2)
Created Session (ID: 0)
Alice and Bob joined the session.

--- Messages Sent ---

=== CHAT HISTORY (Session 0) ===
[Alice]: Hello Bob! Can you hear me?
[Bob]: Hey Alice! Yes, loud and clear.
[Alice]: Great. This C++ chat system is working!