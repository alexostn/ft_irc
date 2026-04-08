```
ft_irc/
├── ircserv                        ← main server binary (make re)
├── channel_flood_bot              ← bot binary (make bot)
├── Makefile
├── include/
│   └── commands/
├── src/
│   ├── Channel.cpp
│   ├── Server.cpp
│   ├── ...                        ← built as part of ircserv
│   └── bot/
│       └── channel_flood_bot.cpp  ← built only via make bot
└── bonus/
        ├── flood_channel.sh       ← flood test (200 PRIVMSG)
        └── test_bot.sh            ← bot command verification
		↑
		via make bot
		└── tests/
			└── bonus/
				├── flood_channel.sh       ← flood test (200 PRIVMSG)
				└── test_bot.sh            ← bot command verification
```

***

# Scenario VII + Bonus — Manual Test

> **Important:** open ALL terminals in the project root directory.
> ```bash
> cd ~/ft_irc_eval   # do this in every terminal before starting
> pwd                # must show the project root
> ```

### 0. Build

```bash
make re && make bot
```

***

## Terminal 1 — server (valgrind)
```bash
valgrind --leak-check=full --track-fds=yes ./ircserv 6667 testpass
```

## Terminal 2 — bot (valgrind)
```bash
./channel_flood_bot
```
Wait for `[*] joined #test — listening` → **Ctrl+Z** ← freeze bot
```
zsh: suspended
```
## Terminal 3 — client A
```bash
nc -C 127.0.0.1 6667
```
```
PASS testpass / NICK clientA / USER clientA 0 * :Client A / JOIN #test
```
Wait for `:ft_irc 366 ... End of /NAMES list` → **Ctrl+Z** ← freeze client A
```
zsh: suspended 
```
## Terminal 4 — flood
```bash
bash bonus/flood_channel.sh
```
Wait for script to finish.

## Terminal 3 — unfreeze client A
```bash
fg
```
✓ All 200 messages arrive → **Ctrl+C**

## Terminal 2 — unfreeze bot
```bash
fg
```
✓ `[<<SVR]` lines appear

## Terminal 5 — verify bot
```bash
bash bonus/test_bot.sh
```

***

## Terminal 2 — Expected output after Terminal 5 commands

After running `test_bot.sh`, Terminal 2 should show:

```
[<<SVR] :bottester!bottester@::ffff:127.0.0.1 PRIVMSG #test :!ping
[BOT>>] PRIVMSG #test :pong!

[<<SVR] :bottester!bottester@::ffff:127.0.0.1 PRIVMSG #test :!uptime
[BOT>>] PRIVMSG #test :uptime: 0h Xm Xs

[<<SVR] :bottester!bottester@::ffff:127.0.0.1 PRIVMSG #test :!echo HELLO EVALUATOR
[BOT>>] PRIVMSG #test :HELLO EVALUATOR
```

✓ All commands received and responded to correctly (IPv6-mapped hostname parsed correctly)

## Terminal 1 → stop server → valgrind
**Ctrl+C** → expect `0 bytes in 0 blocks`, `0 errors`

***

## Pass criteria

| Criterion | Expected Result |
|---|---|
| **Server stability** | No crash or hang after 200-message flood |
| **Flood reception** | Client A receives all 200 messages in burst (msg 1–200 visible in Terminal 3 after `fg`) |
| **Bot resilience** | Bot survives freeze + simultaneous flood; resumes cleanly after `fg` in Terminal 2 |
| **Bot command: `!ping`** | Bot replies: `PRIVMSG #test :pong!` (visible in Terminal 2) |
| **Bot command: `!uptime`** | Bot replies: `PRIVMSG #test :uptime: 0h Xm Xs` (visible in Terminal 2) |
| **Bot command: `!echo <msg>`** | Bot echoes: `PRIVMSG #test :HELLO EVALUATOR` (visible in Terminal 2) |
| **IPv6-mapped parsing** | Hostmask `:bot@::ffff:127.0.0.1` parses correctly; commands execute (not lost) |
| **Bot memory** | Valgrind: 0 bytes in 0 blocks, 0 errors |
| **Server memory** | Valgrind: 0 bytes in 0 blocks, 0 errors |