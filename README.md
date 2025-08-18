## What is StreamCache?

**StreamCache** is a high-performance, log-backed, TTL-aware in-memory caching engine implemented in modern C++. It combines the speed of in-memory operations with a persistent append-only log to enable historical data replay, precise time-based key expiration, and efficient eviction.

StreamCache currently operates entirely via a CLI REPL, allowing direct command execution without a client/daemon layer (planned in a future update).

---

## Key Use Cases

StreamCache’s architecture supports multiple production-grade scenarios:

- **Session management** — Store and automatically expire user sessions, authentication tokens, or temporary credentials with TTL.
- **Event sourcing & replay** — Maintain an immutable log of value changes, enabling point-in-time recovery, analytics, or debugging of past states.
- **Real-time leaderboards & counters** — Cache frequently updated numeric data while preserving change history for audit or rollback.
- **API response caching** — Reduce backend load by caching expensive API responses with precise expiration policies.

---

## Key Features

- **SET / GET** — Store and retrieve values by key with low-latency lookups.
- **TTL support** — Automatic expiration of keys after a defined time.
- **Min-heap eviction** — Priority-based removal of expired keys with minimal overhead.
- **Background eviction thread** — Proactively evicts expired keys and cleans key logs in an event driven manner, so reads/writes don't pay cleanup costs.
- **REPLAY** — Retrieve historical values for a key in its TTL window from the log.
- **Append-only log** — Durable in-memory history for every key.
- **CLI REPL** — Direct, command-line interaction with the engine.

---

## Architecture Overview

- **Hash map** — O(1) key lookups.
- **Min-heap** — Efficiently ordered eviction queue by expiration time.
- **Background eviction thread** — Dedicated worker thread that proactively maintains the cache and logs.
- **Append-only log** — Immutable event history per key.
- **Multi-threaded** — REPL runs on the main thread, with eviction offloaded to a background worker.
- **Standard library only** — No external dependencies.

---

## Example CLI Session

```
> SET name Alex 25
> GET name
Value: Alex
> SET name Ashley
> SET name Brian
> GET name
Value: Brian
> REPLAY name
[2025-08-08 12:00:00] Alex
[2025-08-08 12:00:05] Ashley
[2025-08-08 12:00:12] Brian
```

---

## Upcoming Features

- **Sharding** — Horizontal scaling.
- **Disk AOF + snapshot + recovery** — Persistence and crash recovery.
- **INFO / metrics + slowlog + SCAN** — Operational visibility and performance monitoring.

---

## Status

StreamCache is currently in active development. While stable for its core operations, additional capabilities such as persistence, sharding, and advanced monitoring are planned to extend its scalability, fault tolerance, and observability for broader use cases.

---

## 👨‍💻 Author

**Adnan T.** — [@adnant1](https://github.com/adnant1)
