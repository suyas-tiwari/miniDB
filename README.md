# MiniDB 🚀

A lightweight, high-performance, single-threaded Key-Value store written entirely in C++ using POSIX sockets and the official Redis Serialization Protocol (RESP).

## 🧠 Architecture Overview

MiniDB was built to deeply understand network programming, protocol parsing, and in-memory data management. It bypasses high-level networking libraries in favor of raw system calls to build a database from the ground up.

- **Networking Layer:** Custom TCP server using POSIX sockets (`<sys/socket.h>`), featuring a persistent event-loop to handle long-lived client connections without dropping sockets.
- **Protocol Parser:** Engineered a custom serialization parser strictly adhering to the Redis Serialization Protocol (RESP). It is robust enough to handle byte-level parsing, trailing buffer garbage, and precise string-to-integer extraction.
- **Storage Engine:** Decoupled, Object-Oriented storage layer utilizing standard library hash maps (`std::unordered_map`) for constant O(1) time complexity reads and writes.

## ⚡ Supported Commands

MiniDB seamlessly integrates with the official `redis-cli`, proving its protocol compliance.

| Command | Time Complexity | Description                     | Example                           |
| :------ | :-------------- | :------------------------------ | :-------------------------------- |
| `PING`  | O(1)            | Tests the active TCP connection | `PING` ➡️ `+PONG`                 |
| `ECHO`  | O(1)            | Returns the provided string     | `ECHO hello` ➡️ `$5\r\nhello\r\n` |
| `SET`   | O(1)            | Stores a string value via a key | `SET user suyas` ➡️ `+OK`         |
| `GET`   | O(1)            | Retrieves a string value by key | `GET user` ➡️ `$5\r\nsuyas\r\n`   |

## 🛠️ How to Build and Run

**1. Compile the Engine**
`g++ main.cpp resp_parser.cpp server.cpp database.cpp -o minidb`

**2. Start the Server**
`./minidb`

**3. Connect as a Client**
Open a new terminal and use the official Redis CLI to interface with the database:
`redis-cli -p 6739`

## 💡 Technical Challenges & Learnings

- **Memory & State Management:** Successfully refactored a monolithic server loop into a clean OOP architecture, safely isolating the network handler from the private database engine.
- **Protocol Quirks (RESP):** Navigated the complexities of byte-level parsing, specifically handling implicit carriage returns (`\r\n`) and avoiding out-of-bounds memory errors during extraction.
- **Connection Persistence:** Evolved the server from an ephemeral request/response model to a persistent daemon capable of handling sustained client sessions, overcoming the limitations of standard blocking reads.
