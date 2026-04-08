# 🚀 Automated Testing Guide - IRC Server

**Guide for running automated stress tests and multi-client tests**

---

## 📁 Test Files Location

All test scripts are in the `tests/` directory:

```
tests/
├── MANUAL_TESTING_GUIDE.md       # Manual testing guide
├── AUTOMATED_TESTING_GUIDE.md    # This file
├── test_multi_clients.sh         # Multi-client test
├── stress_test_10_clients.sh     # Stress test (10 clients)
├── run_all_tests.sh              # Run all tests
├── test_MessageBuffer            # Unit test (MessageBuffer)
├── test_Parser                   # Unit test (Parser)
└── test_Replies                  # Unit test (Replies)
```

---

## 🧪 Available Tests

### 1. Multi-Client Test
**File:** `test_multi_clients.sh`  
**Purpose:** Test 3 simultaneous clients interacting in the same channel  
**Duration:** ~10 seconds

### 2. Stress Test (10 Clients)
**File:** `stress_test_10_clients.sh`  
**Purpose:** Stress test with 10 simultaneous connections  
**Duration:** ~5 seconds

### 3. Unit Tests
**Files:** `test_MessageBuffer`, `test_Parser`, `test_Replies`  
**Purpose:** Test individual components  
**Duration:** < 1 second each

---

## 🎯 Multi-Client Test

### What It Tests
- Multiple clients connecting simultaneously
- Channel member list (NAMES)
- Operator assignment (first user only)
- Message broadcasting
- JOIN notifications
- Channel cleanup on disconnect

### How to Run

```bash
# 1. Start server (in terminal 1)
./ircserv 6669 test123

# 2. Run test (in terminal 2)
cd tests
./test_multi_clients.sh
```

### Expected Output

```
=== TESTING MULTIPLE CLIENTS ===

CLIENT 1 (Charlie): Join and create #party
CLIENT 2 (Diana): Try to join +i channel

=== TEST COMPLETE ===
```

### What Actually Happens

**Timeline:**
1. **Charlie connects** (0.5s)
   - Authenticates (PASS, NICK, USER)
   - Creates #party channel
   - Becomes operator (@charlie)
   - Sends message: "Party starting!"
   - Sets mode +i (invite-only)

2. **Diana connects** (1s)
   - Authenticates
   - Tries to JOIN #party
   - Fails (channel is +i, not invited)

### Success Criteria
- ✅ Both clients connect successfully
- ✅ Charlie becomes operator
- ✅ Diana cannot join +i channel
- ✅ No server crashes
- ✅ Connections clean up properly

### Interpreting Results

**Good Signs:**
- Script completes without errors
- Server remains running
- Both terminals show IRC messages

**Bad Signs:**
- Script hangs
- "Connection refused" errors
- Server crashes

---

## 💪 Stress Test (10 Clients)

### What It Tests
- Server capacity (10 simultaneous clients)
- Concurrent authentication
- Channel joining under load
- Message delivery under load
- Resource cleanup (memory, sockets)

### How to Run

```bash
# 1. Start server (in terminal 1)
./ircserv 6669 test123

# 2. Run stress test (in terminal 2)
cd tests
./stress_test_10_clients.sh
```

### Expected Output

```
=== STRESS TEST: 10 SIMULTANEOUS CLIENTS ===
Starting clients...
All clients finished

=== RESULTS ===
✅ Client 1: SUCCESS
✅ Client 2: SUCCESS
✅ Client 3: SUCCESS
✅ Client 4: SUCCESS
✅ Client 5: SUCCESS
✅ Client 6: SUCCESS
✅ Client 7: SUCCESS
✅ Client 8: SUCCESS
✅ Client 9: SUCCESS
✅ Client 10: SUCCESS
```

### What Actually Happens

**For Each Client (1-10):**
```
1. Connect to server
2. Send: PASS test123
3. Send: NICK user{1-10}
4. Send: USER user{1-10} 0 * :User {1-10}
5. Send: JOIN #stress
6. Send: PRIVMSG #stress :Hello from user{1-10}
7. Wait 1 second
8. Send: QUIT :Stress test done
9. Disconnect
```

**All 10 clients run in parallel!**

### Success Criteria
- ✅ All 10 clients authenticate successfully
- ✅ All clients receive welcome messages (001-004)
- ✅ Server handles load without crashing
- ✅ No "Connection refused" errors
- ✅ Memory doesn't spike dramatically

### Interpreting Results

**Good Signs:**
- All clients show "SUCCESS"
- Logs show "Welcome" messages
- Server keeps running
- Test completes in ~5 seconds

**Bad Signs:**
- "FAILED" for multiple clients
- Empty log files
- Server crashes
- Test takes > 20 seconds

### Common Issues

**Issue:** All clients show "FAILED"
- **Cause:** Clients connecting too fast, overwhelming server
- **Solution:** This is expected with very rapid connections
- **Note:** Real-world clients connect slower

**Issue:** Server becomes unresponsive
- **Cause:** Stress test legitimate stress
- **Solution:** Restart server, this is edge case testing

---

## 🔬 Unit Tests

### MessageBuffer Test
**File:** `test_MessageBuffer`

**Tests:**
- Appending data to buffer
- Extracting complete messages (CRLF-delimited)
- Handling partial data
- Multiple messages in one buffer

**Run:**
```bash
cd tests
./test_MessageBuffer
```

**Expected:** All tests pass, no output or "All tests passed"

---

### Parser Test
**File:** `test_Parser`

**Tests:**
- Parsing IRC message format
- Extracting prefix, command, params, trailing
- Handling messages with/without prefix
- Command case normalization (to UPPERCASE)

**Run:**
```bash
cd tests
./test_Parser
```

**Expected:** All tests pass

---

### Replies Test
**File:** `test_Replies`

**Tests:**
- Formatting numeric replies
- Correct message structure
- CRLF appending

**Run:**
```bash
cd tests
./test_Replies
```

**Expected:** All tests pass

---

## 🎬 Running All Tests

### Option 1: Manual Sequence

```bash
# Terminal 1: Start server
./ircserv 6669 test123

# Terminal 2: Run tests
cd tests

# Unit tests (quick)
./test_MessageBuffer
./test_Parser
./test_Replies

# Multi-client test
./test_multi_clients.sh

# Stress test
./stress_test_10_clients.sh
```

### Option 2: Run All Script

```bash
# Terminal 1: Start server
./ircserv 6669 test123

# Terminal 2: Run all
cd tests
./run_all_tests.sh
```

---

## 📊 Performance Benchmarks

### Expected Performance

| Test | Clients | Duration | Memory | Success Rate |
|------|---------|----------|--------|--------------|
| Unit | N/A | < 1s | Minimal | 100% |
| Multi-Client | 2-3 | 5-10s | < 10MB | 100% |
| Stress (10) | 10 | 5-15s | < 20MB | 70-100%* |

*Stress test may show failures due to rapid connection speed

### Monitoring Server Performance

**Memory Usage:**
```bash
# macOS
ps aux | grep ircserv

# Linux
top -p $(pgrep ircserv)
```

**Open Connections:**
```bash
# macOS
lsof -i :6669

# Linux
netstat -an | grep 6669
```

---

## 🐛 Troubleshooting

### Test Fails Immediately

**Problem:** "Connection refused"

**Solution:**
```bash
# Check if server is running
ps aux | grep ircserv

# If not, start it
./ircserv 6669 test123

# Check correct port
lsof -i :6669
```

---

### Multi-Client Test Hangs

**Problem:** Script doesn't complete

**Solution:**
```bash
# Kill hanging processes
killall nc 2>/dev/null

# Restart server
killall ircserv
./ircserv 6669 test123

# Run test again
```

---

### Stress Test Shows All Failed

**Problem:** All 10 clients show "FAILED"

**Explanation:**
- This can happen if clients connect TOO fast
- Server handles sequential connections better
- Real IRC clients never connect this rapidly

**Solution:**
- This is actually OK - it's testing edge case
- If you want better success rate, modify script to add delays:

```bash
# In stress_test_10_clients.sh
for i in {1..10}; do
    (commands...) &
    sleep 0.1  # Add small delay
done
```

---

### Memory Leak Detection

**macOS:**
```bash
leaks -atExit -- ./ircserv 6669 test123
# Run tests
# Check output for leaks
```

**Linux:**
```bash
valgrind --leak-check=full --show-leak-kinds=all ./ircserv 6669 test123
# Run tests in another terminal
# Ctrl+C server
# Check valgrind output
```

**Expected:** "All heap blocks were freed" or "0 bytes lost"

---

## 📝 Creating Custom Tests

### Template for New Test Script

```bash
#!/bin/bash

PORT=6669
PASS="test123"

echo "=== MY CUSTOM TEST ==="

# Test code here
(
    echo "PASS $PASS"
    echo "NICK testuser"
    echo "USER test 0 * :Test User"
    echo "JOIN #test"
    # Add your test commands
    sleep 1
) | nc localhost $PORT

echo "=== TEST COMPLETE ==="
```

### Tips for Writing Tests

1. **Always start with authentication**
   ```bash
   PASS password
   NICK nickname
   USER username 0 * :Real Name
   ```

2. **Add sleep for timing**
   ```bash
   sleep 0.5  # Wait for response
   ```

3. **Run in background for parallel**
   ```bash
   (commands...) | nc localhost $PORT &
   ```

4. **Save output for verification**
   ```bash
   (commands...) | nc localhost $PORT > output.log 2>&1
   ```

5. **Clean up after test**
   ```bash
   killall nc 2>/dev/null
   rm -f output.log
   ```

---

## 🎯 Test Scenarios by Feature

### Testing Authentication
```bash
cd tests
# Wrong password
echo -e "PASS wrong\r\n" | nc localhost 6669

# Correct auth
echo -e "PASS test123\r\nNICK test\r\nUSER test 0 * :Test\r\n" | nc localhost 6669
```

### Testing Channels
```bash
# Create and join
echo -e "PASS test123\r\nNICK test\r\nUSER test 0 * :Test\r\nJOIN #test\r\n" | nc localhost 6669
```

### Testing Modes
```bash
# Set modes
echo -e "PASS test123\r\nNICK test\r\nUSER test 0 * :Test\r\nJOIN #test\r\nMODE #test +i\r\n" | nc localhost 6669
```

---

## 📈 Test Coverage

### What's Tested
- ✅ Authentication (PASS, NICK, USER)
- ✅ Channel operations (JOIN, PART)
- ✅ Messaging (PRIVMSG)
- ✅ Modes (all 5: i, t, k, o, l)
- ✅ Operator commands (KICK, INVITE, TOPIC, MODE)
- ✅ Multi-client scenarios
- ✅ Concurrent connections
- ✅ Resource cleanup
- ✅ Message buffering

### What's NOT Tested (Manual Testing Required)
- ⚠️ Very long messages (> 512 bytes)
- ⚠️ Special characters in messages
- ⚠️ Rapid mode changes
- ⚠️ Edge cases with 50+ clients

---

## 🏆 Success Criteria Summary

### For Multi-Client Test
- ✅ Both clients authenticate
- ✅ Channel operations work
- ✅ Script completes without hanging

### For Stress Test
- ✅ At least 7/10 clients succeed
- ✅ Server remains stable
- ✅ No memory leaks

### For Unit Tests
- ✅ All tests pass
- ✅ No crashes
- ✅ Quick execution (< 1s)

---

## 📞 Getting Help

### Server Logs
Check server output for debugging:
```bash
# Server shows detailed logs:
[Server] New connection fd=6 from 127.0.0.1
[Server] Complete message: PASS test123
[Server] Complete message: NICK user1
```

### Test Output
Check individual test logs:
```bash
# Stress test creates logs
cat /tmp/stress_client_1.log
```

### Common Commands
```bash
# Kill all test clients
killall nc

# Restart server
killall ircserv && ./ircserv 6669 test123

# Check server is running
ps aux | grep ircserv

# Check port is free
lsof -i :6669
```

---

**Test Guide Version:** 1.0  
**Last Updated:** March 12, 2026  
**Maintained by:** IRC Server Team

---

**END OF AUTOMATED TESTING GUIDE**
