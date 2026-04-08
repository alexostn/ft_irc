# MessageBuffer Integration Test

## Overview
Manual integration tests for validating MessageBuffer functionality with real IRC server and `nc` client connections. Tests partial message buffering, complete message extraction, and multi-command handling.

---

## Test Environment

**Terminal 1 - Server:**
```bash
./ircserv 6667 password
```

**Terminal 2 - Client:**
Execute test commands

**Terminal 3 - Monitor (optional):**
```bash
lsof -i :6667
```

---

## Test Cases

### 1. Simple Complete Message
**Purpose:** Verify single complete IRC command processing.

```bash
printf 'NICK test1\r\n' | nc localhost 6667
# nc -С localhost 6667 # = alternatively = the same
```

**Expected Behavior:**
- Server receives: `NICK test1\r\n` (13 bytes)
- MessageBuffer extracts: `"NICK test1"`
- Connection remains open (awaiting USER command per IRC protocol)

**Terminate:** `Ctrl+C` after 1 second

---

### 2. Partial Message Buffering ⚠️ CRITICAL
**Purpose:** Validate buffering of incomplete messages across multiple `recv()` calls.

```bash
(printf "NICK te"; sleep 1; printf "st\r\n") | nc localhost 6667
```

**Expected Behavior:**
- **First chunk:** `"NICK te"` → buffered (no `\r\n`)
- **After 1 second:** `"st\r\n"` → buffer completes
- MessageBuffer extracts: `"NICK test"`

**Server Debug Output:**
```
[MessageBuffer] Buffer HEX: 4e 49 43 4b 20 74 65 73 74 0d 0a
```

**Terminate:** `Ctrl+C` after 3 seconds total

---

### 3. Multiple Commands in Single Stream
**Purpose:** Test extraction of multiple `\r\n`-delimited messages.

```bash
printf 'NICK alice\r\nUSER alice 0 * :Alice\r\n' | nc localhost 6667
```

**Expected Behavior:**
- Single `recv()` receives both commands
- MessageBuffer extracts 2 messages:
  1. `"NICK alice"`
  2. `"USER alice 0 * :Alice"`

**Terminate:** `Ctrl+C` after 1 second

---

## Connection State Monitoring

Check active connections during tests:

```bash
lsof -i :6667
```

**Example Output:**
```
COMMAND    PID     USER   FD   TYPE   NODE NAME
ircserv  12345 oostapen    3u  IPv4   TCP *:ircd (LISTEN)
ircserv  12345 oostapen    4u  IPv4   TCP localhost:ircd->localhost:35650 (ESTABLISHED)
nc       12346 oostapen    3u  IPv4   TCP localhost:35650->localhost:ircd (ESTABLISHED)
```

---

## Why Does `nc` Hang?

**Reason:** IRC protocol requires complete registration (`NICK` + `USER`) before server responds.

**Flow:**
1. Client sends: `NICK test1\r\n`
2. Server buffers command, **waits for `USER`**
3. `nc` waits for EOF or server response
4. **Deadlock** until manual termination

**This is expected behavior** — use `Ctrl+C` to exit.

---

## Valgrind Memory Check

Run the same tests with memory leak detection:

```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         ./ircserv 6667 password
```

Then repeat all test cases (1-3) from Terminal 2.

**Expected:** No memory leaks in MessageBuffer allocation/deallocation.

---

## Success Criteria

- ✅ Test 1: Single complete message extracted
- ✅ Test 2: Partial messages buffered and reassembled correctly
- ✅ Test 3: Multiple messages extracted from single buffer
- ✅ Valgrind: No memory leaks, no invalid reads/writes
- ✅ Server handles `Ctrl+C` client disconnections gracefully

---

## Notes

- All tests use IRC protocol `\r\n` (CRLF) line terminators
- `nc` is a minimal TCP client — does not implement IRC registration flow
- Server behavior (waiting for complete registration) is **correct per RFC 1459**
- Manual `Ctrl+C` required to close incomplete registration sessions

![MessageBuffer integration hand test](../assets/MessageBuffer_INTEGRATION_HAND_TEST.png)