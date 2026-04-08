# PlayBot Documentation Index

All PlayBot documentation is organized in this folder: `docs/bonus/playbot/`

## Quick Documents

### (´▽｀)ノ Getting Started
- **[PLAYBOT_HOW_TO_USE.md](PLAYBOT_HOW_TO_USE.md)** — Complete usage guide
  - Build & run instructions
  - Interactive testing with `nc`
  - Command reference
  - Design patterns & non-blocking timing
  - C++98 compliance & memory management

### ٩(◕‿◕｡)۶ Architecture & Design
- **[PLAYBOT_ARCHITECTURE.md](PLAYBOT_ARCHITECTURE.md)** — Technical deep-dive (English)
  - Three-layer architecture (BotCore/BotCommands/BotMain)
  - Design principles (poll-driven alerts, transparent PING/PONG)
  - Code metrics & performance


### (*´∇｀*) Conceptual Overview
- **[MEANING_OF_PLAYBOT.md](MEANING_OF_PLAYBOT.md)** — What is playbot vs channelfloodbot
  - Quick comparison table
  - Basic test scenario
  - When to use each bot

---

## File Structure

```
docs/bonus/playbot/
├── PLAYBOT_HOW_TO_USE.md           (START HERE)
├── PLAYBOT_ARCHITECTURE.md         (deep-dive: English)
├── MEANING_OF_PLAYBOT.md           (quick conceptual overview)
└── README_PLAYBOT.md               (this file)
```

---

## Source Code Location

```
include/irc/bot/
├── BotCore.hpp       → Connection transport + poll()
└── BotCommands.hpp   → Command dispatch engine

src/bot/
├── BotCore.cpp       → ~190 lines: socket, reconnect, PING/PONG
├── BotCommands.cpp   → ~120 lines: PRIVMSG parse + 5 handlers
└── BotMain.cpp       → ~70 lines: event loop + timing
```

**Build:** `make playbot` → `./playbot 127.0.0.1 6667 testpass playbot #general`

---

## Which Document Should I Read?

| I want to... | Read... |
| :-- | :-- |
| **Just run it** | [PLAYBOT_HOW_TO_USE.md](PLAYBOT_HOW_TO_USE.md) — "Quick Start" section |
| **Understand the design** | [PLAYBOT_ARCHITECTURE.md](PLAYBOT_ARCHITECTURE.md) — "Three-Layer Architecture" |
| **Understand the code flow** | [PLAYBOT_ARCHITECTURE.md](PLAYBOT_ARCHITECTURE.md) — "Design Principles" section |
| **Compare with other bots** | [MEANING_OF_PLAYBOT.md](MEANING_OF_PLAYBOT.md) |
| **See if it's for me** | [MEANING_OF_PLAYBOT.md](MEANING_OF_PLAYBOT.md) — "What it is" & "How it differs" |


---

## Key Features At a Glance

(´▽｀)ノ **Poll-Driven Timing** — 60s alert without `sleep()` blocking  
(´▽｀)ノ **Transparent PING/PONG** — Connection stays alive automatically  
(´▽｀)ノ **Auto-Reconnect** — Up to 5 retries with 3s exponential backoff  
(´▽｀)ノ **C++98 Pure** — -Wall -Wextra -Werror, 0 memory leaks  
(´▽｀)ノ **5 Commands** — !ping !echo !time !uptime !help  
(ﾉ´ヮ´)ﾉ*:･ﾟ✧ **Foundation Ready** — Extensible for Part 2 (!play command)  

---

## Part 1 vs Part 2

**Part 1 (Completed):** BotCore infrastructure + basic command dispatch
- Connection + registration
- Message polling + alert timing
- Auto-reconnect
- 5 basic commands

**Part 2 (Planned):** Game integration (!play command)
- Game state machine integration
- Command routing to game logic
- No architectural changes needed

---

## Testing & Verification

All features covered by:
- (´▽｀) Compilation with strict C++98 flags
- (´▽｀) Manual testing with `nc`
- (´▽｀) Valgrind validation (0 leaks, 0 errors)
- (´▽｀) Alert timing precision tests

See [PLAYBOT_HOW_TO_USE.md](PLAYBOT_HOW_TO_USE.md#testing) for detailed test scenarios.

---

## Git Branch

- **network/03/play_bot_part1** — Current implementation branch
- **main** — When merged, playbot available in all builds

