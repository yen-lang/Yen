#!/bin/bash
# Yen Language Test Runner
# Usage: ./tests/run_tests.sh [path/to/yen]

YEN="${1:-./build/yen}"
DIR="$(dirname "$0")"

if [ ! -f "$YEN" ]; then
    echo "Error: yen binary not found at $YEN"
    echo "Build first: cd build && make yen"
    exit 1
fi

pass=0
fail=0
total=0

for f in "$DIR"/*.yen; do
    name=$(basename "$f")
    # Skip server apps (they run forever, not unit tests)
    case "$name" in webserver.yen|http_client.yen|tcp_shell_server.yen|tcp_shell_client.yen) continue ;; esac
    total=$((total + 1))
    result=$("$YEN" "$f" 2>&1)
    rc=$?
    if [ $rc -eq 0 ]; then
        printf "  PASS  %s\n" "$name"
        pass=$((pass + 1))
    else
        printf "  FAIL  %s\n" "$name"
        echo "$result" | tail -3 | sed 's/^/        /'
        fail=$((fail + 1))
    fi
done

echo
echo "Results: $pass/$total passed"
if [ $fail -gt 0 ]; then
    echo "$fail test(s) FAILED"
    exit 1
else
    echo "All tests passed!"
fi
