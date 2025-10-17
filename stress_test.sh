#!/bin/bash

# Simple Stress Test for ft_irc
# This script helps trigger potential memory leaks and segfaults

PORT=${1:-6667}
PASS=${2:-"testpass"}

echo "🔥 IRC Server Stress Test"
echo "=========================="
echo "Port: $PORT"
echo "Password: $PASS"
echo ""

# Test 1: Rapid connections and disconnections
echo "[Test 1] Rapid connect/disconnect (50 clients)..."
for i in {1..50}; do
    (echo -e "PASS $PASS\r\nNICK rapid$i\r\nUSER rapid$i 0 * :Rapid $i\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
done
wait
echo "✓ Test 1 complete"
echo ""

# Test 2: Abrupt disconnections
echo "[Test 2] Abrupt disconnections (20 clients)..."
for i in {1..20}; do
    (echo -e "PASS $PASS\r\nNICK abrupt$i\r\nUSER abrupt$i 0 * :Abrupt $i\r\n" | timeout 0.5 nc localhost $PORT) &
done
wait
echo "✓ Test 2 complete"
echo ""

# Test 3: Channel operations
echo "[Test 3] Channel operations (10 clients in 5 channels)..."
for i in {1..10}; do
    CHAN=$((i % 5 + 1))
    (echo -e "PASS $PASS\r\nNICK chan$i\r\nUSER chan$i 0 * :Chan $i\r\nJOIN #channel$CHAN\r\nPRIVMSG #channel$CHAN :Test message $i\r\nPART #channel$CHAN\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
done
wait
echo "✓ Test 3 complete"
echo ""

# Test 4: Invalid commands
echo "[Test 4] Invalid commands (10 clients)..."
for i in {1..10}; do
    (echo -e "PASS $PASS\r\nNICK invalid$i\r\nUSER invalid$i 0 * :Invalid $i\r\nINVALIDCOMMAND\r\nNONEXISTENT\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
done
wait
echo "✓ Test 4 complete"
echo ""

# Test 5: MODE operations
echo "[Test 5] MODE operations (5 clients)..."
for i in {1..5}; do
    (echo -e "PASS $PASS\r\nNICK mode$i\r\nUSER mode$i 0 * :Mode $i\r\nJOIN #modetest\r\nMODE #modetest +i\r\nMODE #modetest +t\r\nMODE #modetest +k testkey\r\nMODE #modetest -i\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
done
wait
echo "✓ Test 5 complete"
echo ""

# Test 6: Private messages
echo "[Test 6] Private messages (4 clients)..."
(echo -e "PASS $PASS\r\nNICK priv1\r\nUSER priv1 0 * :Priv 1\r\n" | nc -q 10 localhost $PORT) &
sleep 1
(echo -e "PASS $PASS\r\nNICK priv2\r\nUSER priv2 0 * :Priv 2\r\nPRIVMSG priv1 :Hello priv1\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
(echo -e "PASS $PASS\r\nNICK priv3\r\nUSER priv3 0 * :Priv 3\r\nPRIVMSG priv1 :Hello from priv3\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
sleep 2
(echo -e "QUIT\r\n" | nc -q 1 localhost $PORT) &
wait
echo "✓ Test 6 complete"
echo ""

# Test 7: Wrong password attempts
echo "[Test 7] Authentication failures (10 clients)..."
for i in {1..10}; do
    (echo -e "PASS wrongpass$i\r\nNICK auth$i\r\nUSER auth$i 0 * :Auth $i\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
done
wait
echo "✓ Test 7 complete"
echo ""

# Test 8: Nickname conflicts
echo "[Test 8] Nickname conflicts (5 clients)..."
for i in {1..5}; do
    (echo -e "PASS $PASS\r\nNICK samename\r\nUSER user$i 0 * :User $i\r\nNICK unique$i\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
done
wait
echo "✓ Test 8 complete"
echo ""

# Test 9: Long-running connections
echo "[Test 9] Long-running connections (3 clients for 5 seconds)..."
for i in {1..3}; do
    (echo -e "PASS $PASS\r\nNICK long$i\r\nUSER long$i 0 * :Long $i\r\n"; sleep 5; echo -e "QUIT\r\n") | nc -q 1 localhost $PORT &
done
sleep 6
echo "✓ Test 9 complete"
echo ""

# Test 10: Maximum message flood
echo "[Test 10] Message flood (1 client sending 100 messages)..."
{
    echo -e "PASS $PASS\r\nNICK flooder\r\nUSER flooder 0 * :Flooder\r\nJOIN #flood\r\n"
    for i in {1..100}; do
        echo -e "PRIVMSG #flood :Message number $i\r\n"
    done
    echo -e "QUIT\r\n"
} | nc -q 1 localhost $PORT &
wait
echo "✓ Test 10 complete"
echo ""

echo "=========================="
echo "🎉 All stress tests completed!"
echo ""
echo "Check server output for errors or crashes"
echo "Run with Valgrind to check for memory leaks:"
echo "  valgrind --leak-check=full ./ft_irc_serv $PORT $PASS"
