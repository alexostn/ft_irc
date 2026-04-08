# Bot Comparison: FloodBot vs PlayBot

┌|∵|┘ Quick Overview

Two bots built for ft_irc bonus testing. Each solves different problems.

---

## Side-by-Side Comparison

| Aspect | FloodBot | PlayBot |
|--------|----------|---------|
| **Purpose** | Stress test: flood channel with messages | Testing: respond to commands + periodic alerts |
| **Architecture** | Monolithic (all in one `.cpp`) | Layered (BotCore + BotCommands + BotMain) |
| **Lines of Code** | ~200 lines (self-contained) | ~380 lines (3 modules) |
| **Reconnects?** | [-] No reconnect logic | [+] Auto-reconnect on drop |
| **Command Handlers** | [-] No command handling | [+] Yes: !ping, !echo, !time, !uptime, !help |
| **Periodic Actions** | [-] Just floods | [+] Status alert every 60s |
| **Poll Strategy** | select() — waits for server responses | poll() with timeout — drives timing |
| **Reusable Code** | [-] Monolithic, hard to reuse | [+] BotCore is library-level |
| **Use Case** | **Load testing** — DoS-like | **Bot interaction** — conversation |

---

## FloodBot ┌|∵|┘~~~

**What it does:**
- Connects to IRC server
- Joins a channel
- Floods with messages (stress test)
- Exits when done

**Structure:**
```
main() {
    connect()
    register()
    join()
    send_lots_of_messages()
    exit()
}
```

**Pros:**
- [+] Simple: read & understand in 5 minutes
- [+] Good for: "Does server crash under load?"

**Cons:**
- [-] Dies if connection drops
- [-] No logic, just hammering
- [-] Hard to reuse (monolithic)

**Use Command:**
```bash
./channel_flood_bot 127.0.0.1 6667 testpass floodbot #general
```

---

## PlayBot └[∵┌] 

**What it does:**
- Connects to IRC server
- Joins a channel
- Sends status every 60 seconds
- Responds to commands: !ping, !echo, !time, !uptime, !help
- Auto-reconnects if connection drops

**Structure:**
```
BotMain (loop) {
    BotCore (transport) →  poll(), send, receive
    BotCommands (logic) →  parse, route, alert
}
```

**Pros:**
- [+] Sophisticated: handles chaos (network drops)
- [+] Responds to users
- [+] Reusable: BotCore can power other bots
- [+] Clean separation: network ≠ logic ≠ timing

**Cons:**
- [-] More code (but well-organized)
- [-] Overfull for simple flood testing

**Use Command:**
```bash
./playbot 127.0.0.1 6667 testpass playbot #general
```

---

## Quick Decision Tree

```
Need to stress test? 
  └─ Yes → Use FloodBot (fast, simple)
  └─ No  → Interactive bot needed?
         └─ Yes → Use PlayBot (features, stability)
         └─ No  → Build custom with BotCore (reusable!)
```

---

## Checklist: When to Use

- [OK] (･ω･)ノ FloodBot: "Will server survive 1000 msgs/sec?"
- [OK] (･ω･)ノ FloodBot: "Quick chaos test, then exit"
- [OK] (･ω･)ノ PlayBot: "Bot should respond to commands"
- [OK] (･ω･)ノ PlayBot: "Need resilience (auto-reconnect)"
- [OK] (･ω･)ノ PlayBot: "Building second bot? Reuse BotCore"
- [OK] (･ω･)ノ FloodBot: Code is readable enough? Copy-modify

---

## File Locations

```
src/bot/
├── channel_flood_bot.cpp     FloodBot (monolithic)
├── BotCore.cpp               PlayBot transport layer
├── BotCommands.cpp           PlayBot command handlers
└── BotMain.cpp               PlayBot event loop
```

