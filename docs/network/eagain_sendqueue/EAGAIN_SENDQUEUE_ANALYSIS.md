# EAGAIN & Send Queue: The Technical Truth

**Version:** 1.0  
**Date:** April 22, 2026 
**Context:** Technical discussion aftermath — resolving eval sheet misinterpretation  
**Scope:** errno handling in `send()`, TCP buffer exhaustion, FLOODBOT validation  
**Related:** `src/Server.cpp` — `sendToClient()`, `flushClientSendQueue()`
**Video and LinkedIN discussion 2 links** at the end of that file
**LinkedIn Manual video** — [EAGAIN issue FLOODBOT](https://www.linkedin.com/feed/update/urn:li:activity:7452598819722174464/)
---

## 1. The Problem — Silent Data Loss in `sendToClient()`

### What Went Wrong (Before)

Before the fix, when `send()` returned `-1` on a full TCP buffer, the message was silently dropped—no log, no queue, gone forever.

```cpp
// BEFORE: silent drop
ssize_t sent = send(clientFd, msg.c_str(), msg.size(), 0);
if (sent < 0 && errno == EAGAIN)
    return; // ← message lost forever, no indication
```

**Why this happened:** The code treated `EAGAIN` as "nothing to do" instead of "queue and wait."
The message was never stored for later transmission, so when the TCP buffer filled up during the FLOODBOT test, 200 pending messages vanished silently while clients were suspended with Ctrl+Z.

**Impact during FLOODBOT:** When a client TCP buffer fills (after Ctrl+Z), the server calls `send()` and gets `EAGAIN`. Without a send queue, that message is lost. The TCP/IP layer does NOT retry automatically—the application must.

---

## 2. The Fix — Per-Client Send Queue + POLLOUT

### The Two-Step POSIX Pattern

POSIX requires two-step error handling for `send()`:

**Step 1:** If `sent > 0` (partial write):
```cpp
if (sent > 0 && static_cast<size_t>(sent) < msg.size()) {
    // Partial write: queue the remainder
    std::string remainder = msg.substr(sent);
    clientSendQueue[clientFd].push(remainder);
    // Arm POLLOUT: tell poll() to watch for socket ready-to-write
    updatePollforWritable(clientFd, fd_array, nfds);
    return; // No errno needed—return value tells the story
}
```

**Step 2:** If `sent == -1` (error):
```cpp
if (sent < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // TCP buffer full: queue the entire message
        clientSendQueue[clientFd].push(msg);
        updatePollforWritable(clientFd, fd_array, nfds);
        return; // errno clarified the cause; now event loop handles it
    }
    else if (errno == EPIPE) {
        // Client disconnected
        disconnectClient(clientFd);
        return;
    }
    else {
        // Unexpected error
        logError("send() failed: " + std::string(strerror(errno)));
        disconnectClient(clientFd);
    }
}
```

### How It Integrates: `sendToClient()` + `flushClientSendQueue()`

**`sendToClient()` in `src/Server.cpp`:**
- Called whenever a message is ready to send (broadcast, reply, etc.)
- Attempts immediate `send()`
- On `EAGAIN`: queues message and arms POLLOUT
- On `EPIPE`: disconnects cleanly
- On success: message sent, queue still monitored for future writes

**`flushClientSendQueue()` in `src/Server.cpp`:**
- Called when `poll()` signals `POLLOUT` (socket ready for writing)
- Drains queue in order (FIFO)
- Each `send()` handled with same error logic as `sendToClient()`
- When queue empty: disarm POLLOUT to avoid busy-loop

**Result:** no message is lost; TCP buffer exhaustion becomes a queuing opportunity, not a silent failure.

---

## 3. The Eval Sheet Clarification

### What the Evaluation Sheet Actually Says

**Exact quote from eval checklist:**
> "errno should not be used **to trigger specific action** (e.g. like reading again after errno == EAGAIN)"

### What This Forbids

**Forbidden pattern — errno-triggered immediate retry:**
```cpp
// WRONG: using errno as control flow to loop without poll()
ssize_t sent = send(clientFd, msg.c_str(), msg.size(), 0);
if (sent < 0 && errno == EAGAIN) {
    sent = send(clientFd, msg.c_str(), msg.size(), 0); // ← retry immediately
    if (sent < 0 && errno == EAGAIN) {
        sent = send(clientFd, msg.c_str(), msg.size(), 0); // ← retry again
        // ... spiral of retries without sleep, poll, or yielding control
    }
}
```

This violates the rule: you're using errno to trigger a "specific action" (loop retry) without consulting `poll()` first.

### What This Allows

**Allowed pattern — errno for classification, NOT control:**
```cpp
ssize_t sent = send(clientFd, msg.c_str(), msg.size(), 0);
if (sent < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // errno classifies the -1: not a real error, buffer full
        // Action: queue message and arm POLLOUT
        clientSendQueue[clientFd].push(msg);
        updatePollforWritable(clientFd, fd_array, nfds);
    }
    else if (errno == EPIPE) {
        // errno classifies the -1: client hung up
        // Action: disconnect
        disconnectClient(clientFd);
    }
    return; // ← exit to event loop; dont retry here
}
```

**Key distinction:** errno is used to *distinguish causes* of `-1` (which action to take: queue/disconnect/error), not to *control loop retry*. Once you categorize the error, you return to the event loop and let `poll()` decide what happens next.

---

## 4. What poll() Guarantees — and What It Doesn't

### poll() Signals Opportunity, Not Certainty

**What poll() guarantees:**
- `POLLIN`: at least 1 byte is ready to read
- `POLLOUT`: you can write without blocking

**What poll() does NOT guarantee:**
- That all data will fit in one syscall
- That the buffer won't fill between poll() return and send() call

### The Race Window

Quoting POSIX `man 7 socket`:
> "Even if select() indicates that a socket is ready, a subsequent recv() on a non-blocking socket could still fail with EAGAIN."

**Concrete scenario (the FLOODBOT case):**
1. `poll()` returns: "socket ready for write, buffer has space"
2. Between poll() return and your `send()` call, network hiccup OR another client writes to shared kernel buffer
3. You call `send(fd, 1000_bytes, 0)` expecting success
4. Kernel returns `-1` with `errno == EAGAIN` (buffer unexpectedly full)
5. Your message is NOT sent that cycle

**This is not a design flaw; it's a fundamental OS guarantee:** poll() tells you *current state*, not *future state*. The window is usually microseconds, but it exists.

### Why Send Queue + POLLOUT is the Solution

The send queue + POLLOUT pattern handles this race window:
1. `send()` fails with `EAGAIN` → queue the message
2. Arm `POLLOUT` on that socket
3. Next poll() cycle, same socket has space again
4. `flushClientSendQueue()` retries the queued message
5. If it succeeds: message moves from queue to TCP buffer
6. If it fails again with `EAGAIN`: it stays queued until next POLLOUT signal

No busy-loop, no repeated `send()` calls without poll() in between, no data loss.

---

## 5. FLOODBOT Proof

### Eval Sheet Mandate

The ft_irc evaluation explicitly requires:
> "Stop a client (-Z) on a channel... server should not hang... all stored commands should be processed normally"

This directly mandates errno-based send queue: when `send()` returns `EAGAIN`, messages must be queued and delivered on resume. Without errno classification, the server cannot distinguish EAGAIN (queue) from EPIPE (disconnect) and cannot meet this requirement.

### The Real-World Test Scenario

The FLOODBOT bonus already documents the full test stack in [docs/bonus/floodbot/](../../bonus/floodbot/CHANNEL_FLOOD_BOT_HOW_TO_USE.md).
Here, we show how send queue + EAGAIN handling proves the fix works:

**Setup (already in FLOODBOT docs):**
- Server running on 127.0.0.1:6667
- Bot connected, joined #test
- Client A connected, joined #test
- Flood script ready to send 200 PRIVMSG in rapid succession

**The Critical Test Steps:**

```
1. Flood channel (200 PRIVMSG from independent floodbot connection)
   └─ Server receives all 200 in rapid succession
   └─ Server broadcasts each to channel members (bot, client A)

2. Client A: Ctrl+Z (suspend) BEFORE messages arrive
   └─ kernel TCP buffer for clientA fills after ~N messages
   └─ Server tries send() to clientA socket
   └─ send() returns -1, errno == EAGAIN
   └─ BEFORE FIX: message silently dropped
   └─ AFTER FIX: message queued, POLLOUT armed

3. All 200 PRIVMSG arrive at clientA's TCP buffer while suspended
   └─ Queue grows; POLLOUT remains armed
   └─ Server's poll() loop continues; no hang

4. Client A: fg (resume)
   └─ poll() signals POLLOUT on clientA socket
   └─ flushClientSendQueue() drains entire queue
   └─ All 200 messages displayed in terminal—ZERO loss

5. Bot: Ctrl+Z, then fg (same test for bot)
   └─ Bot receives all 200 messages it sent (echoed back by server)
   └─ Logging confirms zero loss
```

**Why this proves errno is necessary:**
- The server MUST distinguish `send() == -1` cases:
  - `EAGAIN` → queue and wait for POLLOUT
  - `EPIPE` → client disconnected, clean up
  - Other → unexpected, log and disconnect
- Without errno classification, code cannot tell the difference
- Without the difference, you cannot implement send queue correctly

**See also:** [send_eagain_fixed.md](../eagain_lost_in_send_to_client/send_eagain_fixed.md) for the detailed fix writeup.

---

## 6. Summary Table

| Syscall | Return | Meaning | errno Needed? | Action |
|---|---|---|---|---|
| `accept()` | -1 | error | YES | EAGAIN→retry later; other→log&handle |
| `recv()` | -1 | error | YES | EAGAIN→no data yet; ECONNRESET→disconnect |
| `send()` | -1 | error | YES | EAGAIN→queue&wait; EPIPE→disconnect |
| `send()` | 0..N < len | partial | NO | Queue remainder; return (return value says enough) |
| `poll()` | N | ready count | NO | Tells which FDs ready; doesn't guarantee amount |

**When errno is needed:** when return value alone is ambiguous (e.g., `-1` could mean several different things).  
**When errno is forbidden as "trigger":** in loops that retry without re-entering `poll()`.

---

## 7. References & Cross-Links

### Project Documentation

- [docs/bonus/floodbot/CHANNEL_FLOOD_BOT_HOW_TO_USE.md](../../bonus/floodbot/CHANNEL_FLOOD_BOT_HOW_TO_USE.md) — Complete FLOODBOT architecture, test scenario, integration points. **Start here for the manual test.**

- [docs/network/eagain_lost_in_send_to_client/send_eagain_fixed.md](../eagain_lost_in_send_to_client/send_eagain_fixed.md) — Detailed before/after fix writeup with line-by-line code comparison.


### External References

- **ft_irc Subject PDF** — No explicit ban on errno; forbids only fork-based concurrency, blocking I/O misuse, and fcntl abuse. Evaluation focus: "verify every possible error and issue, such as receiving partial data, low bandwidth, etc."

- **POSIX Manual Pages:**
  - `man 2 send` — Return value semantics, EAGAIN/EWOULDBLOCK equivalence
  - `man 2 poll` — POLLOUT guarantee: socket writable, not necessarily all data fits
  - `man 7 socket` — Race window between select/poll and syscall
  - `man 7 errno` — Error classification system

- **LinkedIn Discussion** — [EAGAIN issue](https://www.linkedin.com/posts/oleost_ecole42-42warsaw-cpp-ugcPost-7447466033948377088-2FPV) — Peer debate on eval sheet interpretation; discussion proved errno for error classification is not only allowed but *required*.
- **LinkedIn Manual video** — [EAGAIN issue FLOODBOT](https://www.linkedin.com/feed/update/urn:li:activity:7452598819722174464/)

---

## Conclusion

The eval sheet forbids using errno as a *loop control mechanism* (immediate retries without poll). It does NOT forbid using errno to *classify* what `-1` means, then exiting to the event loop.

The FLOODBOT test proves this distinction matters: suspend a client, flood the channel, and without errno-based send queue handling, 200 messages vanish. With it, all 200 arrive on unsuspend.

—

**Document Version:** 1.0  
**Last Updated:** April 22, 2026  

