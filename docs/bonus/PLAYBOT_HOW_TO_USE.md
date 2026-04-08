# PLAYBOT — IRC Bot with Command Dispatch & Auto-Reconnect

## Overview

`PlayBot` is a stateful IRC bot built from **BotCore** (connection layer) + **BotCommands** (command dispatch) infrastructure. It connects to an IRC server, joins a channel, responds to commands, and automatically reconnects if the connection drops.

**Status:** Alert message every 60 seconds via `poll()` timeout, no `sleep()` in event loop. Fully compatible with C++98 and POSIX standards.

---

## Architecture

### Three Core Components

#### 1. **BotCore** — Connection & Poll Transport
- **File:** `src/bot/BotCore.cpp` / `include/irc/bot/BotCore.hpp`
- **Responsibility:** TCP socket, IRC registration, event multiplexing, automatic PING/PONG reply
- **Key Methods:**
  - `connect()` — DNS lookup (`gethostbyname`), socket creation, non-blocking mode
  - `registerIRC()` — IRC handshake: `PASS` → `NICK` → `USER` → `JOIN`
  - `sendRaw(msg)` — Append `\r\n`, send to server
  - `tick(timeout_ms)` — Single `poll()` call, extract complete `\r\n`-delimited lines
  - `reconnect(max_attempts)` — Exponential backoff (3s sleep between attempts)
  - `handlePing()` — PING/PONG protocol (transparent, no application visibility)

#### 2. **BotCommands** — Command Dispatch Engine
- **File:** `src/bot/BotCommands.cpp` / `include/irc/bot/BotCommands.hpp`
- **Responsibility:** Parse `PRIVMSG`, route to command handlers, proactive alerts
- **Implemented Commands:**
  - `!ping` → responds with `pong!`
  - `!echo <text>` → echoes the text back
  - `!time` → sends current system time
  - `!uptime` → elapsed seconds since bot start
  - `!help` → lists commands + **teaser**: "coming soon: !play"
- **Key Methods:**
  - `dispatch(line)` — Extract `PRIVMSG`, call `handle()`
  - `handle(sender, target, text)` — Route to command handler
  - `sendAlert()` — Proactive status message to channel

#### 3. **BotMain** — Event Loop Orchestrator
- **File:** `src/bot/BotMain.cpp`
- **Responsibility:** Maintain long-term connection, drive alert timing via `poll()` timeout
- **Loop Logic:**
  ```cpp
  while (true) {
      timeout = (ALERT_INTERVAL - elapsed) * 1000 milliseconds
      lines = core.tick(timeout)          // poll() driven by ALERT_INTERVAL
      for each line: cmds.dispatch(line)
      if (poll timeout expired): cmds.sendAlert()
      if (!core.isConnected()): core.reconnect(5)
  }
  ```

---

## Usage

### Build

```bash
# Compile both ircserv and playbot
make

# Or compile only playbot
make playbot

# Clean
make clean
```

### Run

**Terminal 1 — IRC Server:**
```bash
./ircserv 6667 testpass
```

**Terminal 2 — PlayBot:**
```bash
./playbot 127.0.0.1 6667 testpass playbot #general
```

Bot will:
1. Connect and authenticate
2. Join `#general`
3. Listen for commands
4. Send status alert every 60 seconds: `[PLAYBOT] Status OK`

### Interact (Terminal 3)

```bash
nc 127.0.0.1 6667
```

Then manually (or via script):
```
PASS testpass
NICK testuser
USER testuser 0 * :testuser
JOIN #general
PRIVMSG #general :!ping
PRIVMSG #general :!help
PRIVMSG #general :!uptime
PRIVMSG playbot :!echo Hello from PM
```

Expected bot responses:
```
PRIVMSG #general :pong!
PRIVMSG #general :commands: !ping !echo <text> !time !uptime !help | coming soon: !play
PRIVMSG #general :uptime: 15s
PRIVMSG testuser :Hello from PM
```

---

## Design Patterns

### Non-Blocking Socket + Poll Timeout Timing

The bot drives its alert interval through `poll()`:

```cpp
int remaining_ms = (ALERT_INTERVAL - elapsed_seconds) * 1000;
lines = core.tick(remaining_ms);
if (poll_timeout_expired) {
    cmds.sendAlert();
}
```

**Benefit:** No `sleep()` in main loop, responsive to server messages, alerts fire exactly on schedule.

### Transparent PING/PONG

`BotCore::tick()` automatically replies to PING without exposing it to `BotCommands`. Application only sees data-bearing messages.

### Graceful Reconnection

If server closes connection:
1. `core.isConnected()` returns false
2. `core.reconnect(5)` attempts up to 5 reconnections
3. 3-second sleep between attempts
4. Alert timer resets after successful reconnect

---

## C++98 Compliance

- No `nullptr`, `auto`, range-for, lambdas
- `std::ostringstream` for numeric formatting
- `fcntl(fd, F_SETFL, O_NONBLOCK)` — only permitted form
- `std::string`, `std::vector` for collections
- Manual memory management: `new`/`delete` not used (all stack-allocated)

---

## Testing

### Automated Validation

```bash
# Build with strict flags
make clean && make playbot

# Verify no memory leaks
valgrind --leak-check=full ./playbot 127.0.0.1 6667 testpass playbot #general
```

### Manual Test Scenario

```bash
# Terminal 1
./ircserv 6667 testpass

# Terminal 2
./playbot 127.0.0.1 6667 testpass playbot #general

# Terminal 3 — watch status alerts appear every 60 seconds
tail -f /tmp/playbot.log  # (if you add logging)

# Terminal 4 — send commands
bash tests/bonus/test_bot.sh playbot  # (if adapted for playbot)
```

---

## File Manifest

```
include/irc/bot/
  BotCore.hpp       — Connection + poll transport
  BotCommands.hpp   — Command dispatch engine

src/bot/
  BotCore.cpp       — Socket, registration, reconnect logic
  BotCommands.cpp   — Command handlers
  BotMain.cpp       — Event loop (entry point)

docs/bonus/
  PLAYBOT_HOW_TO_USE.md  — This document
```

---

## Known Limitations & Future Work

- **IPv6 Support:** Currently IPv4-only (using `AF_INET` + `gethostbyname`)
  - Migration to `getaddrinfo()` would enable IPv6 + fallback to IPv4
- **!play Command:** Placeholder in `!help` teaser; full implementation in Part 2
- **Logging:** Currently writes to `stdout`/`stderr`; file logging would aid debugging
- **Max Connections:** Single server connection per bot instance

---

## Exit Codes

- `0` — Clean shutdown or user interrupt
- `1` — Connection failure, registration failure, max reconnect attempts exceeded

