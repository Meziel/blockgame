@echo off
emcc src/main.cpp -sWASM=1 -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -o public/bin/main.js
echo Build complete!
pause
