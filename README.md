# MiniDB

A Redis-compatible, persistent key-value store written in C++ from scratch.
Built to understand how databases, network protocols, and I/O systems actually
work under the hood — no networking libraries, no shortcuts.

Connects directly with `redis-cli` out of the box.

## Architecture

**Networking — epoll-based I/O multiplexing**
A single thread manages multiple concurrent client connections using Linux's
`epoll` API. Each client gets a persistent socket that stays open across
commands, mimicking how real Redis handles connections.

**Protocol — custom RESP parser**
Implements the Redis Serialization Protocol (RESP) from scratch at the byte
level. Parses incoming bulk string arrays by scanning for `\r\n` delimiters
and extracting exact byte lengths — no regex, no string splitting.

**Persistence — Append Only File (AOF)**
Every write command is serialized back into RESP format and appended to
`data.aof`. On startup, the file is replayed line by line to reconstruct the
in-memory state, exactly how Redis AOF persistence works internally.

**Storage — O(1) in-memory store**
`std::unordered_map` backed key-value store, fully decoupled from the network
layer behind a clean Database class interface.

## Supported Commands

| Command                 | Description                            |
| :---------------------- | :------------------------------------- |
| `PING`                  | Returns PONG, tests the connection     |
| `ECHO <msg>`            | Returns the message back               |
| `SET <key> <value>`     | Stores a key-value pair                |
| `GET <key>`             | Retrieves value by key                 |
| `EXISTS <key>`          | Returns 1 if key exists, 0 otherwise   |
| `DEL <key>`             | Deletes a key, returns 1 on success    |
| `RENAME <key> <newkey>` | Renames a key                          |
| `INCR <key>`            | Atomically increments an integer value |

## Build & Run

```bash
g++ main.cpp resp_parser.cpp server.cpp database.cpp -o minidb
./minidb
```

Connect using redis-cli:

```bash
redis-cli -p 6739
```

## What's next

- Async write queue — decouple disk writes from the main event loop so SSD
  latency doesn't block command processing
- Idle connection cleanup — close sockets inactive for over 5 minutes to
  prevent file descriptor exhaustion
