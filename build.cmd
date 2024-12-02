gcc -Wall -Wextra -g hex.c -o hex.exe
emcc -sASYNCIFY -sEXPORTED_RUNTIME_METHODS=stringToUTF8 hex.c -o web/assets/hex.js --pre-js web/assets/hex-playground.js
