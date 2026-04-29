@echo off

cmake --preset web-js
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build --preset web-js
if %errorlevel% neq 0 exit /b %errorlevel%
