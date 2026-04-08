# ft_irc — Evaluation Command Checklist

> Concise reference for the defence. Run checks in order.
> Any crash, segfault, or unexpected exit → **final grade 0**.

---

## 🔍 Pre-flight / Basic checks

```bash
# Clone the repo fresh
git clone <repo_url> ./ft_irc_eval && cd ./ft_irc_eval

# Build
make
# ✓ must compile with no errors
# ✓ executable must be named: ircserv

# Count poll() (or equivalent: select / epoll / kqueue)
grep -rn "poll\|select\|epoll\|kqueue" . --include="*.cpp" --include="*.hpp"
# ✓ must find exactly ONE call

# Check fcntl() usage — only this form is allowed
grep -rn "fcntl" . --include="*.cpp" --include="*.hpp"
# ✓ every call must match: fcntl(fd, F_SETFL, O_NONBLOCK)
# ✗ any other flag (F_GETFL etc.) → grade 0

# Check errno abuse after poll/recv/send
grep -n "EAGAIN\|errno" . -r --include="*.cpp"
# ✓ errno must NOT be used to re-trigger reads/writes after poll
```

> **RU:** Клонируем репо в пустую папку. Проверяем компиляцию, ровно один `poll()`,
> только `fcntl(fd, F_SETFL, O_NONBLOCK)`, никакого `errno == EAGAIN` для повторного чтения.

---

## 🌐 Networking — basic connectivity

```bash
# Start the server (choose any free port)
./ircserv 6667 testpass

# Terminal 2 — raw nc connection
nc -C 127.0.0.1 6667
# ✓ server must respond (no hang, no crash)

# Send a minimal IRC handshake manually via nc
PASS testpass
NICK testnick
USER testuser 0 * :Test User
# ✓ server must reply with welcome numerics (001, 002, 003, 004)

# Check server listens on all interfaces
ss -tlnp | grep 6667      # Linux
netstat -an | grep 6667   # macOS
# ✓ 0.0.0.0:6667 or *:6667
```

> **RU:** Запускаем сервер, подключаемся через `nc`. Проверяем что слушает на всех интерфейсах.
> Отправляем IRC-хэндшейк вручную — должны прийти приветственные нумерики.

---

## ⚡ Networking — multiple simultaneous connections

```bash
# Terminal 2 — nc client
nc -C 127.0.0.1 6667

# Terminal 3 — reference IRC client (e.g. irssi, weechat, hexchat)
# Connect to 127.0.0.1:6667 with password testpass

# Terminal 4 — second nc client
nc -C 127.0.0.1 6667

# ✓ all three must be connected and responsive at the same time
# ✓ server must NOT block when one client is slow

# Join a channel and verify broadcast
# In reference client:
/join #test
# In nc (after handshake):
JOIN #test
PRIVMSG #test :hello from nc
# ✓ reference client must receive the message
```

> **RU:** Подключаем одновременно `nc` и IRC-клиент. Проверяем что сообщение в канале
> доходит до всех участников. Сервер не должен подвисать.

---

## 🧩 Networking specials — edge cases

### Partial command via nc

```bash
nc -C 127.0.0.1 6667
# Type "com" then press Ctrl+D  → sends "com" without newline
# Type "man" then press Ctrl+D  → sends "man"
# Type "d"   then press Enter   → sends "d\n"
# ✓ server must NOT crash and must NOT garble other clients' sessions
```

> **RU:** Отправляем команду по кускам через `Ctrl+D`. Сервер должен собрать пакеты
> и обработать команду целиком. Другие подключения должны работать нормально.

### Kill a client abruptly

```bash
# Connect nc, then in another terminal:
kill -9 <nc_pid>
# ✓ server stays up, other clients unaffected
# ✓ new connections still accepted
```

> **RU:** Убиваем клиент через `kill -9`. Сервер должен выжить и принимать новые подключения.

### Kill nc mid-command

```bash
nc -C 127.0.0.1 6667
# Type half a command, e.g. "PRIVMSG #tes"  (no newline)
# Kill the nc process: Ctrl+C or kill -9
# ✓ server must NOT hang or enter broken state
```

> **RU:** Рвём соединение посреди команды. Сервер не должен зависнуть.

### Freeze a client, flood the channel

```bash
# Connect client A to #test, then suspend it:
Ctrl+Z   # in client A's terminal

# From client B, flood the channel:
for i in $(seq 1 200); do echo "PRIVMSG #test :msg $i" | nc -C 127.0.0.1 6667; done
# or send rapidly from the reference client

# ✓ server must NOT hang
# ✓ resume client A: fg
# ✓ stored messages must be delivered to client A
# ✓ check for memory leaks: valgrind ./ircserv 6667 testpass
```

> **RU:** Зависаем клиент A (`Ctrl+Z`), флудим канал с клиента B. Сервер не должен вешаться.
> Размораживаем A (`fg`) — все сообщения должны дойти. Проверяем утечки памяти.

---

## 💬 Client commands — basic

```bash
# Via nc (manual handshake first: PASS / NICK / USER)

# Authenticate
PASS testpass

# Set nickname
NICK alice

# Set username
USER alice 0 * :Alice Smith

# Join a channel
JOIN #general

# Send a message to channel
PRIVMSG #general :Hello everyone

# Send a private message to a user
PRIVMSG bob :Hey Bob, private message

# ✓ test ALL of the above from both nc AND the reference IRC client
```

> **RU:** Проверяем аутентификацию, ник, юзернейм, вход в канал, сообщение в канал,
> личное сообщение — и через `nc`, и через IRC-клиент.

---

## 👑 Channel operator commands

> First confirm a regular user **cannot** run these. Then confirm an operator **can**.

```bash
# ── Setup ──────────────────────────────────────────────────
# Operator:  connect as 'alice', join #ops  (first joiner = operator)
# Regular:   connect as 'bob',   join #ops

# ── KICK ───────────────────────────────────────────────────
KICK #ops bob :you are out
# ✓ bob must be removed from the channel
# ✗ bob trying KICK on alice → must receive ERR_CHANOPRIVSNEEDED (482)

# ── INVITE ─────────────────────────────────────────────────
INVITE charlie #ops
# ✓ charlie receives an invitation and can join

# ── TOPIC ──────────────────────────────────────────────────
TOPIC #ops :New topic here
# ✓ topic is set and broadcast to channel members
TOPIC #ops
# ✓ returns current topic

# ── MODE i  (invite-only) ──────────────────────────────────
MODE #ops +i
# ✓ channel is now invite-only; uninvited JOIN must be rejected
MODE #ops -i
# ✓ channel open again

# ── MODE t  (topic restricted) ────────────────────────────
MODE #ops +t
# ✓ only operators can change topic now
# ✗ bob trying TOPIC → must fail with ERR_CHANOPRIVSNEEDED (482)
MODE #ops -t

# ── MODE k  (channel key / password) ──────────────────────
MODE #ops +k secret123
# ✓ joining without key must fail
JOIN #ops secret123
# ✓ joining with correct key must succeed
MODE #ops -k

# ── MODE o  (give/take operator) ──────────────────────────
MODE #ops +o bob
# ✓ bob is now an operator
MODE #ops -o bob
# ✓ bob is a regular user again

# ── MODE l  (user limit) ──────────────────────────────────
MODE #ops +l 2
# ✓ third user trying to join must receive ERR_CHANNELISFULL (471)
MODE #ops -l
# ✓ limit removed
```

> **RU:** Сначала проверяем что обычный пользователь не может выполнять операторские команды.
> Затем проверяем каждую команду от имени оператора. Убираем балл за каждую нерабочую фичу.

---

## 🧹 Memory leaks check

```bash
# Linux
valgrind --leak-check=full --track-fds=yes ./ircserv 6667 testpass

# macOS
leaks --atExit -- ./ircserv 6667 testpass

# Run a full session: connect clients, send messages, disconnect — then stop server
# ✓ no heap blocks definitely/indirectly lost
```

> **RU:** Запускаем под valgrind / leaks. Проводим полноценную сессию — коннект, команды,
> дисконнект — и смотрим на утечки после остановки сервера.

---

## ⭐ Bonus (only if mandatory is perfect)

```bash
# File transfer — test via reference IRC client DCC SEND
# ✓ file transfers successfully end-to-end

# Bot — interact with the bot in a channel or via private message
# ✓ bot responds meaningfully to at least one command
```

> **RU:** Бонус проверяется только если мандатори прошёл без единого замечания.
> Тестируем передачу файлов через IRC-клиент и взаимодействие с ботом.

---

## 🚩 Quick flag reference

| Situation | Action |
|---|---|
| Empty repository | Flag: empty repo → grade 0 |
| Does not compile | Flag: non-functioning → grade 0 |
| Crash / segfault at any point | Flag: crash → grade 0 |
| More than one `poll()` | Grade 0 |
| `fcntl()` used with wrong flags | Grade 0 |
| Memory leaks confirmed | Flag: memory leak |
| Suspected cheating | Flag: cheat → grade -42 |

---

*Based on ft_irc subject v10.0 and the official evaluation sheet.*
