CC ?= gcc
CFLAGS ?= -Wall -Wextra -g
LDFLAGS ?=

hex: hex.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

wasm: hex.c
	 emcc -sASYNCIFY -sEXPORTED_RUNTIME_METHODS=stringToUTF8 hex.c -o web/assets/hex.js --pre-js web/assets/hex-playground.js


.PHONY: clean
clean:
	rm hex
	rm -r web/out/

.PHONY: test
test:
	./hex test.hex

.PHONY: dtest
dtest:
	./hex -d test.hex

.PHONY: web
web: wasm
	./hex web.hex

.PHONY: dweb
dweb:
	./hex -d web.hex
