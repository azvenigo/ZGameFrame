cmake -S SandboxApps\. -B build
@if %ERRORLEVEL% == 0 (start build) ELSE (pause)