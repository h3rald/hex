CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS =

hex: src/hex.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o hex

web/assets/hex.wasm: src/hex.c
	 emcc -O2 -sASYNCIFY -DBROWSER -sEXPORTED_RUNTIME_METHODS=stringToUTF8 src/hex.c -o web/assets/hex.js --pre-js web/assets/hex-playground.js

hex.wasm: src/hex.c
	 emcc -O2 -sASYNCIFY -sEXPORTED_RUNTIME_METHODS=stringToUTF8 src/hex.c -o hex.js --pre-js src/hex.node.js

ape: src/hex.c
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
	./hex scripts/test.hex

.PHONY: web
web: playground
	./hex scripts/web.hex
