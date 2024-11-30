# Default Compiler and Flags
CC ?= gcc
CFLAGS ?= -Wall -Wextra -g
LDFLAGS ?=

hex: hex.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

.PHONY: clean
clean:
	rm hex

.PHONY: test
test:
	./hex tests.hex

.PHONY: dtest
dtest:
	./hex -d tests.hex

.PHONY: web
web:
	./hex web.hex

.PHONY: dweb
dweb:
	./hex -d web.hex
