Module.pending_fgets = [];
Module.pending_lines = [];
const inputBox = document.querySelector("article input");
const outputBox = document.querySelector("article section section");
Module.print = (text) => {
  outputBox.textContent += text + "\n";
};

Module.printErr = (text) => {
  outputBox.textContent += text + "\n";
};


inputBox.addEventListener("keydown", (e) => {
  if (e.key === "Enter") {
    e.preventDefault();
    Module.pending_lines.push(inputBox.value);
    outputBox.textContent += "> " + inputBox.value + "\n";
    inputBox.value = '';
  } else if (e.key.length === 1) {
    Module.pending_chars.push(e.key);
  }
  if (Module.pending_fgets.length > 0 && Module.pending_lines.length > 0) {
    let resolver = Module.pending_fgets.shift();
    resolver(Module.pending_lines.shift());
  }
});

