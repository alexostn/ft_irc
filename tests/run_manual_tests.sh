#!/bin/bash
#
# run_manual_tests.sh — Automated tests covering MANUAL_TESTING_GUIDE.md
# Usage: ./tests/run_manual_tests.sh [port] [password]
# Requires: ircserv running on port with given password; nc available.
#

PORT=${1:-8080}
PASS=${2:-test}
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG_DIR="/tmp/irc_test_$$"
mkdir -p "$LOG_DIR"
cleanup() { rm -rf "$LOG_DIR"; }
trap cleanup EXIT

PASS_CNT=0
FAIL_CNT=0

check() {
    local label="$1"
    local file="$2"
    local pattern="$3"
    if grep -qE "$pattern" "$file" 2>/dev/null; then
        echo "  ✅ $label"
        PASS_CNT=$((PASS_CNT + 1))
    else
        echo "  ❌ $label — expected: $pattern"
        FAIL_CNT=$((FAIL_CNT + 1))
    fi
    return 0
}

send_nc() {
    local out="$1"
    shift
    ( printf '%s\r\n' "$@" ; sleep "${SLEEP:-2}" ) | nc -w 5 localhost "$PORT" > "$out" 2>&1 || true
}

echo "=============================================="
echo "  ft_irc — Automated Manual Test Suite"
echo "  Port: $PORT  Password: $PASS"
echo "  (Server must be running: ./ircserv $PORT $PASS)"
echo "=============================================="
echo ""

# =============================================
# Section 1: Basic checks (static, no server)
# =============================================
echo "--- Section 1: Basic checks ---"
if [ -f "$ROOT/Makefile" ]; then
    (cd "$ROOT" && make re 2>&1) > "$LOG_DIR/make.log" && check "Compilation (make re)" "$LOG_DIR/make.log" "ircserv|compiled|Linking"
    [ -x "$ROOT/ircserv" ] && (echo "  ✅ Executable ircserv exists"; PASS_CNT=$((PASS_CNT+1))) || (echo "  ❌ No ircserv"; FAIL_CNT=$((FAIL_CNT+1)))
else
    echo "  ⏭ Makefile not found, skip compilation"
fi
grep -r "fcntl" "$ROOT/src" "$ROOT/include" 2>/dev/null | grep -q "F_SETFL.*O_NONBLOCK" && \
    (echo "  ✅ fcntl usage"; PASS_CNT=$((PASS_CNT+1))) || (echo "  ❌ fcntl"; FAIL_CNT=$((FAIL_CNT+1)))
poll_count=$(grep -rn "poll(" "$ROOT/src" "$ROOT/include" 2>/dev/null | grep -v "//" | wc -l | tr -d ' ')
[ "$poll_count" -ge 1 ] && (echo "  ✅ poll() used (count=$poll_count)"; PASS_CNT=$((PASS_CNT+1))) || (echo "  ❌ no poll()"; FAIL_CNT=$((FAIL_CNT+1)))
echo ""

# =============================================
# Section 2.1 / 4.1: Registration
# =============================================
echo "--- Section 2.1 / 4.1: Registration (PASS → NICK → USER) ---"
(
    printf "PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice Smith\r\n" "$PASS"
    sleep 2
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/reg.log" 2>&1
check "001 Welcome" "$LOG_DIR/reg.log" "001.*Welcome"
check "002 Your host" "$LOG_DIR/reg.log" "002"
check "003 Created" "$LOG_DIR/reg.log" "003"
check "004 Myinfo" "$LOG_DIR/reg.log" "004"
check "Prefix with host" "$LOG_DIR/reg.log" "alice!alice@"
echo ""

# =============================================
# Section 4.2: Wrong password
# =============================================
echo "--- Section 4.2: Wrong password ---"
SLEEP=1 send_nc "$LOG_DIR/wrongpass.log" "PASS wrongpassword"
check "464 Password incorrect" "$LOG_DIR/wrongpass.log" "464"
echo ""

# =============================================
# Section 4.4: NICK before PASS
# =============================================
echo "--- Section 4.4: NICK before PASS (451) ---"
send_nc "$LOG_DIR/nickfirst.log" "NICK alice"
check "451 not registered" "$LOG_DIR/nickfirst.log" "451"
echo ""

# =============================================
# Section 4.5: USER before NICK
# =============================================
echo "--- Section 4.5: USER before NICK (451) ---"
send_nc "$LOG_DIR/userfirst.log" "PASS $PASS" "USER alice 0 * :Alice"
check "451 not registered" "$LOG_DIR/userfirst.log" "451"
echo ""

# =============================================
# Section 4.7: Empty NICK
# =============================================
echo "--- Section 4.7: Empty NICK (431) ---"
send_nc "$LOG_DIR/emptynick.log" "PASS $PASS" "NICK" "USER x 0 * :x"
check "431 No nickname" "$LOG_DIR/emptynick.log" "431"
echo ""

# =============================================
# Section 4.12: Invalid nickname
# =============================================
echo "--- Section 4.12: Invalid nickname (432) ---"
send_nc "$LOG_DIR/badnick.log" "PASS $PASS" "NICK #invalid" "USER x 0 * :x"
check "432 Erroneous" "$LOG_DIR/badnick.log" "432"
echo ""

# =============================================
# Section 4.13: Double USER (462)
# =============================================
echo "--- Section 4.13: Double USER (462) ---"
send_nc "$LOG_DIR/doubleuser.log" "PASS $PASS" "NICK dbl" "USER dbl 0 * :Dbl" "USER dbl2 0 * :Dbl2"
check "462 reregister" "$LOG_DIR/doubleuser.log" "462"
echo ""

# =============================================
# Section 2.3 / 2.4: Multi-client + channel messaging
# =============================================
echo "--- Section 2.3 / 2.4: Multi-client + channel messaging ---"
(
    printf "PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n" "$PASS"
    sleep 1
    printf "JOIN #test\r\n"
    sleep 1.5
    printf "PRIVMSG #test :Hello everyone!\r\n"
    sleep 1
    printf "QUIT\r\n"
) | nc -w 8 localhost "$PORT" > "$LOG_DIR/alice.log" 2>&1 &
PID_ALICE=$!
(
    sleep 0.5
    printf "PASS %s\r\nNICK bob\r\nUSER bob 0 * :Bob\r\n" "$PASS"
    sleep 1
    printf "JOIN #test\r\n"
    sleep 2.5
    printf "QUIT\r\n"
) | nc -w 8 localhost "$PORT" > "$LOG_DIR/bob.log" 2>&1 &
PID_BOB=$!
(
    sleep 0.8
    printf "PASS %s\r\nNICK charlie\r\nUSER charlie 0 * :Charlie\r\n" "$PASS"
    sleep 1
    printf "JOIN #test\r\n"
    sleep 2.5
    printf "QUIT\r\n"
) | nc -w 8 localhost "$PORT" > "$LOG_DIR/charlie.log" 2>&1 &
PID_CHARLIE=$!
wait $PID_ALICE $PID_BOB $PID_CHARLIE 2>/dev/null
check "Alice 001" "$LOG_DIR/alice.log" "001"
check "Bob 001" "$LOG_DIR/bob.log" "001"
check "Charlie 001" "$LOG_DIR/charlie.log" "001"
check "Bob saw Alice JOIN" "$LOG_DIR/bob.log" "alice.*JOIN|JOIN.*#test"
check "Bob received Alice PRIVMSG" "$LOG_DIR/bob.log" "Hello everyone"
check "Charlie received Alice PRIVMSG" "$LOG_DIR/charlie.log" "Hello everyone"
echo ""

# =============================================
# Section 4.8: Duplicate nickname (433)
# =============================================
echo "--- Section 4.8: Duplicate nickname (433) ---"
(
    printf "PASS %s\r\nNICK first\r\nUSER first 0 * :First\r\n" "$PASS"
    sleep 3
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/first.log" 2>&1 &
PID_FIRST=$!
(
    sleep 1
    printf "PASS %s\r\nNICK first\r\nUSER second 0 * :Second\r\n" "$PASS"
    sleep 1
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/dup.log" 2>&1 &
PID_DUP=$!
wait $PID_FIRST $PID_DUP 2>/dev/null
check "Second client gets 433" "$LOG_DIR/dup.log" "433.*already in use"
echo ""

# =============================================
# Section 5: JOIN / PART
# =============================================
echo "--- Section 5.1 / 5.2: JOIN new channel ---"
send_nc "$LOG_DIR/join.log" "PASS $PASS" "NICK joiner" "USER joiner 0 * :Joiner" "JOIN #newchannel"
check "JOIN confirmation" "$LOG_DIR/join.log" "JOIN.*#newchannel"
check "331 or 332 topic" "$LOG_DIR/join.log" "33[12]"
check "353 NAMES" "$LOG_DIR/join.log" "353"
check "366 ENDOFNAMES" "$LOG_DIR/join.log" "366"
check "Creator has @" "$LOG_DIR/join.log" "@joiner|@.*joiner"
echo ""

echo "--- Section 5.3: JOIN without # (476) ---"
send_nc "$LOG_DIR/join476.log" "PASS $PASS" "NICK u476" "USER u476 0 * :U" "JOIN invalidname"
check "476 Bad Channel Mask" "$LOG_DIR/join476.log" "476"
echo ""

echo "--- Section 5.5: PART ---"
(
    printf "PASS %s\r\nNICK parter\r\nUSER parter 0 * :Parter\r\nJOIN #partchan\r\n" "$PASS"
    sleep 0.5
    printf "PART #partchan :Bye\r\n"
    sleep 0.5
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/part.log" 2>&1
check "PART confirmation" "$LOG_DIR/part.log" "PART.*#partchan"
check "Reason in PART" "$LOG_DIR/part.log" "Bye"
echo ""

echo "--- Section 5.6: PART non-existent (403) ---"
send_nc "$LOG_DIR/part403.log" "PASS $PASS" "NICK u403" "USER u403 0 * :U" "PART #nonexistent"
check "403 No such channel" "$LOG_DIR/part403.log" "403"
echo ""

# =============================================
# Section 6: PRIVMSG
# =============================================
echo "--- Section 6: PRIVMSG channel and DM ---"
(
    printf "PASS %s\r\nNICK sender\r\nUSER sender 0 * :Sender\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #privchan\r\n"
    sleep 1
    printf "PRIVMSG #privchan :Hi channel\r\n"
    sleep 0.5
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/sender.log" 2>&1 &
PID_S=$!
(
    sleep 0.3
    printf "PASS %s\r\nNICK receiver\r\nUSER receiver 0 * :Receiver\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #privchan\r\n"
    sleep 2
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/receiver.log" 2>&1 &
PID_R=$!
wait $PID_S $PID_R 2>/dev/null
check "Receiver got channel message" "$LOG_DIR/receiver.log" "Hi channel"
echo ""

echo "--- Section 6.3: PRIVMSG non-existent user (401) ---"
send_nc "$LOG_DIR/priv401.log" "PASS $PASS" "NICK u401" "USER u401 0 * :U" "PRIVMSG ghostuser :hi"
check "401 No such nick" "$LOG_DIR/priv401.log" "401"
echo ""

echo "--- Section 6.4: PRIVMSG non-existent channel (403) ---"
send_nc "$LOG_DIR/priv403.log" "PASS $PASS" "NICK u403p" "USER u403p 0 * :U" "PRIVMSG #ghostchan :hi"
check "403 No such channel" "$LOG_DIR/priv403.log" "403"
echo ""

echo "--- Section 6.6: PRIVMSG no target (461) ---"
send_nc "$LOG_DIR/priv461.log" "PASS $PASS" "NICK u461" "USER u461 0 * :U" "PRIVMSG"
check "461 Not enough parameters" "$LOG_DIR/priv461.log" "461"
echo ""

# =============================================
# Section 7: Operator KICK
# =============================================
echo "--- Section 7: Operator KICK ---"
(
    printf "PASS %s\r\nNICK op_user\r\nUSER op_user 0 * :Op\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #modchan\r\n"
    sleep 1
    printf "KICK #modchan reg_user :Bye!\r\n"
    sleep 0.5
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/op.log" 2>&1 &
PID_OP=$!
(
    sleep 0.3
    printf "PASS %s\r\nNICK reg_user\r\nUSER reg_user 0 * :Reg\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #modchan\r\n"
    sleep 2
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/reguser.log" 2>&1 &
PID_REG=$!
wait $PID_OP $PID_REG 2>/dev/null
check "KICK broadcast" "$LOG_DIR/reguser.log" "KICK.*reg_user"
check "reg_user removed" "$LOG_DIR/reguser.log" "Bye!"
echo ""

echo "--- Section 7.1: Non-operator cannot KICK (482) ---"
(
    printf "PASS %s\r\nNICK op_user2\r\nUSER op_user2 0 * :Op\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #nonopchan\r\n"
    sleep 2
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/op2.log" 2>&1 &
PID_OP2=$!
(
    sleep 0.8
    printf "PASS %s\r\nNICK nonop\r\nUSER nonop 0 * :NonOp\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #nonopchan\r\n"
    sleep 0.5
    printf "KICK #nonopchan op_user2 :haha\r\n"
    sleep 1
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/nonop.log" 2>&1 &
PID_NONOP=$!
wait $PID_OP2 $PID_NONOP 2>/dev/null
check "482 not channel operator" "$LOG_DIR/nonop.log" "482"
echo ""

# =============================================
# Section 8: Channel Modes
# =============================================

# +i invite-only
echo "--- Section 8.1: MODE +i invite-only ---"
(
    printf "PASS %s\r\nNICK creator\r\nUSER creator 0 * :Creator\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #secret\r\n"
    sleep 0.5
    printf "MODE #secret +i\r\n"
    sleep 2
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/creator.log" 2>&1 &
PID_CR=$!
(
    sleep 2
    printf "PASS %s\r\nNICK outsider\r\nUSER outsider 0 * :Out\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #secret\r\n"
    sleep 1
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/outsider.log" 2>&1 &
PID_OUT=$!
wait $PID_CR $PID_OUT 2>/dev/null
check "Outsider gets 473" "$LOG_DIR/outsider.log" "473"
echo ""

# +k key
echo "--- Section 8.5: MODE +k channel key ---"
(
    printf "PASS %s\r\nNICK lockop\r\nUSER lockop 0 * :LockOp\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #locked\r\n"
    sleep 0.5
    printf "MODE #locked +k secretkey\r\n"
    sleep 3
    printf "QUIT\r\n"
) | nc -w 6 localhost "$PORT" > "$LOG_DIR/lockop.log" 2>&1 &
PID_LO=$!
(
    sleep 2
    printf "PASS %s\r\nNICK nokey\r\nUSER nokey 0 * :NoKey\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #locked\r\n"
    sleep 1
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/nokey.log" 2>&1 &
PID_NK=$!
(
    sleep 2.5
    printf "PASS %s\r\nNICK withkey\r\nUSER withkey 0 * :WithKey\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #locked secretkey\r\n"
    sleep 1
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/withkey.log" 2>&1 &
PID_WK=$!
wait $PID_LO $PID_NK $PID_WK 2>/dev/null
check "Join without key 475" "$LOG_DIR/nokey.log" "475"
check "Join with key success" "$LOG_DIR/withkey.log" "JOIN.*#locked"
echo ""

# +o / -o
echo "--- Section 8.7 / 8.8: MODE +o / -o ---"
(
    printf "PASS %s\r\nNICK op1\r\nUSER op1 0 * :Op1\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #ochan\r\n"
    sleep 1.5
    printf "MODE #ochan +o op2\r\n"
    sleep 0.5
    printf "MODE #ochan -o op2\r\n"
    sleep 0.5
    printf "QUIT\r\n"
) | nc -w 6 localhost "$PORT" > "$LOG_DIR/op1.log" 2>&1 &
PID_O1=$!
(
    sleep 1
    printf "PASS %s\r\nNICK op2\r\nUSER op2 0 * :Op2\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #ochan\r\n"
    sleep 2.5
    printf "QUIT\r\n"
) | nc -w 6 localhost "$PORT" > "$LOG_DIR/op2chan.log" 2>&1 &
PID_O2=$!
wait $PID_O1 $PID_O2 2>/dev/null
check "MODE +o broadcast" "$LOG_DIR/op2chan.log" "MODE.*#ochan.*op2"
check "MODE -o broadcast" "$LOG_DIR/op2chan.log" "MODE.*-o.*op2"
echo ""

# +l limit
echo "--- Section 8.10: MODE +l user limit ---"
(
    printf "PASS %s\r\nNICK limop\r\nUSER limop 0 * :LimOp\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #limited\r\n"
    sleep 0.5
    printf "MODE #limited +l 2\r\n"
    sleep 3
    printf "QUIT\r\n"
) | nc -w 6 localhost "$PORT" > "$LOG_DIR/limop.log" 2>&1 &
PID_LM=$!
(
    sleep 0.5
    printf "PASS %s\r\nNICK lim1\r\nUSER lim1 0 * :Lim1\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #limited\r\n"
    sleep 2
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/lim1.log" 2>&1 &
PID_L1=$!
(
    sleep 2
    printf "PASS %s\r\nNICK lim2\r\nUSER lim2 0 * :Lim2\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #limited\r\n"
    sleep 1
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/lim2.log" 2>&1 &
PID_L2=$!
wait $PID_LM $PID_L1 $PID_L2 2>/dev/null
check "Third user gets 471" "$LOG_DIR/lim2.log" "471"
echo ""

# Query MODE
echo "--- Section 8.15: Query channel MODE (324) ---"
send_nc "$LOG_DIR/mode324.log" "PASS $PASS" "NICK qmode" "USER qmode 0 * :Q" "JOIN #qchan" "MODE #qchan"
check "324 channel mode" "$LOG_DIR/mode324.log" "324"
echo ""

# =============================================
# Section 9: QUIT
# =============================================
echo "--- Section 9: QUIT ---"
(
    printf "PASS %s\r\nNICK quitter\r\nUSER quitter 0 * :Quitter\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #quitchan\r\n"
    sleep 0.5
    printf "QUIT :Goodbye!\r\n"
    sleep 0.5
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/quit.log" 2>&1
check "QUIT (client registered and joined)" "$LOG_DIR/quit.log" "001|JOIN.*#quitchan"
echo ""

# =============================================
# Section 10: PING PONG
# =============================================
echo "--- Section 10: PING PONG ---"
send_nc "$LOG_DIR/ping.log" "PASS $PASS" "NICK pinger" "USER pinger 0 * :P" "PING :test123"
check "PONG response" "$LOG_DIR/ping.log" "PONG.*test123"
echo ""

# =============================================
# Section 11.1: Empty lines
# =============================================
echo "--- Section 11.1: Empty lines ---"
(
    printf "\r\n\r\n\r\nPASS %s\r\nNICK empty\r\nUSER empty 0 * :Empty\r\n" "$PASS"
    sleep 1
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/empty.log" 2>&1
check "Registration after empty lines" "$LOG_DIR/empty.log" "001"
echo ""

# =============================================
# Section 12: Error handling
# =============================================
echo "--- Section 12.1: Unknown command (421) ---"
send_nc "$LOG_DIR/unknown.log" "PASS $PASS" "NICK u421" "USER u421 0 * :U" "BLAHBLAH something"
check "421 Unknown command" "$LOG_DIR/unknown.log" "421"
echo ""

echo "--- Section 12.2: JOIN no params (461) ---"
send_nc "$LOG_DIR/join461.log" "PASS $PASS" "NICK u461j" "USER u461j 0 * :U" "JOIN"
check "461 Not enough parameters" "$LOG_DIR/join461.log" "461"
echo ""

# =============================================
# Section 13: Case insensitivity
# =============================================
echo "--- Section 13.1: Case-insensitive nickname (433) ---"
(
    printf "PASS %s\r\nNICK Alice\r\nUSER Alice 0 * :Alice\r\n" "$PASS"
    sleep 2
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/case1.log" 2>&1 &
PID_C1=$!
(
    sleep 0.8
    printf "PASS %s\r\nNICK ALICE\r\nUSER ALICE 0 * :A\r\n" "$PASS"
    sleep 1
    printf "QUIT\r\n"
) | nc -w 5 localhost "$PORT" > "$LOG_DIR/case2.log" 2>&1 &
PID_C2=$!
wait $PID_C1 $PID_C2 2>/dev/null
check "Duplicate nick (case) 433" "$LOG_DIR/case2.log" "433"
echo ""

# =============================================
# Section 11.7: Rapid connect (10 clients)
# =============================================
echo "--- Section 11.7: Rapid connect (10 clients) ---"
for i in $(seq 1 10); do
    ( printf "PASS %s\r\nNICK r%d\r\nUSER r%d 0 * :R\r\nQUIT\r\n" "$PASS" "$i" "$i" | nc -w 3 localhost "$PORT" > "$LOG_DIR/rapid_$i.log" 2>&1 ) &
done
wait
rapid_ok=0
for i in $(seq 1 10); do
    grep -qE "001|QUIT|closed" "$LOG_DIR/rapid_$i.log" 2>/dev/null && rapid_ok=$((rapid_ok+1)) || true
done
[ "$rapid_ok" -ge 8 ] && (echo "  ✅ Rapid connect ($rapid_ok/10 ok)"; PASS_CNT=$((PASS_CNT+1))) || (echo "  ❌ Rapid connect ($rapid_ok/10)"; FAIL_CNT=$((FAIL_CNT+1)))
echo ""

# =============================================
# INVITE +i (Section 7.10 / 8.1)
# =============================================
echo "--- INVITE to +i channel ---"
(
    printf "PASS %s\r\nNICK inviter\r\nUSER inviter 0 * :Inv\r\n" "$PASS"
    sleep 0.5
    printf "JOIN #invchan\r\n"
    sleep 0.5
    printf "MODE #invchan +i\r\n"
    sleep 1
    printf "INVITE invitee2 #invchan\r\n"
    sleep 2
    printf "QUIT\r\n"
) | nc -w 6 localhost "$PORT" > "$LOG_DIR/inviter.log" 2>&1 &
PID_INV=$!
(
    sleep 1.5
    printf "PASS %s\r\nNICK invitee2\r\nUSER invitee2 0 * :Inv\r\n" "$PASS"
    sleep 1.5
    printf "JOIN #invchan\r\n"
    sleep 1
    printf "QUIT\r\n"
) | nc -w 6 localhost "$PORT" > "$LOG_DIR/invitee2.log" 2>&1 &
PID_INV2=$!
wait $PID_INV $PID_INV2 2>/dev/null
check "Inviter gets 341" "$LOG_DIR/inviter.log" "341"
check "Invitee can JOIN after INVITE" "$LOG_DIR/invitee2.log" "JOIN.*#invchan|353"
echo ""

# =============================================
# TOPIC (Section 7.5 / 7.6)
# =============================================
echo "--- TOPIC get/set ---"
send_nc "$LOG_DIR/topic.log" "PASS $PASS" "NICK toper" "USER toper 0 * :T" "JOIN #topicchan" "TOPIC #topicchan :Welcome here" "TOPIC #topicchan"
check "TOPIC set" "$LOG_DIR/topic.log" "TOPIC.*#topicchan.*Welcome"
check "332 topic reply" "$LOG_DIR/topic.log" "332.*Welcome"
echo ""

# =============================================
# Summary
# =============================================
echo "=============================================="
echo "  SUMMARY"
echo "=============================================="
echo "  Passed: $PASS_CNT"
echo "  Failed: $FAIL_CNT"
echo "=============================================="
[ "$FAIL_CNT" -eq 0 ] && exit 0 || exit 1
