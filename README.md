*This project was created as part of the 42 curriculum by
[@sarherna](https://github.com/SaraitHernandez),
[@akacprzy](https://github.com/akacprzy),
[@oostapen](https://github.com/alexostn).

This is a clean study copy — original repo: [SaraitHernandez/irc_server](https://github.com/SaraitHernandez/irc_server)*

# ft_irc — Internet Relay Chat server

## Description

**ft_irc** is a TCP IRC server written in **C++98**. It accepts multiple simultaneous clients, parses IRC-style lines (CRLF-delimited), and implements authentication, channels, private and channel messaging, and channel operator commands with modes (`+i`, `+t`, `+k`, `+o`, `+l`).

The goal is to understand **non-blocking network I/O**: a single `poll()` loop, non-blocking sockets, buffering of partial reads, and safe client lifecycle (connect, register, commands, disconnect).

More protocol and design detail: [docs/IRC_LOGIC_AND_DATA_STRUCTURE.md](docs/IRC_LOGIC_AND_DATA_STRUCTURE.md) and [docs/TEAM_CONVENTIONS.md](docs/TEAM_CONVENTIONS.md).

## Instructions

### Prerequisites

- A C++ compiler with C++98 support (`c++` / `clang++` / `g++`)
- `make`
- Optional: `nc` (netcat) for quick manual tests; an IRC client (e.g. Halloy, irssi) for full client testing

### Compilation

```bash
make
```

`make re` cleans and rebuilds. The binary is **`ircserv`**.

### Execution

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 mypassword
```

The server listens on **TCP IPv4** on the given port. Clients must send `PASS` with the configured password before `NICK` / `USER` (strict order: PASS → NICK → USER).

### Quick client example (netcat)

```text
nc localhost 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice Smith
JOIN #test
PRIVMSG #test :Hello
QUIT :Bye
```

### Automated tests (optional)

With the server already running:

```bash
./tests/run_manual_tests.sh [port] [password]
```

Defaults: port `8080`, password `test`. See [tests/MANUAL_TESTING_GUIDE.md](tests/MANUAL_TESTING_GUIDE.md) for manual scenarios.

### Memory checks

- **macOS:** `leaks --atExit -- ./ircserv 6667 test123` (then exercise the server and stop it)
- **Linux:** `valgrind --leak-check=full ./ircserv 6667 test123`

## Features (summary)

| Area | Commands / behaviour |
|------|----------------------|
| Connection | `PASS`, `NICK`, `USER`, `PING` / `PONG`, `QUIT` |
| Channels | `JOIN`, `PART`, `PRIVMSG` (users and channels) |
| Operators | `KICK`, `INVITE`, `TOPIC`, `MODE` |
| Modes | `+i` invite-only, `+t` topic lock, `+k` key, `+o` op, `+l` user limit |

Technical constraints: **one** `poll()`, non-blocking `fcntl`, message reassembly via a buffer, case-insensitive nick/channel matching with display case preserved.

## Resources

### IRC and networking

- [RFC 2812](https://datatracker.ietf.org/doc/html/rfc2812) — Internet Relay Chat: Client Protocol  
- [RFC 2811](https://datatracker.ietf.org/doc/html/rfc2811) — Channel Management (modes, context)  
- [RFC 1459](https://datatracker.ietf.org/doc/html/rfc1459) — Original IRC (historical)  
- [Beej’s Guide to Network Programming](https://beej.us/guide/bgnet/) — sockets, `poll`, error handling  

### Project documentation

- [docs/TEAM_CONVENTIONS.md](docs/TEAM_CONVENTIONS.md) — module boundaries and reply formats  
- [docs/IRC_LOGIC_AND_DATA_STRUCTURE.md](docs/IRC_LOGIC_AND_DATA_STRUCTURE.md) — data structures and flows  
- [tests/MANUAL_TESTING_GUIDE.md](tests/MANUAL_TESTING_GUIDE.md) — evaluation-oriented test checklist  

### Use of AI

AI-assisted tools (e.g. coding assistants) were used **non-exclusively** for: drafting or revising documentation, brainstorming test ideas, and occasional code review or refactoring suggestions. **Design decisions, protocol behaviour, and final code** remain the responsibility of the authors.

## License

This repository is part of the 42 curriculum and is intended for educational use.
