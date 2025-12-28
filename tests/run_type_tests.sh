#!/bin/bash
# Type Checker Test Suite Runner

echo "==========================================="
echo "YEN Type Checker Test Suite"
echo "==========================================="
echo ""

# Compile the test program
echo "Compiling test program..."
g++ -std=c++17 -I ../include \
    run_type_tests.cpp \
    ../src/lexer.cpp \
    ../src/parser.cpp \
    ../src/type_checker.cpp \
    -o type_test_runner || exit 1

echo "✅ Test program compiled"
echo ""

# Run tests
echo "==========================================="
echo "Running Type Checker Tests"
echo "==========================================="
echo ""

./type_test_runner

echo ""
echo "==========================================="
echo "Test Suite Complete"
echo "==========================================="
