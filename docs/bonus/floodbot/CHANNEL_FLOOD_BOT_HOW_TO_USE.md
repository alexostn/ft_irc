# ┌|∵|┘ FLOODBOT solves EAGAIN Silent Data Loss in sendToClient()
[problem problem and motivation behind the document described in send_eagain_fixed.md [o_0] CLICK TO READ](../../network/eagain_lost_in_send_to_client/send_eagain_fixed.md)
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
Wait for `[*] joined #test — listening` → **Ctrl+Z** ← freeze bot \
The process is still running, the TCP connection has not been closed, but the client is not reading incoming data from the socket.
```
zsh: suspended
```
## Terminal 3 — client A
```bash
nc -C 127.0.0.1 6667
```
```
PASS testpass
NICK clientA
USER clientA 0 * :Client A
JOIN #test
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

## Terminal 3 — unfreeze client A (fg - means foreground)
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

________________________________________________________________________________________________________________
## Interpretation of `353` and `366` messages

Standard IRC server responses to the `JOIN` command sent by the bot when connecting to `#test`.

---

### `[<<SVR] :ft_irc 353 flood_bot = #test :@flood_bot`

| Field | Value | Meaning |
|---|---|---|
| `353` | `RPL_NAMREPLY` | Server replies with channel member list |
| `flood_bot` | recipient | Who the reply is addressed to (the bot) |
| `=` | channel type | public channel (`=` public, `@` secret, `*` private) |
| `#test` | channel | channel name |
| `:@flood_bot` | nick list | `@` = bot has **operator** status in the channel |

---

### `[<<SVR] :ft_irc 366 flood_bot #test :End of /NAMES list`

| Field | Value | Meaning |
|---|---|---|
| `366` | `RPL_ENDOFNAMES` | End of member list |
| `flood_bot` | recipient | addressed to the bot |
| `#test` | channel | for this channel |

---

### Role in the test scenario

`353` + `366` = **confirmation that JOIN succeeded** and the client/bot is fully in the channel. The flood test must not be started until `366` is received — it is the readiness signal in both Terminal 2 (bot logs `joined #test — listening`) and Terminal 3 (client A freezes with Ctrl+Z only after seeing `366`).

`RPL_ENDOFNAMES (366)` is just a **terminator**. IRC first sends one or more `353` packets with nick lists, and `366` means "list is over, no more nicks coming".

Similar to HTTP `Content-Length` or EOF: the client has no way to know in advance how many `353` packets will arrive, so `366` tells it "JOIN can now be considered complete".

In this case the bot is alone in the channel, so there is only one `353` with a short list — `@flood_bot`. But the mechanism is identical for channels with thousands of members.