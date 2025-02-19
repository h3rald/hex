CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS =

.PHONY: wasm, playground, clean

hex: src/hex.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o hex

src/hex.c: src/hex.h src/error.c src/doc.c src/utils.c src/interpreter.c src/main.c src/parser.c src/registry.c src/stack.c src/symbols.c src/vm.c src/symboltable.c src/opcodes.c
	bash scripts/amalgamate.sh

web/assets/hex.wasm: src/hex.c web/assets/hex-playground.js
	 emcc -O2 -sASYNCIFY -DBROWSER -sEXPORTED_RUNTIME_METHODS=stringToUTF8 src/hex.c -o web/assets/hex.js --pre-js web/assets/hex-playground.js

hex.wasm: src/hex.c src/hex.node.js
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

test: hex
	./hex -b scripts/test.hex
	./hex scripts/test.hbx

web: playground hex
	./hex scripts/web.hex
