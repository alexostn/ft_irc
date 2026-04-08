# playbot — IRC Bot Commands Reference

All five commands are `PRIVMSG` handlers: the bot receives them from the channel and replies back to the same channel.

## General Flow for All Commands

```
nc sends:        PRIVMSG #general :!ping
server→bot:      :you!u@h PRIVMSG #general :!ping
bot parses trailing (:!ping), checks first word, calls handler
bot replies:     PRIVMSG #general :pong!
```

---

## !ping

The simplest command — checks whether the bot is alive:

```
T3 (nc):   PRIVMSG #general :!ping
PlayBot:   PRIVMSG #general :pong!
```

No logic — just a static reply. Used during evaluation to confirm the bot is connected and reading the channel.

---

## !echo \<text\>

The bot repeats everything after `!echo`:

```
T3:       PRIVMSG #general :!echo hello world
PlayBot:  PRIVMSG #general :hello world
```

Parsing: take the trailing string after `!echo ` (with the space). If no text is provided, reply with `:echo: no text provided`.

```cpp
if (cmd == "!echo") {
    std::string text = (args.empty() ? "no text provided" : args);
    send_privmsg(fd, channel, text);
}
```

---

## !time

Returns the current system time of the machine running the bot:

```
T3:       PRIVMSG #general :!time
PlayBot:  PRIVMSG #general :current time: 2026-03-25 23:41:07
```

```cpp
time_t now = time(NULL);
struct tm *t = localtime(&now);
char buf[64];
strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
send_privmsg(fd, channel, "current time: " + std::string(buf));
```

Difference from `!uptime` — shows **absolute** wall-clock time, not relative elapsed time.

---

## !uptime

Shows how long the bot has been running **continuously** since launch:

```
T3:       PRIVMSG #general :!uptime
PlayBot:  PRIVMSG #general :uptime: 0h 2m 14s
```

```cpp
long s = (long)(time(NULL) - start_time);
long h = s / 3600;
long m = (s % 3600) / 60;
long sec = s % 60;
// → "0h 2m 14s"
```

`start_time` is set **once** in `main()` — it must **not** be reset on reconnect, otherwise the true bot uptime is lost.

---

## !help

Prints the list of available commands and announces upcoming ones:

```
T3:       PRIVMSG #general :!help
PlayBot:  PRIVMSG #general :commands: !ping, !echo <text>, !time, !uptime | coming soon: !play
```

Fully static string — just a hardcoded reply. `coming soon: !play` hints at a future game command (ft_irc bonus).

---

## Command Differences

| Command | Arguments | Logic | Stateful |
|---|---|---|---|
| `!ping` | none | static | no |
| `!echo <text>` | required | reflects args | no |
| `!time` | none | `localtime(time(NULL))` | no |
| `!uptime` | none | `time(NULL) - start_time` | ✅ `start_time` |
| `!help` | none | static | no |

---

## Single-Function Dispatcher (C++98)

```cpp
void dispatch(int fd, const std::string &channel,
              const std::string &trailing, time_t start_time)
{
    // trailing = "!echo hello" → cmd="!echo", args="hello"
    size_t sp = trailing.find(' ');
    std::string cmd  = trailing.substr(0, sp);
    std::string args = (sp == std::string::npos) ? "" : trailing.substr(sp + 1);

    if      (cmd == "!ping")   send_privmsg(fd, channel, "pong!");
    else if (cmd == "!echo")   send_privmsg(fd, channel, args.empty() ? "no text" : args);
    else if (cmd == "!time")   send_privmsg(fd, channel, get_time());
    else if (cmd == "!uptime") send_privmsg(fd, channel, "uptime: " + format_uptime(start_time));
    else if (cmd == "!help")   send_privmsg(fd, channel,
        "commands: !ping, !echo <text>, !time, !uptime | coming soon: !play");
}
```
[PLAYBOT_HOW_TO_USE.md](PLAYBOT_HOW_TO_USE.md)