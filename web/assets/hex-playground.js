Module.pending_fgets = [];
Module.pending_lines = [];
const prompt = document.getElementById("input");
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

const processHexCode = (hexString) => {
  return hexString.replace(/^#!.*?\r?\n/, '').replace(/;.*?\r?\n/gm, ' ').replace(/#\|.*?\|#/gm, '').replace(/\r?\n/mg, ' ');
};

// Function to load and evaluate utils.hex when the playground starts
const loadUtilities = async () => {
  try {
    const sourceResponse = await fetch('/assets/utils.hex');
    if (sourceResponse.ok) {
      const utilsSourceCode = await sourceResponse.text();
      const processedCode = processHexCode(utilsSourceCode);
      // Try to add utils to pending_lines immediately
      Module.pending_lines.push(processedCode);
      Module.pending_lines.push('"lib/utils.hex loaded." puts');
      // Also try to trigger processing if there are waiting fgets
      if (Module.pending_fgets && Module.pending_fgets.length > 0) {
        let resolver = Module.pending_fgets.shift();
        resolver(Module.pending_lines.shift());
      }

    } else {
      console.warn('Failed to load utils.hex source:', sourceResponse.status);
    }
  } catch (error) {
    console.warn('Error loading utils:', error);
  }
};

const originalOnRuntimeInitialized = Module.onRuntimeInitialized;

Module.onRuntimeInitialized = () => {
  if (originalOnRuntimeInitialized) {
    originalOnRuntimeInitialized();
  }
  loadUtilities();
}

eval.addEventListener("click", (e) => {
  const data = processHexCode(textarea.value);
  Module.pending_lines.push(data);
  let resolver = Module.pending_fgets.shift();
  resolver(Module.pending_lines.shift());
  textarea.value = '';
  textarea.style.display = "none";
  eval.style.display = "none";
  upload.style.display = "block";
  hide.style.display = "none";
  prompt.style.display = "flex";
});

upload.addEventListener("click", () => {
  upload.style.display = "none";
  textarea.style.display = "flex";
  eval.style.display = "block";
  hide.style.display = "block";
  prompt.style.display = "none";
});

hide.addEventListener("click", () => {
  textarea.style.display = "none";
  upload.style.display = "block";
  eval.style.display = "none";
  hide.style.display = "none";
  prompt.style.display = "flex";
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

