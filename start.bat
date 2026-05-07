@echo off
setlocal

:: Check for administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [!] This script must be run as Administrator.
    pause
    exit /b
)

:: 1. Map the driver
echo [*] Mapping driver with kdmapper...
if exist "kdmapper.exe" (
    if exist "driver.sys" (
        kdmapper.exe driver.sys
    ) else if exist "Cs2WebRadar.sys" (
        kdmapper.exe Cs2WebRadar.sys
    ) else (
        echo [!] Driver file (driver.sys or Cs2WebRadar.sys) not found in the current directory.
        echo [*] Continuing anyway, usermode may use fallback memory provider...
    )
) else (
    echo [!] kdmapper.exe not found in the current directory.
    echo [*] Continuing anyway, usermode may use fallback memory provider...
)

:: 2. Start the webapp
echo [*] Starting webapp (WebSocket server and Vite dev server)...
if exist "webapp" (
    start "cs2_webradar - webapp" cmd /k "cd webapp && npm run dev"
) else (
    echo [!] Webapp directory not found!
)

:: 3. Start the usermode application
echo [*] Starting usermode application...
if exist "usermode\release\usermode.exe" (
    start "cs2_webradar - usermode" cmd /k "cd usermode\release && usermode.exe"
) else (
    echo [!] Usermode executable not found at usermode\release\usermode.exe
    echo [!] Make sure you have built the project in Visual Studio (Release/x64).
)

echo [*] All components started.
timeout /t 3 /nobreak >nul
exit
