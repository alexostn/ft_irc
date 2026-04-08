#!/usr/bin/env bash
# flood_channel.sh — wrapper for the canonical flood test
# Usage: bash bonus/flood_channel.sh
# This script simply delegates to tests/flood_channel.sh to avoid duplication.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]:-$0}")" && pwd)"
CANONICAL_SCRIPT="$SCRIPT_DIR/../tests/flood_channel.sh"

if [ ! -f "$CANONICAL_SCRIPT" ]; then
  echo "Canonical flood script not found: $CANONICAL_SCRIPT" >&2
  exit 1
fi

if [ ! -x "$CANONICAL_SCRIPT" ]; then
  # Fallback: try to run via bash if not executable
  exec bash "$CANONICAL_SCRIPT" "$@"
else
  exec "$CANONICAL_SCRIPT" "$@"
fi

# The original implementation of the flood test lives in tests/flood_channel.sh.
# Keep that file as the single source of truth; update this wrapper only if the
# canonical script's location changes.

# Example usage:
#   Terminal 1: ./ircserv 6667 testpass
#   Terminal 2: nc -C 127.0.0.1 6667
#                PASS testpass
#                NICK clientA
#                USER clientA 0 * :Client A
#                JOIN #test
#                Ctrl+Z  ←── freeze
#                [1]+ Stopped nc...
#                $ bash bonus/flood_channel.sh   # (messages are sent)
#                fg  ←── resume nc; messages arrive

# Ctrl+Z and fg always go together in a single terminal.
# fg restores the exact process that was suspended in that same shell.
