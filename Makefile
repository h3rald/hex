CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS =

.PHONY: wasm, playground, clean, test, web

hex: src/hex.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o hex

src/hex.c:
	sh scripts/amalgamate.sh

web/assets/hex.wasm: src/hex.c
	 emcc -O2 -sASYNCIFY -DBROWSER -sEXPORTED_RUNTIME_METHODS=stringToUTF8 src/hex.c -o web/assets/hex.js --pre-js web/assets/hex-playground.js

hex.wasm: src/hex.c
	 emcc -O2 -sASYNCIFY -sEXPORTED_RUNTIME_METHODS=stringToUTF8 src/hex.c -o hex.js --pre-js src/hex.node.js

ape: src/hex.c
	cosmocc $(CFLAGS) $(LDFLAGS) $< -o hex

wasm: hex.wasm

playground: web/assets/hex.wasm

clean:
	rm -f src/hex.c
	rm -f hex
	rm -f hex.exe
	rm -f hex.js
	rm -f hex.wasm

test:
	./hex scripts/test.hex

web: playground
	./hex scripts/web.hex
