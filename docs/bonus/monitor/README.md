# Socket Monitoring & Cleanup Documentation

This directory contains tools and guides for monitoring and troubleshooting TCP socket issues, particularly port binding and resource cleanup.

## Files

### [PORT_MONITORING_AND_CLEANUP.md](PORT_MONITORING_AND_CLEANUP.md)

Complete guide covering:
- **Why ports get stuck** (TIME_WAIT state explanation)
- **How SO_REUSEADDR works** (and why we use it)
- **Debug techniques** (lsof, ss, netstat)
- **Quick cleanup scripts** (kill zombie processes)
- **Socket configuration details** (order of operations)
- **Production best practices**

### Quick Commands

```bash
# Find what's using port 6667
lsof -i :6667

# Kill the process
pkill -f "ircserv 6667"

# Wait for TIME_WAIT to clear
sleep 2

# Restart server
./ircserv 6667 testpass
```

## Our Approach: SO_REUSEADDR ✓

In [src/Server.cpp](../../src/Server.cpp) (lines 195-196):

```cpp
int opt = 1;
setsockopt(serverSocketFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

**Why it works:**
- Set **BEFORE** `bind()` call ✓
- Allows immediate restart on same port
- Safe: only if no active process owns port

## Common Errors

```
[Server] Failed to create server socket: Address already in use
```

**Solution:**
```bash
pkill -f "ircserv 6667" && sleep 1 && ./ircserv 6667 testpass
```

See [PORT_MONITORING_AND_CLEANUP.md](PORT_MONITORING_AND_CLEANUP.md) for detailed explanations.

