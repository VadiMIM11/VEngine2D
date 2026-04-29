@echo off
echo Cleaning CMake and Emscripten caches...

REM Remove all build directories
echo Removing build directories...
if exist build rmdir /s /q build
if exist build_web rmdir /s /q build_web
if exist build_web_html rmdir /s /q build_web_html
if exist build_web_js rmdir /s /q build_web_js

REM Remove CMake cache files
echo Removing CMake cache files...
if exist CMakeCache.txt del /q CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles

echo.
echo Cleanup complete!
echo You can now run CMake preset configure commands to rebuild.
