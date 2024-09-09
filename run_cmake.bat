@echo off
setlocal enabledelayedexpansion


cmake -S . -B build
@if %ERRORLEVEL% neq 0 (
pause
exit /b 1
)

start .\build   

endlocal