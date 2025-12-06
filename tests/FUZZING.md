# Fuzzing the URL Parser

This directory contains fuzzing infrastructure for the URL parser using AFL++ (American Fuzzy Lop).

## What is Fuzzing?

Fuzzing is an automated testing technique that feeds malformed, unexpected, or random data to a program to find bugs, crashes, and security vulnerabilities. AFL++ is a coverage-guided fuzzer that intelligently mutates inputs to maximize code coverage.

## Prerequisites

### Install AFL++

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install afl++
```

**From source:**
```bash
git clone https://github.com/AFLplusplus/AFLplusplus
cd AFLplusplus
make
sudo make install
```

## Quick Start

1. **Build the fuzz target:**
   ```bash
   cd tests/
   ./build_afl.sh
   ```

2. **Run fuzzing:**
   ```bash
   ./run_afl.sh
   ```

   Or run for a specific duration:
   ```bash
   ./run_afl.sh 60  # Run for 60 minutes
   ```

3. **View results:**
   - Crashes: `findings/default/crashes/`
   - Hangs: `findings/default/hangs/`
   - Interesting inputs: `findings/default/queue/`

## Directory Structure

```
tests/
├── fuzz_afl.c          # AFL++ fuzz target
├── build_afl.sh        # Build script
├── run_afl.sh          # Run script
├── corpus/             # Seed inputs
│   ├── basic.txt
│   ├── complex.txt
│   ├── ipv4.txt
│   ├── ipv6.txt
│   └── ...
└── findings/           # AFL++ output (created when running)
    └── default/
        ├── crashes/    # Inputs that caused crashes
        ├── hangs/      # Inputs that caused timeouts
        └── queue/      # All interesting inputs found
```

## What Gets Tested

The fuzz target tests multiple parser functions:

- `url_parse()` - Main URL parser (with different flag combinations)
- `url_parse_ipv4()` - IPv4 address parser
- `url_parse_ipv6()` - IPv6 address parser
- `url_serialize()` - URL serialization
- `url_percent_decode()` - Percent-decoding
- `url_remove_white_space()` - Whitespace removal

## Analyzing Results

### Reproducing Crashes

If AFL++ finds a crash, you can reproduce it:

```bash
./fuzz_afl < findings/default/crashes/id:000000,*
```

Or use a debugger:

```bash
gdb ./fuzz_afl
(gdb) run < findings/default/crashes/id:000000,*
```

### Minimizing Crash Inputs

AFL++ can minimize crash inputs to their smallest form:

```bash
afl-tmin -i findings/default/crashes/id:000000,* -o minimized.txt -- ./fuzz_afl
```

### Viewing Statistics

While fuzzing is running, AFL++ displays real-time statistics:

- **Cycles done**: Number of times the fuzzer went through the queue
- **Corpus count**: Number of interesting test cases found
- **Crashes**: Number of unique crashes found
- **Exec speed**: Fuzzing throughput (executions per second)

## Advanced Usage

### Parallel Fuzzing

Run multiple AFL++ instances in parallel for better coverage:

```bash
# Terminal 1 (master)
afl-fuzz -i corpus/ -o findings/ -M fuzzer1 -- ./fuzz_afl

# Terminal 2 (secondary)
afl-fuzz -i corpus/ -o findings/ -S fuzzer2 -- ./fuzz_afl

# Terminal 3 (secondary)
afl-fuzz -i corpus/ -o findings/ -S fuzzer3 -- ./fuzz_afl
```

### Using Different Modes

AFL++ supports various mutation modes:

```bash
# Explore mode (default)
afl-fuzz -i corpus/ -o findings/ -- ./fuzz_afl

# Exploitation mode (focus on found crashes)
afl-fuzz -i corpus/ -o findings/ -p exploit -- ./fuzz_afl

# COE (Continue on Error) mode
afl-fuzz -i corpus/ -o findings/ -c -- ./fuzz_afl
```

### With AddressSanitizer (ASAN)

For better bug detection, rebuild with AddressSanitizer:

```bash
AFL_USE_ASAN=1 ./build_afl.sh
./run_afl.sh
```

Note: ASAN significantly slows down fuzzing but can detect memory issues that wouldn't cause crashes.

### With UndefinedBehaviorSanitizer (UBSAN)

To detect undefined behavior:

```bash
AFL_USE_UBSAN=1 ./build_afl.sh
./run_afl.sh
```

## Performance Tips

1. **Disable CPU frequency scaling:**
   ```bash
   echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
   ```

2. **Increase system limits:**
   ```bash
   sudo sysctl -w kernel.core_pattern=core
   sudo sysctl -w kernel.sched_child_runs_first=1
   ```

3. **Use persistent mode** (requires code changes to fuzz target)

4. **Use dictionary files** for better mutations:
   ```bash
   afl-fuzz -i corpus/ -o findings/ -x url.dict -- ./fuzz_afl
   ```

## Continuous Fuzzing

For long-term fuzzing campaigns:

```bash
# Run in screen/tmux
screen -S fuzzing
./run_afl.sh

# Detach: Ctrl+A, D
# Reattach: screen -r fuzzing
```

Or use a systemd service for continuous fuzzing.

## Troubleshooting

### "No instrumentation detected"

Make sure you built with an AFL++ compiler (afl-gcc, afl-clang-fast, etc.)

### "Suboptimal CPU scaling governor"

Run as suggested by AFL++ or set `AFL_SKIP_CPUFREQ=1`

### Low execution speed

- Close other programs
- Use afl-clang-fast instead of afl-gcc
- Consider using QEMU mode for uninstrumented binaries

### No new paths found

- Add more diverse seed inputs to corpus/
- Try different mutation strategies
- Consider parallel fuzzing

## References

- [AFL++ Documentation](https://github.com/AFLplusplus/AFLplusplus/tree/stable/docs)
- [Fuzzing Tutorial](https://github.com/google/fuzzing/blob/master/tutorial/libFuzzerTutorial.md)
- [OWASP Fuzzing Guide](https://owasp.org/www-community/Fuzzing)
