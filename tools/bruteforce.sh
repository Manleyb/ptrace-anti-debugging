#!/bin/bash
# Brute force numeric passwords against crackme binaries
# Iterates through a range and checks for success indicators in output

BINARY="$1"
START="${2:-0}"
END="${3:-99999}"

if [ -z "$BINARY" ]; then
    echo "Usage: $0 <binary> [start] [end]"
    echo "  binary: Path to the crackme executable"
    echo "  start:  Starting number (default: 0)"
    echo "  end:    Ending number (default: 99999)"
    exit 1
fi

if [ ! -x "$BINARY" ]; then
    echo "Error: $BINARY is not executable or does not exist"
    exit 1
fi

echo "Brute forcing $BINARY from $START to $END..."

for (( i=START; i<=END; i++ )); do
    if echo "$i" | "$BINARY" 2>&1 | grep -qi "OK"; then
        echo "Password found: $i"
        exit 0
    fi
done

echo "Password not found in range $START-$END"
exit 1
