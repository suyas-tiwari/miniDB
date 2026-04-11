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

**Persistence — Async AOF Writer**
Write commands are buffered in memory and flushed to disk every second
by a dedicated background thread. The main thread never waits for disk
I/O — it updates the in-memory store, appends the RESP string to a
shared buffer, and moves on. A mutex protects the buffer during the
swap, held for nanoseconds. This brings SET/INCR performance from
~13,000 req/sec to ~40,000 req/sec at the cost of at most 1 second
of data loss on an unclean shutdown — the same tradeoff Redis makes
with `appendfsync everysec`.

**Storage — O(1) in-memory store**
`std::unordered_map` backed key-value store, fully decoupled from the network
layer behind a clean Database class interface.

## Supported Commands

| Command                 | Description                                 |
| :---------------------- | :------------------------------------------ |
| `PING`                  | Returns PONG, tests the connection          |
| `ECHO <msg>`            | Returns the message back                    |
| `SET <key> <value>`     | Stores a key-value pair                     |
| `GET <key>`             | Retrieves value by key                      |
| `EXISTS <key>`          | Returns 1 if key exists, 0 otherwise        |
| `DEL <key>`             | Deletes a key, returns 1 on success         |
| `RENAME <key> <newkey>` | Renames a key                               |
| `INCR <key>`            | Atomically increments an integer value      |
| `DBSIZE`                | Returns the total number of key-value pairs |

## Benchmarks

Tested on GitHub Codespaces (2-core), compared against official Redis.

| Command | v1.0            | v1.1            | Redis          |
| :------ | :-------------- | :-------------- | -------------- |
| GET     | ~42,889 req/sec | ~40,162 req/sec | 39,161 req/sec |
| SET     | ~13,085 req/sec | ~37,890 req/sec | 37,989 req/sec |
| INCR    | ~13,403 req/sec | ~39,427 req/sec | 35,547 req/sec |

SET/INCR in v1.0 were bottlenecked by synchronous fsync on every
command. v1.1 batches writes every second via a background thread,
eliminating disk I/O from the critical path.

GET dropped slightly in v1.1 due to background thread activity —
both versions comfortably outperform Redis on reads.

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

- **Idle connection cleanup** — close sockets inactive for over 5 minutes to
  prevent file descriptor exhaustion
- **AOF Compaction** — periodically rebuild the AOF file directly from
  the in-memory store to prevent unbounded growth from repeated INCR
  and RENAME commands
