Module.preRun = () => {

    let stdinBuffer = [];
    const inputBox = document.querySelector("article input")
    const input = inputBox.value;
    inputBox.addEventListener("keydown", (event) => {
        if (event.key === "Enter" && !event.shiftKey) {
            event.preventDefault();
            inputBox.value = ""; // Clear the textarea
            stdinBuffer.push(...input + "\n"); // Add input to the buffer
          }
    });
    function stdin() {
        if (i < input.length) {
            var code = input.charCodeAt(i);
            ++i;
            return code;
          } else {
            return null;
          }
    };

    let stdoutBuffer = "";
    function stdout(code) {
      if (code === "\n".charCodeAt(0) && stdoutBuffer !== "") {
        document.querySelector("article section").textContent += stdoutBuffer + "\n";
        stdoutBuffer = "";
      } else {
        stdoutBuffer += String.fromCharCode(code);
      }
    }

    let stderrBuffer = "";
    function stderr(code) {
      if (code === "\n".charCodeAt(0) && stderrBuffer !== "") {
        document.querySelector("article section").textContent += stderrBuffer + "\n";
        stderrBuffer = "";
      } else {
        stderrBuffer += String.fromCharCode(code);
      }
    }
    FS.init(stdin, stdout, stderr);
}

