# IRC Bonus Bot Architecture & Message Flood Mechanism

**Version:** 1.0  
**Date:** March 24, 2026  
**File Location:** `src/bot/channel_flood_bot.cpp`  
**Related Scripts:** `tests/flood_channel.sh`, `bonus/flood_channel.sh`, `bonus/test_bot.sh`

---

## Overview

The ft_irc project includes a bonus IRC bot implementation (`channel_flood_bot`) and test scripts that collectively demonstrate:
1. **Bot resilience** — handles incoming messages while suspended
2. **Server stability** — processes 200 rapid messages without loss or crash
3. **Concurrent client handling** — multiple clients + bot working together

---

## ┌|∵|┘Part 1: What the Bot Does

### Bot Identity & Purpose

```cpp
const char* nick    = (argc > 4) ? argv[4] : "flood_bot";      // default
const char* channel = (argc > 5) ? argv[5] : "#test";          // default
const char* pass    = (argc > 3) ? argv[3] : "testpass";       // default
```

**File:** `src/bot/channel_flood_bot.cpp`  
**Type:** Standalone IRC **client** (not server)  
**Role:** Demonstration bot that listens for commands and responds

### Bot Initialization (Lines 123-156)

```cpp
// 1. Connection Phase
struct hostent* he = gethostbyname(host);           // Resolve hostname
int fd = socket(AF_INET, SOCK_STREAM, 0);          // Create TCP socket
connect(fd, (struct sockaddr*)&addr, sizeof(addr)); // Connect to server

// 2. Registration Phase (IRC Handshake)
send_line(fd, "PASS testpass");                    // Step 1: Authenticate
send_line(fd, "NICK flood_bot");                   // Step 2: Set nickname
send_line(fd, "USER flood_bot 0 * :IRC Bot");      // Step 3: Set user info
sleep(1);                                           // Wait for welcome (001-004 numerics)

// 3. Join Phase
send_line(fd, "JOIN #test");                       // Join target channel
std::cout << "[*] joined #test — listening\n";     // Confirm ready
```

**Output when bot starts:**
```
[*] connected to 127.0.0.1:6667
[BOT>>] PASS testpass
[BOT>>] NICK flood_bot
[BOT>>] USER flood_bot 0 * :IRC Bot
[<<SVR] :ft_irc 001 flood_bot ...        (welcome message)
[<<SVR] :ft_irc 002 flood_bot ...
...more welcome numerics...
[BOT>>] JOIN #test
[<<SVR] :ft_irc 353 flood_bot ...        (NAMES list)
[<<SVR] :ft_irc 366 flood_bot ...        (end NAMES)
[*] joined #test — listening
```

### Bot Command Handlers (Lines 83-118)

The bot implements an **event loop** that:
1. Waits for server messages using `select()` (non-blocking I/O)
2. Parses received PRIVMSG lines
3. Executes 5 built-in commands

#### Command Processing Flow:

```
Incoming IRC line from server
    ↓
select() detects data ready
    ↓
recv() reads from socket into buffer
    ↓
extract_lines() splits by \r\n
    ↓
For each line:
    - If "PING ..." → send "PONG ..."
    - Else if "PRIVMSG" → parse_privmsg()
        ↓
        Check if message text is a command
        ↓
        Handle: !ping, !echo, !time, !uptime, !help
```

#### The 5 Commands:

| Command | Handler | Response | Example |
|---|---|---|---|
| `!ping` | Line 93 | `PRIVMSG #test :pong!` | Direct response |
| `!echo <msg>` | Line 100 | `PRIVMSG #test :<msg>` | Echo back user message |
| `!time` | Line 108 | `PRIVMSG #test :Mon Mar 24 22:40:00 2026` | Current time |
| `!uptime` | Line 116 | `PRIVMSG #test :uptime: 0h 5m 23s` | Since bot join |
| `!help` | Line 97 | `PRIVMSG #test :commands: !ping...` | Show all commands |

### Bot Event Loop (Lines 158-192)

```cpp
while (true)
{
    // Monitor socket for incoming data
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);                              // Watch bot's socket
    
    select(fd + 1, &rfds, NULL, NULL, NULL) < 0;   // Block until data or signal
    
    ssize_t n = recv(fd, tmp, sizeof(tmp) - 1, 0); // Read up to 4096 bytes
    
    if (n <= 0) break;                              // Server closed
    
    // Extract complete lines (ending with \r\n)
    std::vector<std::string> lines = extract_lines(recv_buf, tmp, n);
    
    // Process each complete line
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::cout << "[<<SVR] " << line << std::endl;  // Log received
        
        // Handle PING to prevent timeout (~120s)
        if (line.substr(0, 4) == "PING") {
            send_line(fd, "PONG" + line.substr(4));
            continue;
        }
        
        // Handle PRIVMSG commands
        if (parse_privmsg(line, sender, target, text))
            handle_command(fd, sender, target, text, start_time);
    }
}
```

**Key Feature:** Bot can be suspended (Ctrl+Z) and resumed (fg) without losing messages because:
- The server buffers messages for the bot
- When bot resumes, select() wakes up
- Bot receives all buffered messages at once

---

##  Part 2: What the Scripts Do

### Script 1: `tests/flood_channel.sh` (Canonical)

**Location:** `tests/flood_channel.sh`  
**Purpose:** Generate 200 IRC PRIVMSG commands in a single connection  
**Lines:** 1-24

#### Script Flow:

```bash
#!/usr/bin/env bash
HOST="127.0.0.1"
PORT=6667
PASS="testpass"
NICK="floodbot"
CHAN="#test"
COUNT=200

{
  # Step 1: Register with server
  echo "PASS testpass"
  echo "NICK floodbot"
  echo "USER floodbot 0 * :Flood Bot"
  sleep 1          # Wait for server welcome
  
  # Step 2: Join channel
  echo "JOIN #test"
  sleep 0.3        # Wait for JOIN confirmation
  
  # Step 3: Send 200 messages
  for i in $(seq 1 200); do
    echo "PRIVMSG #test :msg 1"
    echo "PRIVMSG #test :msg 2"
    echo "PRIVMSG #test :msg 3"
    ...
    echo "PRIVMSG #test :msg 200"
  done
  sleep 1          # Let last messages drain
  
} | nc -C 127.0.0.1 6667
```

#### Detailed Message Generation:

**Bash for-loop (Line 20-22):**
```bash
for i in $(seq 1 $COUNT); do
    echo "PRIVMSG $CHAN :msg $i"
done
```

This expands to:
```
PRIVMSG #test :msg 1
PRIVMSG #test :msg 2
PRIVMSG #test :msg 3
...
PRIVMSG #test :msg 198
PRIVMSG #test :msg 199
PRIVMSG #test :msg 200
```

**Total:** 200 individual PRIVMSG commands, each on a new line.

#### Shell Pipeline:
```bash
{ ... } | nc -C 127.0.0.1 6667
          ↓
          Sends all output as TCP stream to server
```

The curly braces `{ }` create a **subshell** that outputs all commands, which are piped to `nc` (netcat).

`nc -C` connects to localhost:6667 and sends every line as part of the IRC protocol stream.

### Script 2: `bonus/flood_channel.sh` (Wrapper)

**Location:** `bonus/flood_channel.sh`  
**Lines:** 1-16

```bash
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]:-$0}")" && pwd)"
CANONICAL_SCRIPT="$SCRIPT_DIR/../tests/flood_channel.sh"

if [ ! -f "$CANONICAL_SCRIPT" ]; then
  echo "Canonical flood script not found: $CANONICAL_SCRIPT" >&2
  exit 1
fi

if [ ! -x "$CANONICAL_SCRIPT" ]; then
  exec bash "$CANONICAL_SCRIPT" "$@"
else
  exec "$CANONICAL_SCRIPT" "$@"
fi
```

**Purpose:** DRY principle — avoids code duplication  
**Function:** Delegates to `tests/flood_channel.sh`

**Why wrapper?**
- Both `bonus/` and `tests/` need the flood test
- Keep single source of truth in `tests/`
- Wrapper ensures consistency

### Script 3: `bonus/test_bot.sh`

**Location:** `bonus/test_bot.sh`  
**Purpose:** Verify bot command responses  
**Lines:** 1-20

#### Script Flow:

```bash
HOST="127.0.0.1"
PORT=6667
PASS="testpass"
NICK="bottester"
CHAN="#test"

{
  # Registration
  echo "PASS $PASS"
  echo "NICK $NICK"
  echo "USER $NICK 0 * :Bot Tester"
  sleep 1
  
  # Join channel
  echo "JOIN $CHAN"
  sleep 0.5
  
  # Send 4 test commands in sequence
  echo "PRIVMSG $CHAN :!ping"
  sleep 0.5
  echo "PRIVMSG $CHAN :!uptime"
  sleep 0.5
  echo "PRIVMSG $CHAN :!echo HELLO EVALUATOR:)"
  sleep 2               # Wait for bot response
  
} | nc -C "$HOST" "$PORT"
```

#### Expected Output (in bot terminal):

```
[<<SVR] :bottester!bottester@::ffff:127.0.0.1 PRIVMSG #test :!ping
[BOT>>] PRIVMSG #test :pong!

[<<SVR] :bottester!bottester@::ffff:127.0.0.1 PRIVMSG #test :!uptime
[BOT>>] PRIVMSG #test :uptime: 0h Xm Xs

[<<SVR] :bottester!bottester@::ffff:127.0.0.1 PRIVMSG #test :!echo HELLO EVALUATOR:)
[BOT>>] PRIVMSG #test :HELLO EVALUATOR:)
```

---

## Part 3: How 200 Messages Are Generated & Processed

### The Message Flood Mechanism

#### Step 1: Message Generation Location

**Where:** Inside `tests/flood_channel.sh` line 20-22

```bash
for i in $(seq 1 200); do        # Loop 200 times
    echo "PRIVMSG #test :msg $i"  # Output one PRIVMSG per iteration
done
```

**Result:** 200 separate PRIVMSG commands printed to stdout.

**Format of each message:**
```
PRIVMSG #test :msg 1
PRIVMSG #test :msg 2
...
PRIVMSG #test :msg 200
```

#### Step 2: Message Transmission

**Connection Path:**
```
flood_channel.sh stdout
    ↓ (pipe)
nc -C socket stream
    ↓ (TCP)
127.0.0.1:6667 (server socket)
```

**What happens:**
1. Shell outputs all 200 lines to stdout
2. `nc -C` pipes them to TCP socket
3. Messages arrive at server TCP port 6667
4. Server receives all 200 in rapid succession

#### Step 3: Server Message Processing

The IRC server (your `ircserv`) receives the stream:

```
Client Connection (floodbot)
    ↓
TCP receive buffer
    ↓
Server's recv() in Poller
    ↓
MessageBuffer per-client
    ↓
Parse each complete \r\n delimited line
    ↓
Process PRIVMSG command
    ↓
Add to channel broadcast queue
    ↓
Send to all channel members via their send queues
```

#### Step 4: Message Broadcasting

When all 200 PRIVMSG from floodbot arrive:

```
For each PRIVMSG #test :msg N:
    → Server identifies command: PRIVMSG
    → Server gets target: #test
    → Server adds to channel message buffer
    → Server iterates all clients in #test
      → Adds message to each client's send queue
      → Checks if client socket is ready (via poll POLLOUT)
      → Drains send queue one recv cycle later
```

**Clients receive:**
```
:floodbot!floodbot@::ffff:127.0.0.1 PRIVMSG #test :msg 1
:floodbot!floodbot@::ffff:127.0.0.1 PRIVMSG #test :msg 2
...
:floodbot!floodbot@::ffff:127.0.0.1 PRIVMSG #test :msg 200
```

---

## Complete Test Scenario

### Execution Sequence

**Terminal 1 — Server:**
```bash
valgrind --leak-check=full ./ircserv 6667 testpass
```

**Terminal 2 — Bot (running):**
```bash
./channel_flood_bot
[*] connected to 127.0.0.1:6667
[BOT>>] PASS testpass
[BOT>>] NICK flood_bot
[BOT>>] USER flood_bot 0 * :IRC Bot
[<<SVR] :ft_irc 001 flood_bot ...
...
[*] joined #test — listening
← Ctrl+Z to suspend bot here
```

**Terminal 3 — Client (running):**
```bash
nc -C 127.0.0.1 6667
PASS testpass
NICK clientA
USER clientA 0 * :Client A
JOIN #test
← Ctrl+Z to suspend client here
```

**Terminal 4 — Flood Script (runs):**
```bash
bash bonus/flood_channel.sh
```

**What happens during flood:**
```
Time    Event
────    ─────────────────────────────────────────────────
T0      floodbot sends 200 PRIVMSG to #test
T1      Server receives all messages (buffered)
T2      Server broadcasts to clientA
T3      clientA receives all 200 messages (buffered in TCP)
T4      flood script ends, logs show success
T5      Terminal 3: fg ← Resume clientA
T6      clientA terminal displays all 200 messages
T7      Terminal 2: fg ← Resume bot
T8      Bot resumes, displays [<<SVR] logs of its own messages
T9      Terminal 5: bash bonus/test_bot.sh
T10     Bot receives test commands, responds with 5 outputs
```

---

##  ∬ Integration Points

### How Parts Work Together

```
┌─────────────────────────────────────────┐
│  tests/flood_channel.sh (canonical)     │
│  - Generates 200 PRIVMSG in bash for    │
│  - Pipes to nc for TCP delivery         │
└──────────────────────┬──────────────────┘
                       │
┌──────────────────────▼──────────────────┐
│  bonus/flood_channel.sh (wrapper)       │
│  - Delegates to canonical above         │
└──────────────────────┬──────────────────┘
                       │
┌──────────────────────▼──────────────────┐
│  Your IRC Server (ircserv)              │
│  - Receives 200 PRIVMSG                 │
│  - Broadcasts to all #test members      │
│  - Handles buffer overflows gracefully  │
└──────────────────────┬──────────────────┘
                       │
          ┌────────────┼────────────┐
          ↓            ↓            ↓
    ┌─────────┐ ┌─────────┐ ┌─────────┐
    │ Bot     │ │ Client  │ │ Test    │
    │ receives│ │ receives│ │ Verifies│
    │ & logs  │ │ & shows │ │ Commands│
    │ messages│ │ messages│ │ Work    │
    └─────────┘ └─────────┘ └─────────┘
```

---

## Message Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    FLOOD TEST MESSAGE FLOW                      │
└─────────────────────────────────────────────────────────────────┘

GENERATION (in shell):
    for i in 1..200
        echo "PRIVMSG #test :msg $i"
    done
              ↓
         [stdout]
              ↓
    ┌─────────────────────┐
    │ 200 PRIVMSG lines   │
    │ msg 1..200          │
    └─────────────────────┘

TRANSMISSION (via nc):
              ↓
    ┌─────────────────────┐
    │ TCP Socket Stream   │
    │ (pipe to nc)        │
    └─────────────────────┘
              ↓
    ┌─────────────────────┐
    │ TCP/IP Protocol     │
    │ 127.0.0.1:6667      │
    └─────────────────────┘

RECEPTION (by server):
              ↓
    ┌─────────────────────┐
    │ Server Recv Buffer  │
    │ (all 200 at once)   │
    └─────────────────────┘
              ↓
    Parse & Process Each Message
              ↓
    ┌─────────────────────┐
    │ Broadcast to #test  │
    │ (floodbot) → (all)  │
    └─────────────────────┘
              ↓
    ┌─────────────────────┐
    │ Client Send Queues  │
    │ (buffered output)   │
    └─────────────────────┘
              ↓
    Poll → Send Queues Drain
              ↓
    CLIENT RECEIVES ALL 200 ✓
```

---

##  Why This Tests Server Robustness

1. **Volume Test:** 200 messages stress-tests:
   - Message buffer capacity
   - Parser performance (your IRC parser in src/Parser.cpp handles rapid sequential PRIVMSG parsing)
   - Broadcast efficiency

2. **Concurrency Test:** Bot frozen while flood runs:
   - Server handles suspended clients
   - Messages queue properly
   - No data loss on resume

3. **Integration Test:** Multiple clients + bot:
   - Floodbot sends (200 messages)
   - Regular client receives (all 200)
   - Bot commands still work (!ping, !echo, etc.)
   - Server doesn't crash or hang

---

##  Summary

| Component | What It Does | Location |
|---|---|---|
| **Bot** | Listens for commands, responds to: !ping, !echo, !time, !uptime, !help | `src/bot/channel_flood_bot.cpp` |
| **flood_channel.sh** | Generates 200 PRIVMSG via bash for-loop + nc | `tests/flood_channel.sh` |
| **bonus/flood_channel.sh** | Wrapper delegating to canonical | `bonus/flood_channel.sh` |
| **test_bot.sh** | Verifies bot commands work correctly | `bonus/test_bot.sh` |

**The 200 messages:**
- **Generated:** Inside bash for-loop (1..200)
- **Location:** Variable assignment in shell stdout
- **Transmitted:** Via `nc` TCP socket pipe
- **Processed:** By server's recv() → broadcast → all clients in #test

---

**Document Version:** 1.0  
**Last Updated:** March 24, 2026
