# Socket Queue & Backlog Mechanism

**Part of the networking documentation suite** — [← Back to main pipeline](01_WORKFLOW_PIPELINE.md)

Detailed explanation of the `backlog` parameter in `listen()` system call.

---

## Concept Overview

The **backlog** parameter in `listen(socket_fd, backlog)` specifies the maximum number of client connections that can be waiting in the queue for acceptance by the server.

```
listen(socket_fd, backlog)
        └─► Maximum pending (accepted but not yet handled) connections
```

---

## TCP Connection Establishment

### 3-Way Handshake Flow

```
CLIENT                              SERVER
  │                                   │
  ├── SYN ───────────────────────────►│
  │                                   │ [SYN received]
  │                                   │
  │◄─────────────────────── SYN-ACK ──┤
  │ [ACK received]                    │
  │                                   │
  ├── ACK ───────────────────────────►│
  │                                   │ [Connection in queue]
  │                                   ├─► Accept queue (size = backlog) f.ex: 10, 128 
  │                                   │
  │                                   └─► accept() pulls from queue
  │◄─── Connection established ───────┤
```

---

## Queue States

### When Connection Completes (3-Way Handshake)

1. **SYN Received**: Connection in incomplete state (SYN_RECV)
2. **ACK Received**: Connection moves to **Accept Queue** (ESTABLISHED)
3. **accept()**: Application pulls connection from queue

### Backlog Affects Accept Queue

```
accept() ────────────┐
                     │
                     ├─ Pull connection from queue
                     │
                     └────────────┬─────────────────┬─────────── ...
                                  │					│
                    Waiting conn 1│ Waiting conn 2	│ (backlog = 10)
                                  │					│
                                  ├─────────────────┤
```

---

## Your Code

```cpp
// In Server::listenSocket()
void Server::listenSocket() {
    if (listen(serverSocketFd_, 10) < 0) {  // backlog = 10
        close(serverSocketFd_);
        throw std::runtime_error("listen failed");
    }
    std::cout << "[Server] Listening backlog=10" << std::endl;
}
```

### What This Means

| Scenario | Behavior |
|----------|----------|
| Connections **1-10** | Accepted into queue, wait for `accept()` |
| Connection **11** | Rejected with RST (connection reset) |
| After `accept()` on conn 1 | Queue size becomes 9, conn 11 can now join |

---

## Behavior Illustration

```
Time: T                    Time: T+1               Time: T+2
─────────────────────────────────────────────────────────────
Queue: [1][2][3]          Queue: [2][3]           Queue: [3][11]
       [4][5][6]                 [4][5]                  [4][5]
       [7][8][9]                 [6][7]                  [6][7]
       [10]                      [8][9]                  [8][9]
                                 [10]                    [10]
                                                         
Status: Full (10/10)      accept(conn 1)          New conn 11
        No new conn        called, queue           accepted
        can join           decreases               (queue 9/10)
```

---

## Why Backlog Matters

### High-Traffic Scenarios

```
Client A connects ──┐
Client B connects   ├──► All 10 slots fill
Client C connects   │
...
Client J connects ──┘

Client K connects ──► REJECTED (no backlog space)
                    └─► Must retry later or fail
```

### Server Not Calling accept() Fast

```
Server overloaded, not calling accept() frequently:
  • Queue fills up quickly
  • New clients get RST (rejected)
  • Connection establishment fails
  
Solution: 
  • Call accept() more frequently
  • Increase backlog to temporary buffer delays
  • Use async/non-blocking pattern (like your IRC server!)
```

### Your IRC Server Pattern

```cpp
// In main event loop (processEvents)
for (each event) {
    if (fd == serverSocketFd_) {
        handleNewConnection()  // ← accept() called here
            └─► accept(serverSocketFd_, ...)
}

// With poll(timeout=1000ms):
// accept() is called every time socket becomes ready
// Backlog absorbs brief delays between poll() calls
```

---

## Recommended Values

### By Use Case

| Scenario | Backlog | Explanation |
|----------|---------|-------------|
| **Development/Testing** | 10 | Sufficient for manual connections |
| **Small Server** | 32 | Comfortable buffer for few connections |
| **Normal Server** | 128 | Standard (Linux default) |
| **High-Load Server** | 256+ | Absorb connection spikes |
| **Maximum Portable** | `SOMAXCONN` | System-defined maximum |

### Linux System Default

```bash
# Check system maximum
cat /proc/sys/net/core/somaxconn

# On most Linux systems: 128
# On some high-performance systems: 4096
```

---

## Best Practice for Your Code

### Current (OK for testing)
```cpp
listen(serverSocketFd_, 10);
```

### Recommended (production)
```cpp
#include <sys/socket.h>

// Use system maximum
listen(serverSocketFd_, SOMAXCONN);

// Or specific value for IRC
listen(serverSocketFd_, 64);  // IRC typically has moderate connect rate
```

### Hybrid Approach
```cpp
// Define at top of Server.hpp
#ifndef LISTEN_BACKLOG
# define LISTEN_BACKLOG 128
#endif

// In Server::listenSocket()
if (listen(serverSocketFd_, LISTEN_BACKLOG) < 0) {
    // ...
}
```

---

## Connection Rejection Behaviors

When backlog is full and new connection arrives:

### Option 1: Hard Reject (Default)
```
Client sends SYN ──► Server (queue full) ──► Send RST
                     └─► No accept() queue space
```

### Option 2: SYN Queue Overflow (older kernels)
```
Some systems keep separate SYN queue:
  - listen(fd, backlog)
  - SYN received → SYN queue
  - ACK received → accept queue
  - backlog primarily affects accept queue
```

### Option 3: SYN Cookies (modern kernels)
```
Many modern systems use SYN cookies to protect against SYN flood:
  - Server doesn't allocate space for SYN
  - Responds with cookie in SYN-ACK
  - Client echoes cookie in ACK
  - Server verifies then accepts
```

---

## Monitoring & Debugging

### Check Queue Status (Linux)

```bash
# Monitor listening socket stats
ss -ltn | grep LISTEN

# Example output:
# Recv-Q Send-Q Local Address      Foreign Address    State
# 0      10     0.0.0.0:6667       0.0.0.0:*          LISTEN

# Recv-Q = current queue size
# Send-Q = backlog value
```

### Under Your IRC Server

```
Before accept():  Recv-Q = 3  (3 connections waiting)
After accept():   Recv-Q = 2  (1 accepted and handled)
```

---

## Connection Flow in Your IRC Server

```
1. New client sends PASS command
   └─► TCP handshake completes
   └─► Connection added to accept queue (uses 1 backlog slot)

2. Poller::poll() detects serverSocketFd_ has POLLIN
   └─► processEvents() called
   └─► handleNewConnection() called
   └─► accept() pulls from queue (frees 1 backlog slot)

3. Client object created
   └─► Registration flow begins (PASS → NICK → USER)

4. Multiple clients connected:
   ┌─► Client 1: accepted, in USER command
   ├─► Client 2: accepted, waiting for PASS
   ├─► Client 3: just accepted
   └─► Client 4: in backlog queue (TCP handshake done, awaiting accept())
```

---

## Summary

- **Backlog = 10**: Maximum 10 completed TCP connections waiting for `accept()`
- **Exceeding backlog**: New connections get RST (rejected)
- **Your server**: Good for testing, fine for small deployments
- **Production**: Consider `SOMAXCONN` or higher value
- **Non-blocking pattern**: Your server handles this well with event-driven `accept()`

---

##  Related Documentation

| Document | Topic |
|---|---|
| [01_WORKFLOW_PIPELINE.md](01_WORKFLOW_PIPELINE.md) | Complete call chain showing where backlog is used |
| [03_v4_v6_in_socket_VERIFIED_AGAINST_SUBJECT.md](03_v4_v6_in_socket_VERIFIED_AGAINST_SUBJECT.md) | Socket creation, poll() mechanism, and non-blocking I/O |
| [0_TERMS_MEANING.md](0_TERMS_MEANING.md) | Hardware interrupts and TCP handshake low-level details |

