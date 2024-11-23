# Default Compiler and Flags
CC ?= gcc
CFLAGS ?= -Wall -Wextra -g
LDFLAGS ?=

main: hex.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c hex.c -o hex
