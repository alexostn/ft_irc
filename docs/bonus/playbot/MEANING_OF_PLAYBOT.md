# playbot — quick brief

## 1. What it is

An always-on bot that lives in the channel, answers simple commands, and posts a "still alive" message every 60 seconds. Like a watchman who checks in every hour.

## 2. How it differs from channelfloodbot

|  | channelfloodbot | playbot |
| :-- | :-- | :-- |
| Purpose | stress-test the server | permanent channel resident |
| Event loop | `select()` — waits for data | `poll(timeout)` — wakes on timer |
| Alert timer | no | yes — every 60s |
| Reconnect | no | yes — up to 5 attempts |
| Architecture | one monolithic `.cpp` | `BotCore / BotCommands / BotMain` |
| Commands | `!ping !echo !time !uptime !help` | same + teaser `coming soon: !play` |

## 3. How to test

```bash
./ircserv 6667 testpass          # T1 server
./playbot 127.0.0.1 6667 testpass  # T2 bot
nc -C 127.0.0.1 6667             # T3 you

PRIVMSG #general :!ping    → pong!
PRIVMSG #general :!uptime  → uptime: 0h 2m 14s
PRIVMSG #general :!help    → list + "coming soon: !play"
# after 60s: [MONITOR] OK | uptime 0h 1m 0s
# kill bot → waits 3s → reconnects
```


## 4. Path to Transcendance

```
playbot (now)         BotCore — connect, listen, reply, timer
  └── + BotLife       ColorRoom, !play !track !like !skip, Life rules
        └── + Trans   BotCore→WebSocketCore  ColorRoom→GameLobby
                      sendRaw(PRIVMSG) → ws.send(JSON)  — logic unchanged
```
....continuation is open to discussion
