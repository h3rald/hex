CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS =

hex: hex.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o hex

web/assets/hex.wasm: hex.c
	 emcc -O2 -sASYNCIFY -sEXPORTED_RUNTIME_METHODS=stringToUTF8 hex.c -o web/assets/hex.js --pre-js web/assets/hex-playground.js

ape: hex.c
	cosmocc $(CFLAGS) $(LDFLAGS) $< -o hex

.PHONY: wasm
wasm: web/assets/hex.wasm

.PHONY: clean
clean:
	rm hex

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
dweb: wasm
	./hex -d web.hex
