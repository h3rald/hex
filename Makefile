# Default Compiler and Flags
CC ?= gcc
CFLAGS ?= -Wall -Wextra -g
LDFLAGS ?=

hex: hex.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@
