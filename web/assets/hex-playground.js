let stdinBuffer = [];
const inputBox = document.querySelector("article input")
inputBox.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        const input = inputBox.value;
        inputBox.value = ""; // Clear the textarea
        stdinBuffer.push(...input + "\n"); // Add input to the buffer
      }
});

Module.print = (text) => {
    document.querySelector("article section").textContent += text + "\n";
};
Module.printErr = (text) => {
    document.querySelector("article section").textContent += text + "\n";
};
Module.stdin = () => {
    return stdinBuffer.length > 0 ? stdinBuffer.shift().charCodeAt(0) : null;
};

