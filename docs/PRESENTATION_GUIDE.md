# 🎓 IRC Server - Presentation Guide

**For Defense/Evaluation**  
**Project:** ft_irc  
**Last Updated:** March 12, 2026

---

## 📋 Quick Reference Card

**Compile:** `make`  
**Run:** `./ircserv 6667 password`  
**Connect:** `nc localhost 6667` then `PASS password`, `NICK name`, `USER name 0 * :Real`

**Test Results:** ✅ 31/31 (100%)  
**Halloy Tests:** ✅ 4/4 (100%)  
**Multi-Client:** ✅ Verified  

---

## 🎯 5-Minute Demo Script

### 1. Compilation (30 seconds)
```bash
make clean
make
# Shows: Clean compilation, no warnings
```

### 2. Start Server (15 seconds)
```bash
./ircserv 6667 test123
# Shows: Server listening on port 6667
```

### 3. Basic Connection (1 minute)
```bash
# In new terminal
nc localhost 6667
PASS test123
NICK evaluator
USER eval 0 * :Evaluation User
```
**Expected:** Welcome messages 001-004

### 4. Channel Operations (1 minute)
```bash
JOIN #test
TOPIC #test :Demo channel
PRIVMSG #test :Hello!
```
**Expected:** JOIN notification, NAMES list, message sent

### 5. Modes (1 minute)
```bash
MODE #test +i
MODE #test +k secretkey
MODE #test
```
**Expected:** Mode changes broadcast, query shows current modes

### 6. Multiple Clients (1.5 minutes)
```bash
# Open 2nd terminal
nc localhost 6667
PASS test123
NICK alice
USER alice 0 * :Alice
JOIN #test
```
**Expected:** Both clients see each other, alice cannot join (+i mode)

---

## 💡 Key Concepts to Explain

### 1. Non-Blocking I/O

**Question:** "How does your server handle multiple clients?"

**Answer:**
- We use **non-blocking I/O** with `poll()`
- ALL sockets set to `O_NONBLOCK` using `fcntl()`
- One `poll()` call monitors all file descriptors
- Server responds to `POLLIN` (data ready), `POLLHUP` (disconnect)
- No threads, no fork - single process handles all clients

**Code to Show:** `src/Poller.cpp` - single `poll()` call

---

### 2. Message Buffering

**Question:** "How do you handle partial data?"

**Answer:**
- TCP is a **stream protocol** - data can arrive fragmented
- Each client has a `MessageBuffer` that accumulates data
- We search for complete messages (ending with `\r\n`)
- Incomplete data stays in buffer for next `recv()`
- This handles the subject's Ctrl+D test

**Example:**
```
Receive: "NICK tes"        → Buffer: "NICK tes"
Receive: "tuser\r\n"       → Extract: "NICK testuser\r\n"
Receive: "JOIN #te"        → Buffer: "JOIN #te"
Receive: "st\r\n"          → Extract: "JOIN #test\r\n"
```

**Code to Show:** `src/MessageBuffer.cpp` - `extractMessages()`

---

### 3. Registration State Machine

**Question:** "Explain the authentication flow"

**Answer:**
- **Strict order required:** PASS → NICK → USER (Halloy requirement)
- Client has 3 states tracked by `registrationStep_`:
  - Step 0: Need PASS
  - Step 1: Need NICK (after PASS)
  - Step 2: Need USER (after NICK)
  - Step 3: Registered (can use all commands)
- Only after all 3, welcome messages (001-004) sent
- Other commands return ERR_NOTREGISTERED (451) before registration

**Code to Show:** `src/Client.cpp` - registration logic

---

### 4. Channel Management

**Question:** "How do channels work?"

**Answer:**
- Channel created on first JOIN
- First user automatically becomes operator (@)
- Channel stores:
  - Members map (`clients_`)
  - Operators vector
  - Topic (with setter and timestamp)
  - Modes (i, t, k, l)
  - Invite list (for +i mode)
- Channel deleted when last member leaves
- All case-insensitive (stored lowercase, display preserved)

**Code to Show:** `src/Channel.cpp` - `addClient()`, `removeClient()`

---

### 5. Command Execution Flow

**Question:** "Walk me through what happens when a client sends a command"

**Answer:**
1. Client sends: `"PRIVMSG #test :Hello\r\n"`
2. **Poller** detects POLLIN event
3. **Server** calls `recv()` → gets partial or full data
4. **MessageBuffer** accumulates and extracts complete message
5. **Parser** parses into `Command` struct (command, params, trailing)
6. **CommandRegistry** routes to appropriate handler
7. **Command Handler** validates and executes
8. **Server** sends response via `sendToClient()`

**Code to Show:** `src/Server.cpp` - `handleClientInput()`

---

### 6. Operator Privileges

**Question:** "How do operator permissions work?"

**Answer:**
- First user to JOIN a channel = automatic operator
- Operators can:
  - KICK users from channel
  - INVITE users to +i (invite-only) channels
  - Change TOPIC in +t (topic-protected) channels
  - Use MODE to change channel settings
  - Give/remove operator status (MODE +o/-o)
- Commands check `channel->isOperator(&client)` before allowing

**Code to Show:** Any operator command (KICK, INVITE, MODE)

---

## ❓ Common Questions & Answers

### Technical Implementation

**Q: Why `poll()` and not `select()` or `epoll()`?**

A: We chose `poll()` because:
- More portable than `epoll` (Linux-only) or `kqueue` (BSD/macOS)
- No FD_SETSIZE limit like `select()`
- Simpler API for our use case
- Sufficient performance for project scope
- Subject allows poll(), select(), epoll(), or kqueue

---

**Q: How do you prevent memory leaks?**

A: Multiple strategies:
1. **Proper destructors** in all classes
2. **Disconnect cleanup** removes client from all channels
3. **Empty channel deletion** when last user leaves
4. **RAII principles** - resource cleanup in destructors
5. **Verified with** `leaks` (macOS) / `valgrind` (Linux)

**Code to Show:** `src/Server.cpp` - `disconnectClient()` (lines 323-333)

---

**Q: How do you handle case-insensitive nicknames?**

A: We store **two versions**:
- `nickname_` - lowercase for comparison/lookup
- `nicknameDisplay_` - original case for display

Example: User sets "BoB"
- Stored as `nickname_ = "bob"` (comparison)
- Stored as `nicknameDisplay_ = "BoB"` (display)
- Search for "bob", "BOB", or "Bob" all find same user
- Messages show `BoB!user@host` (original case)

**Code to Show:** `src/Client.cpp` - `setNickname()`

---

**Q: Explain your architecture (3 layers)**

A:
1. **Network Layer (Alex)**
   - Socket operations (bind, listen, accept)
   - `poll()` for I/O multiplexing
   - Non-blocking sockets
   - Message buffering

2. **Parser Layer (Artur)**
   - Parse IRC message format
   - Command registry/dispatch
   - Reply formatting (numeric codes)

3. **Logic Layer (Sara)**
   - Client state management
   - Channel state management
   - All 13 command implementations
   - Registration state machine

**Benefit:** Clear separation of concerns, easier to test/maintain

---

**Q: How does MODE command work?**

A: **Simplified implementation** - one mode at a time:
- Parse mode string (e.g., "+i", "+k password")
- Check if client is operator
- Apply mode to channel
- Broadcast change to all members

Supported: +i, +t, +k, +o, +l (and their negatives)

**Code to Show:** `src/commands/Mode.cpp`

---

### Halloy Compatibility

**Q: What is Halloy and why is it important?**

A: Halloy is a common IRC client used for testing. Has specific requirements:
1. **Strict order:** PASS → NICK → USER (we enforce this)
2. **Password retry:** Allow 3 attempts before disconnect (we do)
3. **Case handling:** Case-insensitive comparison, case-preserving display (we do)
4. **Empty parameters:** Handle correctly (we do)

All 4 requirements tested and passing ✅

---

**Q: Show me the password retry mechanism**

A:
```cpp
// In PASS command handler:
if (password != server.getPassword()) {
    client.incrementPasswordAttempts();
    if (client.hasExceededPasswordAttempts()) {
        // 3 attempts exceeded
        server.sendToClient(fd, "Password incorrect (max attempts)");
        server.disconnectClient(fd);
    } else {
        // Allow retry
        server.sendToClient(fd, "Password incorrect");
    }
}
```

**Code to Show:** `src/commands/Pass.cpp`

---

### Testing & Verification

**Q: How did you test the server?**

A: Comprehensive testing:
1. **Unit tests** - Each command individually
2. **Integration tests** - Full client flow
3. **Multi-client** - 3+ simultaneous users
4. **Stress tests** - Rapid connections
5. **Partial data** - Subject's Ctrl+D test
6. **Halloy tests** - All 4 requirements
7. **Memory tests** - leaks/valgrind

**Results:** 31/31 tests passed (100%)

**Document:** `docs/FINAL_TEST_REPORT.md`

---

**Q: Show me multiple clients working**

A: Demo with 3 clients:
1. Start server
2. Connect alice → creates #test, becomes @alice
3. Connect bob → joins #test, sees @alice bob
4. Connect charlie → joins #test, sees @alice bob charlie

Each client receives JOIN notifications when others join.

**Can demonstrate live**

---

## 🐛 Common Issues & Solutions

### Issue: Port already in use
**Solution:** Kill existing server or use different port
```bash
killall ircserv
# or
./ircserv 6668 password
```

### Issue: Clients don't see each other
**Solution:** Clients must stay connected (not disconnect immediately)
```bash
# Wrong - disconnects immediately:
echo "JOIN #test" | nc localhost 6667

# Right - stays connected:
nc localhost 6667
# then type commands interactively
```

### Issue: "ERR_NOTREGISTERED"
**Solution:** Must send PASS, NICK, USER in order before other commands

---

## 📊 Project Statistics

**Lines of Code:**
- Headers: ~1,500 lines
- Implementation: ~3,000 lines
- Total: ~4,500 lines (excluding docs)

**Files:**
- 23 source files (.cpp)
- 23 header files (.hpp)
- 13 command implementations

**Development Time:**
- Study phase: 2 weeks
- Implementation: 6 weeks
- Testing: 1 week
- **Total: ~9 weeks**

---

## 🎯 What Makes This Implementation Good

### 1. Clean Architecture
- Clear separation between layers
- Well-defined interfaces
- Easy to test and maintain

### 2. Standards Compliant
- C++98 (no C++11 features)
- IRC RFC 1459/2812
- No forbidden functions

### 3. Robust Error Handling
- Proper IRC error codes
- Network error handling
- State validation

### 4. Memory Safe
- No leaks (verified)
- Proper cleanup on disconnect
- RAII where appropriate

### 5. Well Documented
- Team conventions documented
- Code comments where needed
- Comprehensive test reports

---

## 💭 If Asked: "What Would You Improve?"

### Could Add (Optional):
1. **NOTICE command** - Like PRIVMSG but no error replies
2. **WHO/WHOIS commands** - User information queries
3. **Multiple modes per command** - `MODE #chan +it` (currently one at a time)
4. **Channel bans** - Ban users from channels
5. **TLS/SSL** - Encrypted connections

### Why Not Added:
- Not required by subject
- Would add significant complexity
- Current implementation is complete and functional

---

## 🔑 Key Phrases to Remember

- "Non-blocking I/O with a single `poll()` call"
- "Three-layer architecture for separation of concerns"
- "Strict PASS → NICK → USER order for Halloy compatibility"
- "Case-insensitive comparison, case-preserving display"
- "MessageBuffer handles partial data with CRLF framing"
- "31/31 tests passed, fully functional"

---

## ✅ Pre-Defense Checklist

**Before the defense:**
- [ ] Server compiles cleanly (`make`)
- [ ] Know where main `poll()` call is (`src/Poller.cpp`)
- [ ] Can explain registration flow
- [ ] Can explain disconnect cleanup (your code!)
- [ ] Can demonstrate multiple clients
- [ ] Can explain one operator concept
- [ ] Know test results (31/31, 100%)

**During defense:**
- [ ] Stay calm and confident
- [ ] Offer to show code for explanations
- [ ] If don't know something, say so honestly
- [ ] Explain design decisions
- [ ] Show the server working live

---

## 🎓 Final Tips

1. **Be Confident** - You tested everything, it works
2. **Show, Don't Just Tell** - Demo features live
3. **Know Your Code** - Especially your parts (Sara: Logic Layer)
4. **Explain Decisions** - Why you chose certain approaches
5. **Be Honest** - If unsure, say "let me check the code"
6. **Stay Professional** - Technical discussion, not defensive

---

## 📝 Your Contribution (Sara)

**What You Implemented:**
- Client.cpp (client state management)
- Channel.cpp (channel state management)
- All 13 command handlers
- Registration state machine
- Operator management logic
- Channel modes implementation
- **Disconnect cleanup** (final critical piece!)

**Lines:** ~2,000 lines of logic layer

**Impact:** Essential for server functionality

---

**You're Ready!** 🚀

Remember: You built a fully functional IRC server from scratch. That's impressive. Be proud of your work.

**Good luck with your defense!** 🎓

---

**END OF PRESENTATION GUIDE**
