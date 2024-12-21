Module.pending_fgets = [];
Module.pending_lines = [];
const inputBox = document.querySelector("article textarea");
const btn = document.querySelector("article button");
const outputBox = document.querySelector("article section section");
Module.print = (text) => {
  outputBox.textContent += text + "\n";
};

Module.printErr = (text) => {
  outputBox.textContent += text + "\n";
};

btn.addEventListener("click", () => {
    Module.pending_lines.push(inputBox.value);
    outputBox.textContent += "> " + inputBox.value + "\n";
    inputBox.value = '';
});

inputBox.addEventListener("keydown", (e) => {
  if (Module.pending_fgets.length > 0 && Module.pending_lines.length > 0) {
    let resolver = Module.pending_fgets.shift();
    resolver(Module.pending_lines.shift());
  }
});

