Module.pending_fgets = [];
Module.pending_lines = [];
const inputBox = document.querySelector("article input");
const textarea = document.querySelector("article textarea");
const upload = document.getElementById("upload");
const eval = document.getElementById("evaluate");
const outputBox = document.querySelector("article section section");

eval.style.display = "none";
hide.style.display = "none";
textarea.style.display = "none";

Module.print = (text) => {
  outputBox.textContent += text + "\n";
};

Module.printErr = (text) => {
  outputBox.textContent += text + "\n";
};

eval.addEventListener("click", (e) => {
  const data = textarea.value.replace(/^#!.*?\n/, '').replace(/;.*?\n/g, ' ').replace(/#|.*?|#/mg, '').replace(/\n/mg, ' ');
  Module.pending_lines.push(data);
    let resolver = Module.pending_fgets.shift();
    resolver(Module.pending_lines.shift());
    textarea.value = '';
    textarea.style.display = "none";
    eval.style.display = "none";
    upload.style.display = "block";
    hide.style.display = "none";
});

upload.addEventListener("click", () => {
  upload.style.display = "none";
  textarea.style.display = "flex";
  eval.style.display = "block";
  hide.style.display = "block";
});
hide.addEventListener("click", () => {
  textarea.style.display = "none";
  upload.style.display = "block";
  eval.style.display = "none";
  hide.style.display = "none";
});


inputBox.addEventListener("keydown", (e) => {
  if (e.key === "Enter") {
    e.preventDefault();
    Module.pending_lines.push(inputBox.value);
    outputBox.textContent += "> " + inputBox.value + "\n";
    inputBox.value = '';
  }
  if (Module.pending_fgets.length > 0 && Module.pending_lines.length > 0) {
    let resolver = Module.pending_fgets.shift();
    resolver(Module.pending_lines.shift());
  }
});

