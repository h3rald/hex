CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS =

hex: hex.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o hex

web/assets/hex.wasm: hex.c
	 emcc -O2 -sASYNCIFY -DBROWSER -sEXPORTED_RUNTIME_METHODS=stringToUTF8 hex.c -o web/assets/hex.js --pre-js web/assets/hex-playground.js

hex.wasm: hex.c
	 emcc -O2 hex.c -o hex.js

ape: hex.c
	cosmocc $(CFLAGS) $(LDFLAGS) $< -o hex

.PHONY: wasm
wasm: hex.wasm

.PHONY: playground
playground: web/assets/hex.wasm

.PHONY: clean
clean:
	rm -f hex
	rm -f hex.exe
	rm -f hex.js
	rm -f hex.wasm

.PHONY: test
test:
	./hex test.hex

.PHONY: dtest
dtest:
	./hex -d test.hex

.PHONY: web
web: playground
	./hex web.hex

.PHONY: dweb
dweb: playground
	./hex -d web.hex
