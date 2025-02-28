# Emscripten Game Project Setup

This project uses Emscripten to compile C/C++ code (with OpenGL ES) for the web. Follow the instructions below for setting up your environment for building and running the game on Windows.

---

## Requirements

### For Building
- **Windows OS**
- **Emscripten SDK**  
  Download and install the Emscripten SDK from [https://emscripten.org/docs/getting_started/downloads.html](https://emscripten.org/docs/getting_started/downloads.html).
- **Permanent Environment Variables**  
  After installing the SDK, set up the environment permanently by running the following command from the emsdk folder:

      emsdk activate latest --permanent

  This ensures that all necessary environment variables are available in every new terminal session.

### For Running
- **Windows OS**
- **Python 3**  
  Python 3 is required to run the local server. Download it from [https://www.python.org/downloads/](https://www.python.org/downloads/) if you haven't already.
- The project’s build artifacts (e.g., main.js and main.wasm) generated from the build process.

---

## Building the Project

1. **Ensure the SDK is set up permanently:**  
   Make sure you've run "emsdk activate latest --permanent" so your environment variables are configured correctly.

2. **Build the project:**  
   Simply run the provided `build.bat` file. You can do this by double-clicking it in Windows Explorer or by opening a Command Prompt in the project directory and typing:

       build.bat

   This batch file will compile your C/C++ source code using Emscripten and output the necessary files (e.g., main.js and main.wasm).

---

## Running the Project

Once the project is built, you can test it locally by running the provided `run.bat` file:

1. Open a Command Prompt in the project directory.
2. Run:

       run.bat

   This batch file starts a local HTTP server (for example, using emrun or Python’s http.server) on port 8080, serving files from the designated directory (e.g., public/).

3. Open your web browser and navigate to:

       http://localhost:8080

   You should see the game running in your browser.

---

## Additional Notes

- **Permanent Environment Setup:**  
  Setting the environment variables permanently using "emsdk activate latest --permanent" is recommended so you don't have to run the activation command each time you open a new terminal.

- **Python 3 Requirement:**  
  Python 3 is needed for running the local server. Verify your installation by running "python --version" or "python3 --version" from the Command Prompt.

- **Batch Files:**  
  - `build.bat` handles the build/compilation process using Emscripten.
  - `run.bat` launches the local server to test the build output.

Happy coding and enjoy building your game!
