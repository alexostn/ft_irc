#!/usr/bin/env bash

# Test PlayBot Commands
# Usage: bash bonus/test_playbot.sh
#
# Prerequisites:
#   Terminal 1: ./ircserv 6667 testpass
#   Terminal 2: ./playbot 127.0.0.1 6667 testpass playbot #general
#   Terminal 3: bash bonus/test_playbot.sh

HOST="127.0.0.1"
PORT=6667
PASS="testpass"

echo "[*] Testing PlayBot commands..." >&2
echo "[*] Make sure playbot is running in another terminal" >&2
echo "" >&2

{
  # Step 1: Register with server
  echo "PASS $PASS"
  echo "NICK testuser"
  echo "USER testuser 0 * :testuser"
  sleep 1

  # Step 2: Join channel
  echo "JOIN #general"
  sleep 0.5

  # Step 3: Send test commands
  echo "PRIVMSG #general :!ping"     ; echo "[TEST] Sent: !ping"     >&2 ; sleep 0.5
  echo "PRIVMSG #general :!help"     ; echo "[TEST] Sent: !help"     >&2 ; sleep 0.5
  echo "PRIVMSG #general :!uptime"   ; echo "[TEST] Sent: !uptime"   >&2 ; sleep 0.5
  echo "PRIVMSG #general :!time"     ; echo "[TEST] Sent: !time"     >&2 ; sleep 0.5
  echo "PRIVMSG playbot :!echo Hello from PM" ; echo "[TEST] Sent: !echo Hello from PM (PM)" >&2 ; sleep 2

} | nc -C "$HOST" "$PORT"

echo "" >&2
echo "[*] Test completed. Check playbot output above for responses." >&2
