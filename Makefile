# Default Compiler and Flags
CC ?= gcc
CFLAGS ?= -Wall -Wextra -g
LDFLAGS ?=

hex: hex.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

.PHONY: clean, test, debug-test
clean:
	rm hex
test:
	./hex tests.hex
dtest:
	./hex -d tests.hex
