# playbot — Bonus Bot (ﾉ´ヮ´)ﾉ*:･ﾟ✧

IRC bot for **ft_irc**. Connects as a client, dispatches commands, auto-reconnects.

---

## Run

```bash
./ircserv 6667 testpass            # T1
./playbot 127.0.0.1 6667 testpass playbot #general  # T2
bash bonus/test_playbot.sh         # T3
```

---

## Commands  (￣ー￣)ゞ～✔～✔～✔

| Command | Response | Notes |
|---|---|---|
| `!ping` | `pong!` | static |
| `!echo <text>` | `<text>` | PM-aware |
| `!time` | `Thu Mar 26 00:57:58 2026` | `ctime()` stripped `\n` |
| `!uptime` | `uptime: 0h 0m 12s` | `difftime(now, start_time)` |
| `!help` | `commands: … \| coming soon: !play` | static |

---

## Eval Checklist  (･_･)ノ✎

```
[✔] (ﾉ゜▽゜)ﾉ !ping      → pong!
[✔] (ﾉ゜▽゜)ﾉ !echo      → echoes text, PM-aware
[✔] (ﾉ゜▽゜)ﾉ !time      → wall-clock, no trailing \n
[✔] (ﾉ゜▽゜)ﾉ !uptime    → Xh Ym Zs (never resets on reconnect)
[✔] (ﾉ゜▽゜)ﾉ !help      → list + coming soon: !play
[✔] (ﾉ゜▽゜)ﾉ MONITOR    → [PLAYBOT] Status OK every 60s via poll()
[✔] (ﾉ゜▽゜)ﾉ reconnect  → sleep(3) → retry up to 5x
★ミ(o*･ω･)ﾉ  ALL DONE!
```

---

## Key Design Notes

- `start_time_` set **once** in constructor — never reset on reconnect
- Alert timer driven by `poll()` timeout — no `sleep()` in event loop
- `target[0] == '#'` → reply to channel; else → reply to sender (PM)
- PING/PONG handled transparently in `BotCore::tick()`

---

## Architecture

```
(･_･)ノ [playbot]
  BotCore      — TCP socket, poll(), PASS/NICK/USER/JOIN, PING/PONG
  BotCommands  — PRIVMSG parser, command dispatch, sendAlert()
  BotMain      — event loop, alert interval, reconnect logic
```

(•̀ᴗ•́)و ✧ ✔✔✔  
