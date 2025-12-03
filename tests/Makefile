.PHONY: all clean

ifeq ($(shell uname -s),Windows_NT)
	EXT = .exe
else
	EXT =
endif

all:
	gcc -o test$(EXT) test.c ../url.c cJSON.c -Wall -Wextra -ggdb

clean:
	rm test test.exe
