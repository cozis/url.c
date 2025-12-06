#!/bin/bash
# Run AFL++ fuzzing
#
# Usage:
#   ./run_afl.sh [duration_in_minutes]
#
# Examples:
#   ./run_afl.sh        # Run indefinitely (press Ctrl+C to stop)
#   ./run_afl.sh 60     # Run for 60 minutes

set -e

# Check if fuzz target exists
if [ ! -f "fuzz_afl" ]; then
    echo "Error: fuzz_afl binary not found!"
    echo "Please run ./build_afl.sh first"
    exit 1
fi

# Check if AFL++ is installed
if ! command -v afl-fuzz &> /dev/null; then
    echo "Error: afl-fuzz not found!"
    echo "Please install AFL++:"
    echo "  sudo apt-get install afl++"
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p findings

# Set AFL++ options for better performance
export AFL_SKIP_CPUFREQ=1
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1

echo "Starting AFL++ fuzzing..."
echo "Corpus directory: corpus/"
echo "Output directory: findings/"
echo ""
echo "Press Ctrl+C to stop fuzzing"
echo ""

# Run AFL++ with optional timeout and dictionary
DICT_ARG=""
if [ -f "url.dict" ]; then
    DICT_ARG="-x url.dict"
    echo "Using dictionary: url.dict"
fi

if [ -n "$1" ]; then
    timeout_sec=$((60 * $1))
    echo "Running for $1 minutes ($timeout_sec seconds)"
    timeout $timeout_sec afl-fuzz -i corpus/ -o findings/ $DICT_ARG -- ./fuzz_afl || true
else
    afl-fuzz -i corpus/ -o findings/ $DICT_ARG -- ./fuzz_afl
fi

echo ""
echo "Fuzzing completed!"
echo ""
echo "Results:"
echo "  Crashes: findings/default/crashes/"
echo "  Hangs:   findings/default/hangs/"
echo "  Queue:   findings/default/queue/"
echo ""

# Check for crashes
if [ -d "findings/default/crashes" ]; then
    num_crashes=$(find findings/default/crashes -type f ! -name 'README.txt' | wc -l)
    if [ $num_crashes -gt 0 ]; then
        echo "⚠️  Found $num_crashes crash(es)!"
        echo "To reproduce a crash:"
        echo "  ./fuzz_afl < findings/default/crashes/id:000000,*"
    else
        echo "✓ No crashes found"
    fi
fi

# Check for hangs
if [ -d "findings/default/hangs" ]; then
    num_hangs=$(find findings/default/hangs -type f ! -name 'README.txt' | wc -l)
    if [ $num_hangs -gt 0 ]; then
        echo "⚠️  Found $num_hangs hang(s)!"
    else
        echo "✓ No hangs found"
    fi
fi
