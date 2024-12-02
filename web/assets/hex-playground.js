


Module.pending_fgets = [];
Module.pending_chars = [];
Module.pending_lines = [];
Module.print = (text) => {
  document.querySelector("article section section").textContent += text + '\n';
};

Module.printErr = (text) => {
  document.querySelector("article section section").textContent += text + '\n';
};

const inputBox = document.querySelector("article input");

inputBox.addEventListener("keydown", (e) => {
  if (e.key === 'Enter') {
    e.preventDefault();
    Module.pending_lines.push(Module.pending_chars.join(''));
    Module.pending_chars = [];
    inputBox.value = '';
  } else if (e.key.length === 1) {
    Module.pending_chars.push(e.key);
  }
  if (Module.pending_fgets.length > 0 && Module.pending_lines.length > 0) {
    let resolver = Module.pending_fgets.shift();
    resolver(Module.pending_lines.shift());
  }
});

