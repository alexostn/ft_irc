#!/usr/bin/env bash
HOST="127.0.0.1"; PORT=6667
PASS="testpass"; NICK="bottester"; CHAN="#test"

{
  echo "PASS $PASS"; echo "NICK $NICK"
  echo "USER $NICK 0 * :Bot Tester"
  sleep 1
  echo "JOIN $CHAN"
  sleep 0.5
  echo "PRIVMSG $CHAN :!ping"
  sleep 0.5
  echo "PRIVMSG $CHAN :!uptime"
  sleep 0.5
  echo "PRIVMSG $CHAN :!echo HELLO EVALUATOR:)"
  sleep 2
} | nc -C "$HOST" "$PORT"
# nc prints everything the server sends to bot_tester
# bot replies arrive here because bot_tester is also in #test

# The key point is that the final sleep must be long enough for the bot to receive the command, 
# process it, and send a response back through the server