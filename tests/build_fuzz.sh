#!/bin/bash
gcc -o fuzz.out fuzz.c ../url.c -Wall -Wextra -ggdb -O2
