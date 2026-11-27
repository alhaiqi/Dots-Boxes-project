#!/bin/sh
# net_game.sh - Plays a full network game between two copies of the game on this machine.
#
#   sh tests/net_game.sh                       normal run
#   TOOL="valgrind --tool=helgrind" sh ...     run both players under a Valgrind tool
#
# The host (Player A) and the client (Player B) both "type" every line of the board in
# order. A line the other player already drew is rejected and the next one is tried, so
# the game always runs to the end. Both sides must then report the same final result.

PORT=${PORT:-5599}
DIR=$(mktemp -d)
BIN=./dots_and_boxes

moves() {
    r=0; while [ $r -le 4 ]; do c=0; while [ $c -le 4 ]; do
        echo "$r $c $r $((c + 1))"; c=$((c + 1)); done; r=$((r + 1)); done
    r=0; while [ $r -le 3 ]; do c=0; while [ $c -le 5 ]; do
        echo "$r $c $((r + 1)) $c"; c=$((c + 1)); done; r=$((r + 1)); done
}

{ echo 3; echo 1; echo $PORT; moves; echo; echo 0; } > "$DIR/host.txt"
{ echo 3; echo 2; echo 127.0.0.1; echo $PORT; moves; echo; echo 0; } > "$DIR/join.txt"

$TOOL $BIN < "$DIR/host.txt" > "$DIR/host.out" 2> "$DIR/host.err" &
HOST_PID=$!
if [ -n "$TOOL" ]; then sleep 6; else sleep 1; fi   # let the host start listening
$TOOL $BIN < "$DIR/join.txt" > "$DIR/join.out" 2> "$DIR/join.err"
JOIN_STATUS=$?
wait $HOST_PID
HOST_STATUS=$?

HOST_RESULT=$(grep -E "wins|draw" "$DIR/host.out" | tail -1)
JOIN_RESULT=$(grep -E "wins|draw" "$DIR/join.out" | tail -1)
echo "Host (A) saw:   $HOST_RESULT"
echo "Client (B) saw: $JOIN_RESULT"
if [ -n "$TOOL" ]; then
    grep -h "ERROR SUMMARY" "$DIR/host.err" "$DIR/join.err"
fi

STATUS=0
if [ -z "$HOST_RESULT" ] || [ "$(echo "$HOST_RESULT" | sed 's/ (you)//; s/ (opponent)//')" != \
     "$(echo "$JOIN_RESULT" | sed 's/ (you)//; s/ (opponent)//')" ]; then
    echo "FAILED: the two players do not agree on the result (outputs kept in $DIR)"
    STATUS=1
elif [ $HOST_STATUS -ne 0 ] || [ $JOIN_STATUS -ne 0 ]; then
    echo "FAILED: exit codes host=$HOST_STATUS client=$JOIN_STATUS (outputs kept in $DIR)"
    STATUS=1
else
    echo "Network game OK: both sides finished with the same result."
    rm -rf "$DIR"
fi
exit $STATUS
