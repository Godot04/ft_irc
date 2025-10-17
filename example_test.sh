#!/bin/bash

# Example: Complete Memory Leak Testing Session
# This shows exactly how to test your IRC server for memory leaks and segfaults

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Complete IRC Server Memory Testing Example                 ║"
echo "╔══════════════════════════════════════════════════════════════╗"
echo ""

PORT=6667
PASS="secretpass"

# Step 1: Compile
echo "Step 1: Compiling server..."
make re
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi
echo "✓ Compilation successful"
echo ""

# Step 2: Run basic Valgrind test
echo "Step 2: Running Valgrind test (30 seconds)..."
echo "Starting server with Valgrind..."

valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=example_valgrind.log \
         ./ft_irc_serv $PORT $PASS &

SERVER_PID=$!
sleep 3

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "❌ Server failed to start!"
    cat example_valgrind.log
    exit 1
fi

echo "✓ Server running (PID: $SERVER_PID)"
echo ""

# Step 3: Send test connections
echo "Step 3: Sending test connections..."

echo "  → Test 1: Basic connection"
(echo -e "PASS $PASS\r\nNICK alice\r\nUSER alice 0 * :Alice\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
sleep 1

echo "  → Test 2: Join channel"
(echo -e "PASS $PASS\r\nNICK bob\r\nUSER bob 0 * :Bob\r\nJOIN #lobby\r\nPRIVMSG #lobby :Hello\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
sleep 1

echo "  → Test 3: Multiple users"
for i in {1..5}; do
    (echo -e "PASS $PASS\r\nNICK user$i\r\nUSER user$i 0 * :User$i\r\nJOIN #test\r\nQUIT\r\n" | nc -q 1 localhost $PORT) &
done
sleep 2

echo "  → Test 4: Abrupt disconnect"
(echo -e "PASS $PASS\r\nNICK abrupter\r\n" | timeout 0.3 nc localhost $PORT) &
sleep 1

echo "✓ Tests completed"
echo ""

# Step 4: Stop server
echo "Step 4: Stopping server..."
kill -INT $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
echo "✓ Server stopped"
echo ""

# Step 5: Analyze results
echo "Step 5: Analyzing Valgrind results..."
echo "════════════════════════════════════════"
echo ""

# Extract leak summary
echo "MEMORY LEAK SUMMARY:"
grep "LEAK SUMMARY" example_valgrind.log -A 5 | head -6
echo ""

# Extract error summary
echo "ERROR SUMMARY:"
grep "ERROR SUMMARY" example_valgrind.log | tail -1
echo ""

# Check for definite leaks
DEFINITELY_LOST=$(grep "definitely lost:" example_valgrind.log | awk '{print $4}' | head -1)

if [ "$DEFINITELY_LOST" = "0" ]; then
    echo "✅ SUCCESS: No memory leaks detected!"
else
    echo "❌ WARNING: Found $DEFINITELY_LOST bytes definitely lost!"
    echo ""
    echo "Leak details (first occurrence):"
    grep -A 15 "definitely lost" example_valgrind.log | head -20
fi

echo ""
echo "════════════════════════════════════════"
echo ""

# Step 6: Show what to do next
echo "Step 6: Next steps..."
echo ""

if [ "$DEFINITELY_LOST" = "0" ]; then
    echo "Great! Your server passed the basic memory leak test."
    echo ""
    echo "Recommended next steps:"
    echo "  1. Run stress test:"
    echo "     ./stress_test.sh $PORT $PASS"
    echo ""
    echo "  2. Run automated tester:"
    echo "     ./memory_leak_tester.sh"
    echo ""
    echo "  3. Long-running test (leave for 30+ minutes):"
    echo "     valgrind --leak-check=full ./ft_irc_serv $PORT $PASS"
    echo "     # In another terminal, periodically run:"
    echo "     for i in {1..100}; do ./stress_test.sh; sleep 60; done"
else
    echo "Memory leaks detected. Here's how to fix:"
    echo ""
    echo "  1. Read the full Valgrind log:"
    echo "     cat example_valgrind.log | less"
    echo ""
    echo "  2. Look for the source of the leak:"
    echo "     grep -B 5 \"definitely lost\" example_valgrind.log"
    echo ""
    echo "  3. Check the critical bugs file:"
    echo "     cat CRITICAL_BUGS.md"
    echo ""
    echo "  4. Use GDB to debug:"
    echo "     gdb ./ft_irc_serv"
    echo "     (gdb) run $PORT $PASS"
fi

echo ""
echo "Full Valgrind log saved to: example_valgrind.log"
echo ""

# Step 7: Quick code analysis
echo "Step 7: Quick source code analysis..."
echo ""

NEW_COUNT=$(grep -r "new " srcs/*.cpp inc/*.hpp 2>/dev/null | grep -v "//" | wc -l)
DELETE_COUNT=$(grep -r "delete " srcs/*.cpp 2>/dev/null | grep -v "//" | wc -l)

echo "Source code statistics:"
echo "  • Found $NEW_COUNT instances of 'new'"
echo "  • Found $DELETE_COUNT instances of 'delete'"

if [ $NEW_COUNT -gt $((DELETE_COUNT + 3)) ]; then
    echo "  ⚠️  Warning: Significantly more 'new' than 'delete'"
    echo "     This might indicate missing cleanup code"
else
    echo "  ✓ Reasonable balance between new/delete"
fi

echo ""

# Summary
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Testing Session Complete                                    ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

if [ "$DEFINITELY_LOST" = "0" ]; then
    echo "🎉 Status: PASSED"
else
    echo "⚠️  Status: NEEDS ATTENTION"
fi

echo ""
echo "Files generated:"
echo "  • example_valgrind.log - Full Valgrind output"
echo ""
echo "For more testing options, see:"
echo "  • QUICK_LEAK_GUIDE.md - Quick reference"
echo "  • DEBUGGING_GUIDE.md - Comprehensive guide"
echo "  • CRITICAL_BUGS.md - Known issues + fixes"
echo ""
