const readline = require('readline');
const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
});

Module.pending_fgets = [];
Module.pending_lines = [];

rl.on('line', (line) => {
    Module.pending_lines.push(line);
    if (Module.pending_fgets.length > 0 && Module.pending_lines.length > 0) {
        const resolver = Module.pending_fgets.shift();
        resolver(Module.pending_lines.shift());
    }
});
