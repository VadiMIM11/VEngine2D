@echo off

cmake --preset win-release
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build --preset win-release
if %errorlevel% neq 0 exit /b %errorlevel%