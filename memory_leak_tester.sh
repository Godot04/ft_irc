#!/bin/bash

# Memory Leak and Segfault Tester for ft_irc
# This script helps identify memory leaks and segfaults in your IRC server

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SERVER_PORT=6667
SERVER_PASS="testpass123"
SERVER_BINARY="./ft_irc_serv"
VALGRIND_LOG="valgrind_output.log"
HELGRIND_LOG="helgrind_output.log"

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  FT_IRC Memory Leak & Segfault Tester     ║${NC}"
echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo ""

# Function to compile the server
compile_server() {
    echo -e "${YELLOW}[1/6] Compiling server with debug symbols...${NC}"
    make re > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Compilation successful${NC}"
    else
        echo -e "${RED}✗ Compilation failed${NC}"
        make re
        exit 1
    fi
}

# Function to run Valgrind memory leak check
run_valgrind() {
    echo -e "${YELLOW}[2/6] Running Valgrind memory leak detection...${NC}"
    echo -e "${BLUE}This will start the server and run for 30 seconds${NC}"

    # Start server with Valgrind in background
    valgrind --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             --verbose \
             --log-file=$VALGRIND_LOG \
             $SERVER_BINARY $SERVER_PORT $SERVER_PASS &

    SERVER_PID=$!
    sleep 2

    # Check if server started
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${RED}✗ Server failed to start${NC}"
        cat $VALGRIND_LOG
        exit 1
    fi

    echo -e "${GREEN}✓ Server started (PID: $SERVER_PID)${NC}"

    # Run test connections
    echo -e "${BLUE}  Running test connections...${NC}"
    sleep 1

    # Test 1: Basic connection and disconnect
    (echo -e "PASS $SERVER_PASS\r\nNICK testuser1\r\nUSER testuser1 0 * :Test User\r\nQUIT\r\n" | nc localhost $SERVER_PORT) &
    sleep 1

    # Test 2: Multiple connections
    for i in {1..5}; do
        (echo -e "PASS $SERVER_PASS\r\nNICK user$i\r\nUSER user$i 0 * :User $i\r\nJOIN #test\r\nPRIVMSG #test :Hello\r\nQUIT\r\n" | nc localhost $SERVER_PORT) &
    done
    sleep 2

    # Test 3: Connection without proper quit
    (echo -e "PASS $SERVER_PASS\r\nNICK abrupter\r\n" | nc localhost $SERVER_PORT) &
    sleep 1

    echo -e "${BLUE}  Waiting for tests to complete...${NC}"
    sleep 5

    # Stop server gracefully
    echo -e "${BLUE}  Stopping server...${NC}"
    kill -INT $SERVER_PID 2>/dev/null || kill -9 $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null

    echo -e "${GREEN}✓ Valgrind test complete${NC}"
}

# Function to analyze Valgrind output
analyze_valgrind() {
    echo -e "${YELLOW}[3/6] Analyzing Valgrind results...${NC}"

    if [ ! -f $VALGRIND_LOG ]; then
        echo -e "${RED}✗ Valgrind log not found${NC}"
        return
    fi

    # Check for memory leaks
    DEFINITELY_LOST=$(grep "definitely lost:" $VALGRIND_LOG | awk '{print $4}')
    INDIRECTLY_LOST=$(grep "indirectly lost:" $VALGRIND_LOG | awk '{print $4}')
    POSSIBLY_LOST=$(grep "possibly lost:" $VALGRIND_LOG | awk '{print $4}')
    STILL_REACHABLE=$(grep "still reachable:" $VALGRIND_LOG | awk '{print $4}')

    echo ""
    echo -e "${BLUE}Memory Leak Summary:${NC}"
    echo -e "  Definitely lost:   ${RED}${DEFINITELY_LOST:-0} bytes${NC}"
    echo -e "  Indirectly lost:   ${YELLOW}${INDIRECTLY_LOST:-0} bytes${NC}"
    echo -e "  Possibly lost:     ${YELLOW}${POSSIBLY_LOST:-0} bytes${NC}"
    echo -e "  Still reachable:   ${BLUE}${STILL_REACHABLE:-0} bytes${NC}"
    echo ""

    # Check for errors
    ERROR_COUNT=$(grep "ERROR SUMMARY:" $VALGRIND_LOG | tail -1 | awk '{print $4}')
    if [ "$ERROR_COUNT" = "0" ]; then
        echo -e "${GREEN}✓ No memory errors detected${NC}"
    else
        echo -e "${RED}✗ Found $ERROR_COUNT memory errors${NC}"
        echo -e "${YELLOW}  See $VALGRIND_LOG for details${NC}"
    fi

    # Show leak locations if any
    if [ "$DEFINITELY_LOST" != "0" ] && [ ! -z "$DEFINITELY_LOST" ]; then
        echo -e "${RED}Memory leak details:${NC}"
        grep -A 10 "definitely lost" $VALGRIND_LOG | head -20
    fi
}

# Function to run Helgrind for race conditions
run_helgrind() {
    echo -e "${YELLOW}[4/6] Running Helgrind for race condition detection...${NC}"
    echo -e "${BLUE}This checks for threading issues (if any)${NC}"

    valgrind --tool=helgrind \
             --log-file=$HELGRIND_LOG \
             $SERVER_BINARY $SERVER_PORT $SERVER_PASS &

    SERVER_PID=$!
    sleep 3

    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${RED}✗ Server failed to start with Helgrind${NC}"
        return
    fi

    # Quick test
    (echo -e "PASS $SERVER_PASS\r\nNICK test\r\nUSER test 0 * :Test\r\nQUIT\r\n" | nc localhost $SERVER_PORT) &
    sleep 2

    kill -INT $SERVER_PID 2>/dev/null || kill -9 $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null

    if grep -q "ERROR SUMMARY: 0 errors" $HELGRIND_LOG; then
        echo -e "${GREEN}✓ No race conditions detected${NC}"
    else
        echo -e "${YELLOW}⚠ Potential race conditions found - see $HELGRIND_LOG${NC}"
    fi
}

# Function to test with AddressSanitizer
run_asan() {
    echo -e "${YELLOW}[5/6] Testing with AddressSanitizer...${NC}"
    echo -e "${BLUE}Recompiling with ASAN flags...${NC}"

    # Backup Makefile
    cp Makefile Makefile.backup

    # Add ASAN flags
    if grep -q "fsanitize=address" Makefile; then
        echo -e "${BLUE}  ASAN flags already present${NC}"
    else
        echo -e "${YELLOW}  Note: Add -fsanitize=address -g to CXXFLAGS in Makefile for ASAN${NC}"
        echo -e "${YELLOW}  Skipping ASAN test (manual setup required)${NC}"
        return
    fi

    make re > /dev/null 2>&1

    # Run server briefly
    $SERVER_BINARY $SERVER_PORT $SERVER_PASS &
    SERVER_PID=$!
    sleep 2

    (echo -e "PASS $SERVER_PASS\r\nNICK asantest\r\nUSER asantest 0 * :ASAN Test\r\nQUIT\r\n" | nc localhost $SERVER_PORT) &
    sleep 2

    kill -INT $SERVER_PID 2>/dev/null || kill -9 $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null

    # Restore Makefile
    mv Makefile.backup Makefile
    make re > /dev/null 2>&1

    echo -e "${GREEN}✓ ASAN test complete${NC}"
}

# Function to check for common issues
check_common_issues() {
    echo -e "${YELLOW}[6/6] Checking for common memory issues...${NC}"

    echo -e "${BLUE}Checking source code for potential issues:${NC}"

    # Check for missing delete
    echo -n "  • Checking for new without delete... "
    NEW_COUNT=$(grep -r "new " srcs/*.cpp inc/*.hpp 2>/dev/null | grep -v "//" | wc -l)
    DELETE_COUNT=$(grep -r "delete " srcs/*.cpp 2>/dev/null | grep -v "//" | wc -l)
    if [ $NEW_COUNT -gt $DELETE_COUNT ]; then
        echo -e "${YELLOW}⚠ Found $NEW_COUNT 'new' but only $DELETE_COUNT 'delete'${NC}"
    else
        echo -e "${GREEN}✓${NC}"
    fi

    # Check for unclosed sockets
    echo -n "  • Checking for socket() without close()... "
    SOCKET_COUNT=$(grep -r "socket(" srcs/*.cpp 2>/dev/null | grep -v "//" | wc -l)
    CLOSE_COUNT=$(grep -r "close(" srcs/*.cpp 2>/dev/null | grep -v "//" | wc -l)
    if [ $SOCKET_COUNT -gt 0 ]; then
        echo -e "${GREEN}✓ (Found $CLOSE_COUNT close calls)${NC}"
    else
        echo -e "${GREEN}✓${NC}"
    fi

    # Check for missing null checks
    echo -n "  • Checking for potential null pointer dereferences... "
    grep -rn "->.*(" srcs/*.cpp inc/*.hpp 2>/dev/null | grep -v "if.*!=" | grep -v "if.*==" | head -5 > /tmp/potential_nulls.txt
    if [ -s /tmp/potential_nulls.txt ]; then
        echo -e "${YELLOW}⚠ Found potential issues (review code manually)${NC}"
    else
        echo -e "${GREEN}✓${NC}"
    fi
}

# Main execution
main() {
    compile_server
    run_valgrind
    analyze_valgrind
    run_helgrind
    # run_asan
    check_common_issues

    echo ""
    echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║  Testing Complete                         ║${NC}"
    echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
    echo ""
    echo -e "${YELLOW}Results saved to:${NC}"
    echo -e "  • $VALGRIND_LOG"
    echo -e "  • $HELGRIND_LOG"
    echo ""
    echo -e "${YELLOW}To view full Valgrind report:${NC}"
    echo -e "  cat $VALGRIND_LOG"
    echo ""
    echo -e "${YELLOW}For detailed debugging, run manually:${NC}"
    echo -e "  valgrind --leak-check=full --show-leak-kinds=all ./ft_irc_serv $SERVER_PORT $SERVER_PASS"
}

# Run main function
main
