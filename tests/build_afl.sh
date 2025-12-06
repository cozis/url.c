#!/bin/bash
# Build script for AFL++ fuzzing
#
# Prerequisites:
#   - AFL++ installed (https://github.com/AFLplusplus/AFLplusplus)
#
# Install AFL++ on Ubuntu/Debian:
#   sudo apt-get install afl++
#
# Or build from source:
#   git clone https://github.com/AFLplusplus/AFLplusplus
#   cd AFLplusplus
#   make
#   sudo make install

set -e

echo "Building fuzz target with AFL++..."

# Try different AFL++ compilers in order of preference
if command -v afl-clang-fast &> /dev/null; then
    echo "Using afl-clang-fast (recommended)"
    AFL_CC=afl-clang-fast
elif command -v afl-gcc &> /dev/null; then
    echo "Using afl-gcc"
    AFL_CC=afl-gcc
elif command -v afl-clang &> /dev/null; then
    echo "Using afl-clang"
    AFL_CC=afl-clang
else
    echo "Error: No AFL++ compiler found!"
    echo "Please install AFL++ first:"
    echo "  sudo apt-get install afl++"
    echo "Or see: https://github.com/AFLplusplus/AFLplusplus"
    exit 1
fi

# Build the fuzz target
$AFL_CC -o fuzz_afl fuzz_afl.c ../url.c -Wall -Wextra -ggdb

echo "Build successful! Binary: fuzz_afl"
echo ""
echo "To run fuzzing:"
echo "  ./run_afl.sh"
