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