# Manual Testing Guide - ft_irc

Complete manual testing guide
Run each test, verify the expected result, and check off the box.

---

## Setup

```bash
# Terminal 1: Start server
make re && ./ircserv 6667 test123

# Terminal 2: Connect with nc
nc localhost 6667

# Terminal 3: Second client (when needed)
nc localhost 6667
```

> **Tip:** Every command you type in `nc` is sent with just `\n`.
> The server handles both `\n` and `\r\n`, so manual testing with `nc` works fine.

---

## Section 1 — Basic Checks (eval sheet)

> *"If any of these points is wrong, the evaluation ends now and the final mark is 0."*

### 1.1 Compilation

```bash
make re
```

- [ ] Project compiles with no errors
- [ ] No compiler warnings (`-Wall -Wextra -Werror`)
- [ ] Executable is named `ircserv`
- [ ] Written in C++ (C++98)

### 1.2 fcntl usage

```bash
grep -rn "fcntl" src/ include/
```

- [ ] Every `fcntl()` call uses exactly: `fcntl(fd, F_SETFL, O_NONBLOCK)`
- [ ] No other flags or uses of `fcntl()`

### 1.3 Single poll()

```bash
grep -rn "poll(" src/ include/ | grep -v "//"
```

- [ ] There is exactly **one** `poll()` call in the entire codebase
- [ ] `poll()` is called before every `accept`, `recv`, `send`

### 1.4 Server startup

```bash
./ircserv 6667 test123
```

- [ ] Server starts without error
- [ ] Listens on the specified port
- [ ] No crash, no segfault

### 1.5 Memory leaks

```bash
# macOS:
leaks --atExit -- ./ircserv 6667 test123
# Then connect, run commands, disconnect, Ctrl+C the server

# Linux:
valgrind --leak-check=full ./ircserv 6667 test123
```

- [ ] 0 memory leaks reported
- [ ] All heap allocations properly freed

---

## Section 2 — Networking (eval sheet)

### 2.1 Basic connection with nc

**Terminal 2:**
nc -z
-z (zero I/O mode) — simply checks whether the port is open; it does not send anything. This is the standard way to verify that the server is already listening:
```
nc -z 127.0.0.1 6667 && echo "up" || echo "down"

up

```
```
nc 127.0.0.1 6667
PASS test123
NICK alice
USER alice 0 * :Alice Smith
```

our implementation does not need to proceed <hostname> <servername>
designed for server-to-server standard In modern practice clients like Halloy send 0 and * there automatically
needed by the WHO and WHOIS commands filled automatically
howhever it saves it and WHO or WHOIS would easily access if needed (call to write)
```
USER <username> <hostname> <servername> :<realname>
USER  alice      0           *           :Alice Smith
```
- [ ] Server responds with 4 welcome messages (001, 002, 003, 004)
- [ ] Connection stays open
- [ ] Client can send further commands

### 2.2 Connection with IRC client (reference client)

> Our reference client is **Halloy** (also tested with **irssi**).

Connect with your chosen IRC client to `localhost:6667` with password `test123`.

- [ ] Client connects successfully
- [ ] Registration completes (welcome messages)
- [ ] Client can join channels and send messages

### 2.3 Multiple simultaneous connections

Open **3 terminals** with `nc localhost 6667` and register each:

**Client 1:**
```
PASS test123
NICK alice
USER alice 0 * :Alice
```

**Client 2:**
```
PASS test123
NICK bob
USER bob 0 * :Bob
```

**Client 3:**
```
PASS test123
NICK charlie
USER charlie 0 * :Charlie
```

- [ ] All 3 clients connect and register simultaneously
- [ ] Server does not block or slow down
- [ ] Each client gets independent responses

### 2.4 Channel messaging reaches all members

All 3 clients join the same channel:
```
JOIN #test
```

Client 1 sends:
```
PRIVMSG #test :Hello everyone!
```

- [ ] Client 2 receives the message
- [ ] Client 3 receives the message
- [ ] Client 1 does NOT receive their own message back

### 2.5 nc and IRC client at the same time

Connect one `nc` client and one IRC client (irssi/Halloy) to the same server.
Both join `#mixed`. Send messages from both.

- [ ] Messages from `nc` appear in the IRC client
- [ ] Messages from the IRC client appear in `nc`
- [ ] Server handles both without issues

---

## Section 3 — Networking Specials (eval sheet)

> *"Network communications can be disturbed by many strange situations."*

### 3.1 Frozen client (Ctrl+Z) and flood

1. Connect **Client A** and **Client B**, both join `#flood`
2. **Freeze Client A** with `Ctrl+Z` in its terminal
3. From Client B, send 20+ messages rapidly:
   ```
   PRIVMSG #flood :msg1
   PRIVMSG #flood :msg2
   ...
   PRIVMSG #flood :msg20
   ```
4. **Unfreeze Client A** with `fg`

- [ ] Server does NOT hang while Client A is frozen
- [ ] Client B can still send messages normally
- [ ] When Client A unfreezes, it receives all buffered messages
- [ ] No memory leaks during this operation
- [ ] Server remains responsive throughout

### 3.2 Kill nc with partial command

1. Connect with `nc localhost 6667`
2. Register normally
3. Type a partial command (do NOT press Enter):
   ```
   PRIVMSG #te
   ```
4. Kill `nc` with `Ctrl+C`

- [ ] Server does NOT crash
- [ ] Server does NOT block
- [ ] Other connected clients are unaffected
- [ ] New clients can still connect

### 3.3 Unexpectedly kill a client

1. Connect Client A and Client B to `#kill`
2. Kill Client A's terminal window (close it, or `kill -9` the nc process)

- [ ] Server continues running normally
- [ ] Client B is unaffected and can still send messages
- [ ] New clients can connect after the kill
- [ ] Client A is properly removed from channels

### 3.4 Partial commands via nc

Send data byte-by-byte or in chunks:

```bash
# Method 1: Using printf with separate sends
(printf "PASS "; sleep 0.5; printf "test1"; sleep 0.5; printf "23\n"; sleep 0.5; printf "NICK al"; sleep 0.5; printf "ice\n"; sleep 0.5; printf "USER alice 0 * :Alice\n"; sleep 1) | nc localhost 6667
```

- [ ] Server correctly reassembles partial data
- [ ] Welcome messages are received after full registration
- [ ] Other connections are not affected while waiting for partial data

---

## Section 4 — Authentication

> **IRC numeric reply format (RFC 2812):** Replies look like  
> `:servername numeric nickname_or_* :message`  
> Before you have a **nickname**, the server uses `*` in the nickname slot.  
> Example: `:ft_irc 464 * :Password incorrect` — same meaning as “464 :Password incorrect” in short form.  
> This is **correct**; not a bug.

### 4.1 Successful registration (PASS → NICK → USER)

```
PASS test123
NICK alice
USER alice 0 * :Alice Smith
```

- [ ] Receives `001` RPL_WELCOME
- [ ] Receives `002` RPL_YOURHOST
- [ ] Receives `003` RPL_CREATED
- [ ] Receives `004` RPL_MYINFO
- [ ] Prefix includes real IP: `alice!alice@127.0.0.1`

### 4.2 Wrong password

```
PASS wrongpassword
```

- [ ] Receives `464` with trailing `Password incorrect` (full form: `:ft_irc 464 * :Password incorrect` — `*` is the nick slot before registration)
- [ ] Can retry with correct password

### 4.3 Password retry limit (3 attempts)

```
PASS wrong1
PASS wrong2
PASS wrong3
```

- [ ] Receives `464` error for each attempt
- [ ] Disconnected after 3rd failed attempt

### 4.4 NICK before PASS (wrong order)

```
NICK alice
```

- [ ] Receives `451` (full form: `:ft_irc 451 * :You have not registered`)

### 4.5 USER before NICK (wrong order)

```
PASS test123
USER alice 0 * :Alice
```

- [ ] Receives `451` (full form: `:ft_irc 451 * :You have not registered`)

### 4.6 Commands before registration

```
JOIN #test
PRIVMSG #test :hello
```

- [ ] Receives `451 :You have not registered` for each command (nick slot may be `*` before registration)

### 4.7 Empty NICK

```
PASS test123
NICK
```

- [ ] Receives `431 :No nickname given`

### 4.8 Duplicate nickname

**Client 1** registers as `alice`. **Client 2** tries:

```
PASS test123
NICK alice
```

- [ ] Client 2 receives `433 :Nickname is already in use`

### 4.9 Case-insensitive duplicate nickname

**Client 1** registers as `Alice`. **Client 2** tries:

```
PASS test123
NICK ALICE
```

- [ ] Client 2 receives `433 :Nickname is already in use`
- [ ] Case-insensitive comparison works

### 4.10 Nickname change after registration

```
PASS test123
NICK oldnick
USER old 0 * :Old
NICK newnick
```

- [ ] Receives `:oldnick!old@127.0.0.1 NICK :newnick`
- [ ] New nickname is active for subsequent commands

### 4.11 Change NICK to one already in use

**Client 1** is `alice`. **Client 2** is `bob`. Client 2 tries:

```
NICK alice
```

- [ ] Receives `433 :Nickname is already in use`
- [ ] Client 2 keeps `bob` as nickname

### 4.12 Invalid nickname characters

```
PASS test123
NICK #invalid
```

- [ ] Receives `432 :Erroneous nickname`

### 4.13 Double registration (USER twice)

```
PASS test123
NICK alice
USER alice 0 * :Alice
USER alice2 0 * :Alice2
```

- [ ] Second USER receives `462 :You may not reregister`

### 4.14 PASS after registration

```
PASS test123
NICK alice
USER alice 0 * :Alice
PASS test123
```

- [ ] Receives `462 :You may not reregister`

---

## Section 5 — Channel Operations

### 5.1 JOIN a new channel

```
JOIN #newchannel
```

- [ ] Receives `:nick!user@host JOIN :#newchannel`
- [ ] Receives topic (331 No topic) or (332 Topic)
- [ ] Receives NAMES list (353) with `@nick` (operator prefix)
- [ ] Receives end of NAMES (366)
- [ ] Creator is operator (has `@` in NAMES list)

### 5.2 JOIN an existing channel

**Client 2** joins a channel that Client 1 already created:

```
JOIN #newchannel
```

- [ ] Client 2 receives JOIN confirmation
- [ ] Client 2 receives NAMES list showing both users
- [ ] Client 1 receives notification that Client 2 joined
- [ ] Client 2 does NOT have `@` (not operator)

### 5.3 JOIN a channel without # prefix

```
JOIN invalidname
```

- [ ] Receives `476 :Bad Channel Mask`

### 5.4 Double JOIN (same channel)

```
JOIN #test
JOIN #test
```

- [ ] Second JOIN is silently ignored (no error, no duplicate)

### 5.5 PART a channel

```
JOIN #test
PART #test
```

- [ ] Receives `:nick!user@host PART #test :Leaving`
- [ ] Other members receive the PART notification
- [ ] Client is removed from the channel

### 5.6 PART a channel you're not in

```
PART #nonexistent
```

- [ ] Receives `403 :No such channel`

### 5.7 PART with a reason

```
JOIN #test
PART #test :Going away
```

- [ ] Reason is included in the PART message

### 5.8 PART then rejoin

```
JOIN #test
PART #test
JOIN #test
```

- [ ] Rejoin works normally
- [ ] User gets operator again (if channel was empty)

---

## Section 6 — Messaging (PRIVMSG)

> **Halloy / clients without `:`:** Some clients send `PRIVMSG #channel hello` (no `:` before a single word). The server accepts the message as the second parameter — same effect as `PRIVMSG #channel :hello`.

### 6.1 Channel message

Both clients in `#test`. Client 1 sends:

```
PRIVMSG #test :Hello everyone!
```

- [ ] Client 2 receives: `:nick!user@host PRIVMSG #test :Hello everyone!`
- [ ] Client 1 does NOT receive their own message

### 6.2 Private message to a user

```
PRIVMSG bob :Hey Bob, private message!
```

- [ ] Bob receives: `:alice!alice@host PRIVMSG bob :Hey Bob, private message!`
- [ ] Only Bob receives it (no one else)

### 6.3 Message to non-existent user

```
PRIVMSG ghostuser :hello?
```

- [ ] Receives `401 ghostuser :No such nick/channel`

### 6.4 Message to non-existent channel

```
PRIVMSG #ghostchan :hello?
```

- [ ] Receives `403 #ghostchan :No such channel`

### 6.5 Message to channel you're not in

Client is NOT in `#private`:

```
PRIVMSG #private :I'm not in here
```

- [ ] Receives `404 #private :Cannot send to channel`

### 6.6 PRIVMSG with no target

```
PRIVMSG
```

- [ ] Receives `461 :Not enough parameters`

### 6.7 PRIVMSG with empty message

```
PRIVMSG bob :
```

- [ ] Receives `412 :No text to send`

### 6.8 PRIVMSG to yourself

```
PRIVMSG alice :talking to myself
```

- [ ] Message is delivered to yourself (this is valid IRC behavior)

---

## Section 7 — Channel Operator Commands (eval sheet)

> *"Check that a regular user does not have privileges to do channel operator actions.
> Then test with an operator."*

### Setup for operator tests

**Client 1 (operator):**
```
PASS test123
NICK op_user
USER op_user 0 * :Operator
JOIN #moderated
```

**Client 2 (regular):**
```
PASS test123
NICK reg_user
USER reg_user 0 * :Regular
JOIN #moderated
```

Client 1 is `@op_user` (operator). Client 2 is `reg_user` (regular).

### 7.1 Regular user cannot KICK

**Client 2 (regular):**
```
KICK #moderated op_user :haha
```

- [ ] Receives `482 :You're not channel operator`

### 7.2 Operator can KICK

**Client 1 (operator):**
```
KICK #moderated reg_user :Bye!
```

- [ ] Both clients receive: `:op_user!... KICK #moderated reg_user :Bye!`
- [ ] reg_user is removed from the channel

### 7.3 KICK user not in channel

**Client 1:**
```
KICK #moderated nobody :Bye
```

- [ ] Receives `401 nobody :No such nick/channel`

### 7.4 Regular user cannot set TOPIC (with +t)

**Client 1 sets +t:**
```
MODE #moderated +t
```

**Client 2 (re-joins and) tries:**
```
JOIN #moderated
TOPIC #moderated :New topic
```

- [ ] Receives `482 :You're not channel operator`

### 7.5 Operator can set TOPIC

**Client 1:**
```
TOPIC #moderated :Welcome to the moderated channel
```

- [ ] All members receive: `:op_user!... TOPIC #moderated :Welcome to the moderated channel`

### 7.6 Query TOPIC

```
TOPIC #moderated
```

- [ ] Receives `332 nick #moderated :Welcome to the moderated channel`

### 7.7 TOPIC without +t (anyone can set)

Create a NEW channel (no +t by default):
```
JOIN #open
TOPIC #open :Anyone can set this
```

**Client 2 also joins and tries:**
```
JOIN #open
TOPIC #open :I changed it!
```

- [ ] Both users can set topic (no +t means anyone can)

### 7.8 Regular user cannot set MODE

**Client 2:**
```
MODE #moderated +i
```

- [ ] Receives `482 :You're not channel operator`

### 7.9 Regular user cannot INVITE

**Client 1 sets +i:**
```
MODE #moderated +i
```

**Client 2 (if in channel) tries to invite someone:**
```
INVITE someone #moderated
```

- [ ] Receives `482 :You're not channel operator` (or `401` if user doesn't exist)

### 7.10 Operator can INVITE

**Client 1:**
```
INVITE reg_user #moderated
```

- [ ] op_user receives `341` (RPL_INVITING)
- [ ] reg_user receives `:op_user!... INVITE reg_user #moderated`
- [ ] reg_user can now `JOIN #moderated` even though it's +i

---

## Section 8 — Channel Modes

### 8.1 MODE +i (invite-only)

**Client 1 (operator in #secret):**
```
JOIN #secret
MODE #secret +i
```

**Client 2 tries to join:**
```
JOIN #secret
```

- [ ] Receives `473 :Cannot join channel (+i)`

**Client 1 invites Client 2:**
```
INVITE reg_user #secret
```

**Client 2 tries again:**
```
JOIN #secret
```

- [ ] Client 2 joins successfully after invite

### 8.2 MODE -i (remove invite-only)

**Client 1:**
```
MODE #secret -i
```

**Client 3 can now join:**
```
JOIN #secret
```

- [ ] Joins successfully without invite

### 8.3 MODE +t (topic protection)

```
MODE #test +t
```

- [ ] Only operators can change the topic
- [ ] Regular users receive `482` when trying to set topic

### 8.4 MODE -t (remove topic protection)

```
MODE #test -t
```

- [ ] Any member can now change the topic

### 8.5 MODE +k (channel password)

**Client 1:**
```
MODE #locked +k secretkey
```

**Client 2 tries without key:**
```
JOIN #locked
```

- [ ] Receives `475 :Cannot join channel (+k)`

**Client 2 with wrong key:**
```
JOIN #locked wrongkey
```

- [ ] Receives `475 :Cannot join channel (+k)`

**Client 2 with correct key:**
```
JOIN #locked secretkey
```

- [ ] Joins successfully

### 8.6 MODE -k (remove channel password)

**Client 1:**
```
MODE #locked -k
```

**Client 3 can now join without key:**
```
JOIN #locked
```

- [ ] Joins successfully without key

### 8.7 MODE +o (give operator)

**Client 1 (operator):**
```
MODE #test +o reg_user
```

- [ ] reg_user is now operator
- [ ] reg_user has `@` in NAMES list
- [ ] MODE change is broadcast to all channel members

### 8.8 MODE -o (remove operator)

**Client 1:**
```
MODE #test -o reg_user
```

- [ ] reg_user is no longer operator
- [ ] MODE change broadcast includes target nick: `MODE #test -o reg_user`
- [ ] reg_user can no longer use operator commands

### 8.9 MODE -o on yourself

```
MODE #test -o op_user
```

- [ ] You lose operator status
- [ ] Subsequent operator commands return `482`

### 8.10 MODE +l (user limit)

**Client 1:**
```
MODE #limited +l 2
```

Client 1 and Client 2 are in the channel (2 users). Client 3 tries:
```
JOIN #limited
```

- [ ] Receives `471 :Cannot join channel (+l)`

### 8.11 MODE -l (remove limit)

**Client 1:**
```
MODE #limited -l
```

**Client 3 can now join:**
```
JOIN #limited
```

- [ ] Joins successfully

### 8.12 MODE +l with invalid values

```
MODE #test +l 0
MODE #test +l -5
MODE #test +l abc
```

- [ ] Server returns an error for each (does NOT silently accept)

### 8.13 MODE +k without key parameter

```
MODE #test +k
```

- [ ] Receives `461 :Not enough parameters`

### 8.14 MODE +o on non-existent user

```
MODE #test +o ghostuser
```

- [ ] Receives `401 ghostuser :No such nick/channel`

### 8.15 Query channel modes

```
MODE #test
```

- [ ] Receives `324` with current mode string (e.g. `+it`, `+k <key>` when `+k` is set, `+l <number>` when `+l` is set — parameters must appear after the mode letters)

---

## Section 9 — QUIT and Disconnect

### 9.1 Clean QUIT

```
JOIN #test
QUIT :Goodbye!
```

- [ ] Connection is closed
- [ ] Other clients in `#test` receive quit notification
- [ ] Client is removed from all channels

### 9.2 QUIT without message

```
QUIT
```

- [ ] Connection is closed cleanly

### 9.3 Disconnect without QUIT (close nc)

Register and join a channel, then close `nc` with `Ctrl+C`.

- [ ] Server detects the disconnect
- [ ] Client is removed from all channels
- [ ] Channel is destroyed if it was the last member
- [ ] No memory leaks from the disconnected client

### 9.4 Commands after QUIT

```
QUIT
JOIN #afterquit
```

- [ ] Server ignores commands after QUIT (connection should be closed)

---

## Section 10 — PING/PONG

### 10.1 Server PING

```
PING :test123
```

- [ ] Receives `PONG :test123`
- [ ] Token is echoed back correctly

### 10.2 Client PONG

> **Note:** This server does **not** send periodic keepalive PINGs to clients (no idle timeout). You cannot test “reply to server PING” unless you add that feature later.  
> The **PONG** command is still accepted: if you send `PONG :anything` (e.g. from a client that echoes), the server handles it and the connection stays up.

**Optional manual check:** send `PONG :test` after registration.

```text
PONG :test
```

- [ ] Server does not disconnect (no error); connection stays alive

---

## Section 11 — Edge Cases and Robustness

### 11.1 Empty lines

Send several empty lines (just press Enter multiple times):

```
(empty)
(empty)
(empty)
PASS test123
NICK edgetest
USER edgetest 0 * :Edge
```

- [ ] Server ignores empty lines
- [ ] Registration still works after empty lines

### 11.2 Very long nickname (1000+ characters)

```
PASS test123
NICK AAAA....(1000 A's)....AAAA
```

- [ ] Server does NOT crash
- [ ] Server handles gracefully (accepts or rejects, but no segfault)

### 11.3 Very long message (50000+ characters)

```
PRIVMSG #test :XXXX....(50000 X's)....XXXX
```

- [ ] Server does NOT crash
- [ ] No segfault, no hang

### 11.4 Very long channel name

```
JOIN #ZZZZ....(10000 Z's)....ZZZZ
```

- [ ] Server does NOT crash

### 11.5 Binary/garbage data

Send random bytes to the server:

```bash
(printf "\xff\xfe\xfd\x00\x01"; sleep 1) | nc localhost 6667
```

- [ ] Server does NOT crash
- [ ] Other clients are unaffected

### 11.6 Null bytes in messages

```
PRIVMSG #test :\x00\x00\x00
```

- [ ] Server does NOT crash

### 11.7 Rapid connect/disconnect (50 clients)

```bash
for i in $(seq 1 50); do
  (printf "PASS test123\nNICK f$i\nUSER f 0 * :x\nQUIT\n") | nc localhost 6667 &
done
wait
```

- [ ] Server survives all 50 rapid connections
- [ ] Server is still responsive after the flood

### 11.8 Connect and immediately close (no data sent)

```bash
nc -w 0 localhost 6667 < /dev/null
```

- [ ] Server does NOT crash
- [ ] Server does NOT block

---

## Section 12 — Error Handling

### 12.1 Unknown command

```
PASS test123
NICK alice
USER alice 0 * :Alice
BLAHBLAH something
```

- [ ] Receives `421 :Unknown command`

### 12.2 Commands with no parameters

After registration, try each:

```
JOIN
PART
PRIVMSG
KICK
INVITE
TOPIC
MODE
```

- [ ] Each returns `461 :Not enough parameters`

### 12.3 KICK yourself

```
JOIN #test
KICK #test alice :self-kick
```

- [ ] Server handles gracefully (allowed in most IRC implementations)

### 12.4 INVITE to a channel you're not in

```
INVITE someone #randomchan
```

- [ ] Receives appropriate error (403 or 442)

### 12.5 MODE on a channel you're not in

First create a channel from another client, then:

```
MODE #otherchannel +i
```

- [ ] Receives `442 :You're not on that channel`

---

## Section 13 — Case Insensitivity (Halloy compatibility)

### 13.1 Nicknames are case-insensitive

**Client 1** registers as `Alice`. **Client 2** tries `ALICE`:

- [ ] `433 :Nickname is already in use`

### 13.2 Channel names are case-insensitive

**Client 1** joins `#General`. **Client 2** joins `#GENERAL`:

- [ ] Both are in the same channel
- [ ] Client 1 sees Client 2 join

### 13.3 Case is preserved for display

After joining with `#MyChannel`, check NAMES list:

- [ ] Channel name displays as originally created (e.g. `#MyChannel`)
- [ ] Nickname displays as originally set (e.g. `Alice` not `alice`)

---

## Quick Checklist Summary

### Must Pass (evaluation stops at first failure)

| # | Check | Status |
|---|-------|--------|
| 1 | Compiles with `-Wall -Wextra -Werror` | [ ] |
| 2 | Executable named `ircserv` | [ ] |
| 3 | `fcntl()` only used as `fcntl(fd, F_SETFL, O_NONBLOCK)` | [ ] |
| 4 | Single `poll()` in the code | [ ] |
| 5 | No segfault at any point | [ ] |
| 6 | No memory leaks | [ ] |

### Networking

| # | Check | Status |
|---|-------|--------|
| 7 | Connect with `nc` | [ ] |
| 8 | Connect with reference IRC client | [ ] |
| 9 | Multiple simultaneous connections | [ ] |
| 10 | Channel messages reach all members | [ ] |
| 11 | `nc` and IRC client work together | [ ] |
| 12 | Frozen client (Ctrl+Z) does not hang server | [ ] |
| 13 | Kill nc mid-command — server survives | [ ] |
| 14 | Unexpectedly kill client — server continues | [ ] |
| 15 | Partial commands handled correctly | [ ] |

### Commands

| # | Check | Status |
|---|-------|--------|
| 16 | PASS / NICK / USER registration | [ ] |
| 17 | PRIVMSG to channel | [ ] |
| 18 | PRIVMSG to user (DM) | [ ] |
| 19 | JOIN / PART | [ ] |
| 20 | TOPIC (get and set) | [ ] |
| 21 | KICK | [ ] |
| 22 | INVITE (with +i) | [ ] |
| 23 | MODE +i (invite-only) | [ ] |
| 24 | MODE +t (topic protection) | [ ] |
| 25 | MODE +k (channel password) | [ ] |
| 26 | MODE +o (operator) | [ ] |
| 27 | MODE +l (user limit) | [ ] |
| 28 | Regular user CANNOT use operator commands | [ ] |
| 29 | PING / PONG | [ ] |
| 30 | QUIT | [ ] |

### Robustness

| # | Check | Status |
|---|-------|--------|
| 31 | Empty lines do not crash server | [ ] |
| 32 | Long strings do not crash server | [ ] |
| 33 | Binary data does not crash server | [ ] |
| 34 | Rapid connections do not crash server | [ ] |
| 35 | Unknown commands return 421 | [ ] |
| 36 | Case-insensitive nicknames | [ ] |
| 37 | Case-insensitive channels | [ ] |

---

## Running Automated Tests

After manual verification, you can also run the automated scripts:

```bash
# From the project root, with server running:

# Multi-client interaction test (15 checks)
./tests/test_multi_clients.sh 6667 test123

# Stress test with 10 simultaneous clients
./tests/stress_test_10_clients.sh 6667 test123
```
