@echo off
setlocal enabledelayedexpansion


cmake -S . -B build
@if %ERRORLEVEL% neq 0 (
pause
exit /b 1
)


:: set "solutionPath=build\ZGameFrame.sln"
:: set "foundPID="



:: Check for the specified solution in running Visual Studio instances
:: echo Checking for running Visual Studio instances...
:: for /f "tokens=3,* delims=," %%a in ('wmic process where "name='devenv.exe'" get ProcessId^,CommandLine /format:csv') do (
::         set "foundPID=%%a"
:: )



:: if not defined foundPID ( goto notfound )
::     echo The PID for the Visual Studio instance with the solution "%solutionPath%" is %foundPID%.


    
REM Construct the PowerShell command to bring Visual Studio to the front and activate it
:: set "psCommand=Add-Type -Name Util -Namespace Win32 -MemberDefinition '[DllImport(\"user32.dll\")] public static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow); [DllImport(\"user32.dll\")] public static extern bool SetForegroundWindow(IntPtr hWnd);'; `
::     Add-Type -TypeDefinition $Util; `
::     $process = Get-Process -Id %foundPID%; `
::     if ($process) { `
::         $handle = $process.MainWindowHandle; `
::         [Win32.Util]::ShowWindowAsync($handle, 1); `
::         Start-Sleep -Seconds 1; `
::         [Win32.Util]::SetForegroundWindow($handle); `
::     } else { `
::         Write-Host 'Visual Studio process with PID %foundPID% not found.'; `
::     }"

REM Execute the PowerShell command
:: powershell -command "& { !psCommand! }"


exit /b 0



:notfound
    echo No instance of Visual Studio with the solution "%solutionPath%" was found.
    echo Starting Visual Studio with the solution...
    
    
    
    echo start "" "%solutionPath%"

endlocal