#!/usr/bin/env bash
set -e
echo "Testing compilation environment..."
gcc --version | head -n 1
ld --version | head -n 1
xorriso --version | head -n 1
echo "All tools accessible."
