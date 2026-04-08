## Correct Terminal Order

```bash
# Terminal 1 — Server
./ircserv 6667 testpass
```

```bash
# Terminal 2 — Client A (pause it with Ctrl+Z)
nc -C 127.0.0.1 6667
```
Then manually enter the handshake:
```
PASS testpass
NICK clientA
USER clientA 0 * :Client A
JOIN #test
```
Once you’ve confirmed the JOIN-ack has arrived → **Ctrl+Z** ← freeze

```bash
# Terminal 3 — flood from Client B
bash tests/flood_channel.sh
```

```bash
# Terminal 2 — unfreeze Client A
fg
# ✓ all 200 messages should arrive in the buffer
```

***

## What each step checks

| Step | What is checked |
|---|---|
| Client A performs JOIN before freezing | the server knows that A is in the channel and buffers for it |
| Ctrl+Z (not Ctrl+C) | process is **suspended**, socket remains open — server sees a live connection |
| flood from Client B | server does not hang with a slow/frozen subscriber |
| `fg` → messages arrive | server correctly delivered the backlog — write buffering check |

> **Important:** `Ctrl+C` would kill `nc` completely (this is a V/VI scenario), while `Ctrl+Z` only **suspends** it — the socket remains active, and the server continues to write to it until the kernel’s send buffer overflows.

```
Terminal 1          Terminal 2 (clientA)        Terminal 3 (flood)
──────────          ────────────────────        ──────────────────
./ircserv           nc -C 127.0.0.1 6667
6667 testpass       PASS testpass
                    NICK clientA
                    USER clientA 0 * :Client A
                    JOIN #test
                    
                    Ctrl+Z  ←── freeze
                    [1]+ Stopped nc...
                    $                           bash tests/flood_channel.sh
                                                (200 messages are sent)
                    
                    fg  ←── enter HERE
                    (all 200 messages are sent)
```
Ctrl+Z and fg always go together in a single terminal
fg restores the exact process that was suspended in that same shell

### var 1 NO NEED CHMOD
```
bash tests/flood_channel.sh
```

### var 2 CHMOD
```
chmod +x tests/flood_channel.sh && ./tests/flood_channel.sh
```
____________________________________________________________
# possible results
That's right—`fg` worked, and `nc` resumed. 

Now check to see **if any messages appeared** in this terminal after `fg`:

```
:floodbot!floodbot@127.0.0.1 PRIVMSG #test :msg 1
:floodbot!floodbot@127.0.0.1 PRIVMSG #test :msg 2
:floodbot!floodbot@127.0.0.1 PRIVMSG #test :msg 3
...
:floodbot!floodbot@127.0.0.1 PRIVMSG #test :msg 200
```

## Three Possible Outcomes

| What you see after `fg` | What it means |
|---|---|
| All 200 messages arrived |  Test passed — the server buffered correctly |
| Fewer than 200 arrived |  Some were lost — problem with `write` buffering |
| Nothing at all |  the server does not buffer for frozen clients |

> `[2] - continued` means this was the second background job in zsh — this is normal; the main thing is that `nc` continued running.

_________________________________________________________
# etymology
**`fg`** is a built-in Unix shell command for **job control** (task/process management), and its etymology is extremely simple.

## Etymology

**`fg`** = an abbreviation of **fore**·**g**round 

- **fore** (English, archaic) — “before, in front of” (from Proto-Germanic *fura*, cognate with German *vor*)
- **ground** — here in the sense of “execution environment,” not “ground”

The pair `fg` / `bg` is symmetrical: **bg** = **b**ack**g**round (background mode).

## What `fg` does

`fg` brings a **job** (a background or paused process) back to the foreground — that is, it makes it active again in the current terminal and connects it to stdin/stdout.

```bash
./ircserv 6667 password &   # run server in background
fg                          # bring it back to foreground
fg %1                       # bring job #1 specifically
```

## Connection to the IRC project

In the context of `ft_irc`, this is useful for debugging: you start the server in the background using `&`, test connections via `nc` in another terminal, and then use `fg` to bring the server back to the foreground to view its output or terminate it with `Ctrl+C`. For `valgrind` memory leak checks (required during evaluation), this is a convenient way to switch between sessions. 
