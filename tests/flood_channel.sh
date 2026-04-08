#!/usr/bin/env bash
# flood_channel.sh — persistent single-connection flood test
# Usage: bash tests/flood_channel.sh
# Requires: server already running on PORT with PASS

HOST="127.0.0.1"
PORT=6667
PASS="testpass"
NICK="floodbot"
CHAN="#test"
COUNT=200

{
  echo "PASS $PASS"
  echo "NICK $NICK"
  echo "USER $NICK 0 * :Flood Bot"
  sleep 1        # wait for server welcome (001-004)
  echo "JOIN $CHAN"
  sleep 0.3      # wait for JOIN ack
  for i in $(seq 1 $COUNT); do
    echo "PRIVMSG $CHAN :msg $i"
  done
  sleep 1        # let last writes drain before EOF
} | nc -C "$HOST" "$PORT"

# Terminal 1          Terminal 2 (clientA)        Terminal 3 (flood)
# ──────────          ────────────────────        ──────────────────
# ./ircserv           nc -C 127.0.0.1 6667
# 6667 testpass       PASS testpass
#                     NICK clientA
#                     USER clientA 0 * :Client A
#                     JOIN #test
                    
#                     Ctrl+Z  ←── freeze
#                     [1]+ Stopped nc...
#                     $                           bash tests/flood_channel.sh
#                                                 (200 messages are sent)
                    
#                     fg  ←── enter HERE
#                     (all 200 messages are sent)

# Ctrl+Z and fg always go together in a single terminal
# fg restores the exact process that was suspended in that same shell
