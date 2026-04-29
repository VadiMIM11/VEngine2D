@echo off

cmake --preset web-html
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build --preset web-html
if %errorlevel% neq 0 exit /b %errorlevel%
