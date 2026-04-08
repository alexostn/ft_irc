# PlayBot Architecture Overview

## Quick Start

```bash
# Build
make playbot

# Run server (Terminal 1)
./ircserv 6667 testpass

# Run bot (Terminal 2)
./playbot 127.0.0.1 6667 testpass playbot #general

# Bot appears in channel with status updates every 60s
# Responds to: !ping, !echo <text>, !time, !uptime, !help
```

---

## Three-Layer Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      BotMain (Event Loop)                   │
│  • Polling every 60s for status alert via ALERT_INTERVAL    │
│  • Handles PRIVMSG dispatch + reconnect logic               │
└──────────────────────┬──────────────────────────────────────┘
                       │
        ┌──────────────┴──────────────┐
        │                             │
┌───────▼─────────────┐     ┌─────────▼──────────────┐
│   BotCore           │     │   BotCommands          │
│ ─────────────────────    │ ─────────────────────────
│ • connect()         │     │ • dispatch(line)       │
│ • registerIRC()     │     │ • handle(cmd)          │
│ • sendRaw(msg)      │     │ • sendAlert()          │
│ • tick(ms)          │     │                        │
│ • reconnect(n)      │     │ Commands:              │
│                     │     │ !ping !echo !time      │
│ Transport Layer:    │     │ !uptime !help          │
│ poll() + recv/send  │     └────────────────────────┘
│ PING/PONG silent    │
└─────────────────────┘
```

---

## Core Concepts

### 1. Transport (BotCore)

**Responsibility:** TCP socket, polling, message buffering, auto-reconnect.

**Key Design:**
- Non-blocking I/O: `fcntl(fd, F_SETFL, O_NONBLOCK)`
- Single `poll()` call per tick with timeout = remaining time until next alert
- PING/PONG handled internally — never exposed to application
- Reconnect: 3-second sleep between attempts, up to N retries

**Public API:**
```cpp
bool connect();                           // gethostbyname + socket + connect
bool registerIRC();                       // PASS NICK USER JOIN sequence
std::vector<std::string> tick(int ms);   // poll(ms) → extract lines
void sendRaw(const std::string& msg);    // append \r\n, send
bool reconnect(int max_attempts);        // retry N times with backoff
bool isConnected() const;                // fd >= 0
```

### 2. Commands (BotCommands)

**Responsibility:** Parse PRIVMSG, route to handlers, send proactive alerts.

**Key Design:**
- `parsePrivmsg()`: Extract sender, target, text from `:nick!user@host PRIVMSG #chan :message`
- Routes to individual `cmd*()` handlers based on text prefix
- `sendAlert()`: Proactive message sent when `poll()` timeout expires

**Handlers:**
```
!ping         → "pong!"
!echo <text>  → echoes the text
!time         → current system time
!uptime       → uptime in seconds since bot start
!help         → command list + teaser: "coming soon: !play"
```
[detailed commands explained in](PLAYBOT_COMMANDS.md)

### 3. Event Loop (BotMain)

**Responsibility:** Long-running stability, timing coordination, reconnect handling.

```cpp
while (true) {
    int remaining_ms = (ALERT_INTERVAL - elapsed_seconds) * 1000;
    lines = core.tick(remaining_ms);       // poll()-driven timing
    
    for (each line) cmds.dispatch(line);   // process messages
    
    // Alert fires when poll timeout elapsed
    if (elapsed >= ALERT_INTERVAL) {
        cmds.sendAlert();
        last_alert = now;
    }
    
    // Reconnect on drop
    if (!core.isConnected()) {
        core.reconnect(5) or exit(1);
    }
}
```

---

## Design Principles

### 1. **Poll-Driven Alert Timing (No Sleep)**
Problem: `sleep()` in main loop blocks PRIVMSG response.
Solution: Use `poll(timeout)` to drive alert interval.

**Result:**
- Alert fires exact 60s apart
- Response to PRIVMSG: sub-millisecond (zero blocking)
- Zero accumulated micro-sleeps

### 2. **Transparent PING/PONG**
Problem: Application shouldn't care about IRC keep-alive.
Solution: `BotCore::tick()` intercepts PING internally.

**Result:**
- Application only sees data-bearing PRIVMSG
- BotCommands never sees PING/PONG
- Connection stays alive indefinitely

### 3. **C++98 Purity**
- No `nullptr`, `auto`, range-for, lambdas
- `std::ostringstream` for numeric→string conversions
- Manual memory: all stack-allocated (no new/delete)
- Single `fcntl()` form: `fcntl(fd, F_SETFL, O_NONBLOCK)`

---

## File Structure

```
include/irc/bot/
  ├─ BotCore.hpp        (interface: connect, tick, reconnect)
  └─ BotCommands.hpp    (interface: dispatch, sendAlert)

src/bot/
  ├─ BotCore.cpp        (~190 lines: socket/poll/reconnect)
  ├─ BotCommands.cpp    (~120 lines: PRIVMSG parse + handlers)
  └─ BotMain.cpp        (~70 lines: event loop + timing)

docs/bonus/playbot/
  ├─ PLAYBOT_HOW_TO_USE.md (detailed usage + architecture)
  ├─ PLAYBOT_ARCHITECTURE.md (this document)
  └─ PLAYBOT_ARCHITECTURE_RU.md (Russian version)
```

---

## Compilation & Testing

```bash
# Compile (linked with -Wall -Wextra -Werror -std=c++98)
make playbot    # standalone binary
make            # includes main server + playbot

# Verify no leaks
valgrind --leak-check=full ./playbot 127.0.0.1 6667 testpass playbot #general

# Unit tests (future: part 2)
# - Command parsing with malformed PRIVMSG
# - Reconnect scenarios (simulated connection drop)
# - Alert timing precision under load
```

---

## Part 2 Preview

**Planned:** Integrate `!play` command into existing framework.

- `BotCommands::cmdPlay()` — Route to game state machine
- Game state stored in BotCore/BotMain context
- PRIVMSG dispatch unchanged — just add new case to `handle()`

---

## Metrics

- **Code Size:** ~380 lines (BotCore + BotCommands + BotMain)
- **Compilation:** ~0.2s on modern CPU
- **Memory:** ~2KB for buffers + sockets
- **Alert Precision:** ±100ms (within poll() resolution)
- **Reconnect Latency:** 3s + DNS lookup (~50-500ms)

---

## Quick Brain Map ┌|∵|┘

**Three Layers = Three Files:**

| Layer | File | Job | Breaks If... |
|-------|------|-----|---|
| **Transport** | `BotCore.cpp` | Socket + poll + reconnect | Can't connect |
| **Logic** | `BotCommands.cpp` | Parse MessageCommandRoute | Doesn't understand messages |
| **Timing** | `BotMain.cpp` | Loop + alert every 60s | Bot freezes |

**Why This Shape?**
- **Separation of concerns:** Network ≠ Logic ≠ Timing
- **Easy debugging:** Connection issue? Check BotCore. Message issue? Check BotCommands.
- **Reusable:** BotCore can be used by other bots, BotCommands handlers can be extended.

(￢_￢;) Architecture Checklist:

- [OK] (･ω･)ノ Each file does ONE thing
- [OK] (･ω･)ノ BotCore handles network chaos (reconnects, PING/PONG)
- [OK] (･ω･)ノ BotCommands never touches sockets
- [OK] (･ω･)ノ BotMain is just an orchestrator
- [OK] (･ω･)ノ No global state, no spaghetti

---

## Modularity: Reusable Library Pattern ┌|∵|┘

**BotCore is NOT PlayBot-specific.** It's a library for any IRC bot.

| Component | Reusable? | Locked To PlayBot? |
|-----------|-----------|---|
| **BotCore.cpp** | [+] **YES** — pure IRC transport | [-] NO — generic socket/poll/register |
| **BotCommands.cpp** | [-] **NO** — has `!ping`, `!echo`, `!time` | [+] YES — PlayBot-only handlers |
| **BotMain.cpp** | [-] **NO** — has 60s alert loop | [+] YES — PlayBot timing logic |

**Proof: Any bot could use BotCore**

If you built `song_bot`:
```cpp
// song_bot.cpp — hypothetical new bot
#include "irc/bot/BotCore.hpp"

int main() {
    BotCore core("127.0.0.1", 6667, "pass", "song_bot", "#song");
    core.connect();
    core.registerIRC();
    
    while (true) {
        std::vector<std::string> lines = core.tick(1000);
        for (auto& line : lines) {
            if (line.find("!play") != std::string::npos) {
                core.sendRaw("PRIVMSG #song :Now singing...");
            }
        }
    }
}
```

[+] **BotCore used. BotCommands NOT used. Clean reuse.**

**Design: Cake Pattern (from bottom)**
```
Layer 3: BotMain         [PlayBot loop logic]
   ▲
   ├─ uses →
   │
Layer 2: BotCommands     [PlayBot dispatch logic]
   ▲
   ├─ uses →
   │
Layer 1: BotCore         [Generic IRC library]
```

Swap Layers 2+3 → get different bot. Layer 1 lives forever. └[∵┌] 

