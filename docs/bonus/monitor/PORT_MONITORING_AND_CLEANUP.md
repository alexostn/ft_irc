# Port Monitoring and Cleanup Guide

## Problem: "Address already in use"

When starting the IRC server, you may see:
```
[Server] Failed to create server socket: Address already in use
```

**Root cause:** Port 6667 (or specified port) is held by a previous process that didn't fully terminate.

---

## Why This Happens

### TCP TIME_WAIT State

When a server process exits, the OS doesn't immediately release the port. Instead, it enters **TIME_WAIT** state for 60-120 seconds to ensure no late packets arrive from old connections.

```
Process terminates → TCP closes connection → TIME_WAIT state (60-120s) → Port released
```

**During TIME_WAIT:** The port appears "in use" even though no process is bound to it.

### Our Solution: SO_REUSEADDR

In [Server.cpp](../../src/Server.cpp) (lines 195-196), we set:

```cpp
int opt = 1;
setsockopt(serverSocketFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

**What SO_REUSEADDR does:**
- Allows binding a socket to a port in TIME_WAIT state
- Must be set **BEFORE `bind()`** call
- Prevents "address already in use" errors on rapid restart

**Why it's safe:**
- Only works if no other process actively holds the port
- Prevents full port conflicts but allows graceful restart

---

## Debugging: Port Is Still Stuck

If SO_REUSEADDR is set but you still see "address already in use", a **background process** is actively holding the port.

### Step 1: Find Which Process Holds Port 6667

```bash
# Method 1: lsof (recommended)
lsof -i :6667

# Output example:
# COMMAND   PID     USER   FD   TYPE             DEVICE SIZE/OFF NODE NAME
# ircserv   1234    user    3u  IPv6 0x1234567890123456      0t0  TCP *:6667 (LISTEN)

# Method 2: ss (systemd-style)
ss -tlnp | grep 6667

# Method 3: netstat (legacy)
netstat -tlnp | grep 6667
```

### Step 2: Kill the Zombie Process

```bash
# Kill by name (all instances)
pkill -f "ircserv 6667"

# Kill by specific PID
kill <PID>

# Force kill if regular kill doesn't work
kill -9 <PID>
```

### Step 3: Wait for TIME_WAIT (if needed)

```bash
# If port still shows in TIME_WAIT, wait a bit:
sleep 2

# Or check port state:
ss -tlnp | grep 6667

# State should show "TIME-WAIT" (no LISTEN)
# After 60s it will be fully released
```

### Step 4: Restart Server

```bash
./ircserv 6667 testpass
```

---

## Complete Setup Script

For development, use this script to ensure clean restart:

```bash
#!/bin/bash
# kill_and_restart.sh

PORT=6667

# Step 1: Kill any existing ircserv
echo "[*] Killing existing ircserv processes on port $PORT..."
pkill -f "ircserv $PORT" 2>/dev/null || true

# Step 2: Wait for TIME_WAIT to clear
echo "[*] Waiting for TIME_WAIT state to clear..."
sleep 2

# Step 3: Verify port is free
if lsof -i :$PORT > /dev/null 2>&1; then
    echo "[!] WARNING: Port $PORT still in use after cleanup!"
    lsof -i :$PORT
    exit 1
fi

# Step 4: Build and run
echo "[*] Rebuilding server..."
make clean && make

echo "[*] Starting server on port $PORT..."
./ircserv $PORT testpass
```

**Usage:**
```bash
chmod +x bonus/monitor/kill_and_restart.sh
bonus/monitor/kill_and_restart.sh
```

---

## Monitoring Port Health

### Check Port Status Continuously

```bash
#!/bin/bash
# monitor_port.sh - Watch port status every 2 seconds

PORT=6667

while true; do
    clear
    echo "=== Port $PORT Monitor ==="
    echo "Time: $(date)"
    echo ""
    
    if ss -tlnp 2>/dev/null | grep -q ":$PORT"; then
        echo "✓ Port $PORT is LISTENING"
        ss -tlnp | grep ":$PORT"
    else
        if lsof -i :$PORT 2>/dev/null | grep -q TIME_WAIT; then
            echo " Port $PORT is in TIME_WAIT state"
            lsof -i :$PORT
        else
            echo "✗ Port $PORT is FREE"
        fi
    fi
    
    sleep 2
done
```

**Usage:**
```bash
chmod +x bonus/monitor/monitor_port.sh
bonus/monitor/monitor_port.sh
```

---

## Socket Configuration Details

### Why SO_REUSEADDR Works

**Before SO_REUSEADDR:**
```
Kernel: "Port 6667 is in TIME_WAIT, REJECT new bind()"
```

**After SO_REUSEADDR:**
```
Kernel: "Port 6667 is in TIME_WAIT, but SO_REUSEADDR is set
         and no other process owns it → ALLOW bind()"
```

### Our Implementation (Verified ✓)

```cpp
// Server.cpp, socket initialization
void Server::createSocket() {
    serverSocketFd_ = socket(AF_INET6, SOCK_STREAM, 0);
    
    // ... IPv6 setup ...
    
    // ✓ SO_REUSEADDR is set BEFORE bind()
    int opt = 1;
    setsockopt(serverSocketFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

void Server::bindSocket() {
    // ✓ bind() called AFTER SO_REUSEADDR
    bind(serverSocketFd_, (struct sockaddr*)&addr, sizeof(addr));
}
```

**Order matters:**
1. `socket()` — create socket descriptor
2. `setsockopt(...SO_REUSEADDR...)` — set permission
3. `bind()` — bind to port ✓

---

## Troubleshooting Reference

| Issue | Root Cause | Solution |
|-------|-----------|----------|
| "Address already in use" on restart | Old process still running | `pkill -f "ircserv 6667"` |
| Port shows LISTEN but server won't start | Another app using port | `lsof -i :6667` to identify |
| SO_REUSEADDR seemed to not work | Socket not created yet | Verify SO_REUSEADDR set BEFORE bind() |
| Port stuck even after 60s | IPv4 mapped IPv6 socket | Check dual-stack config (IPv6_V6ONLY=0) |

---

## Testing Port Cleanup

```bash
# Terminal 1: Start and immediately stop server
./ircserv 6667 testpass
# Ctrl+C to stop

# Terminal 2: Check port status
watch -n 1 "lsof -i :6667 || echo 'Port free'"

# Terminal 3: Restart server immediately
./ircserv 6667 testpass
```

If SO_REUSEADDR is working, **restart succeeds immediately** without waiting 60s.

---

## Production Notes

**For deployment:**
- SO_REUSEADDR is **safe** for production
- It prevents false "address in use" errors on legitimate restarts
- Does not compromise security (only if no active process holds port)
- Consider adding graceful shutdown handler:

```cpp
// Catch SIGTERM to cleanup properly
signal(SIGTERM, [](int) {
    g_server->stop();  // Clean shutdown
    exit(0);           // Let destructors run
});
```

**See also:** [Signal Handling Documentation](../network/signals/)

---

## Tools Used

- **lsof:** List open files/ports (most portable)
- **ss:** Modern socket utility (Linux)
- **netstat:** Legacy network utility
- **pkill:** Kill process by name pattern

