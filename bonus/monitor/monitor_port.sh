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