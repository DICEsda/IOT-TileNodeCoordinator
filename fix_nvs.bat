@echo off
REM Complete NVS Fix Script for ESP32-S3
REM This script erases the flash and re-uploads the firmware

setlocal enabledelayedexpansion

echo ========================================
echo ESP32-S3 NVS Complete Fix
echo ========================================
echo.

REM Try to find COM port
for /f "tokens=1" %%a in ('wmic logicaldisk get name') do (
    if "%%a"=="C:" (
        echo Checking for ESP32-S3...
    )
)

echo.
echo Available COM ports (check Device Manager if yours isn't listed):
wmic logicaldisk list /format:list | find "Name" || (
    echo.
    echo *** Could not detect ports. Please specify manually. ***
    echo *** Look in Device Manager under "Ports (COM & LPT)" ***
    echo.
)

echo.
set /p PORT="Enter COM port (e.g., COM3, COM4, COM5): "

if "!PORT!"=="" (
    echo Error: No port specified
    exit /b 1
)

echo.
echo ========================================
echo Step 1: Erasing entire flash...
echo ========================================
echo.

python -m esptool --chip esp32s3 --port !PORT! erase_flash

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Flash erase failed!
    echo Please check:
    echo   - ESP32-S3 is connected to !PORT!
    echo   - Board is in bootloader mode (hold BOOT button while plugging in)
    echo   - esptool is installed (pip install esptool)
    pause
    exit /b 1
)

echo.
echo ========================================
echo Step 2: Building firmware...
echo ========================================
echo.

cd /d "%~dp0coordinator"
python -m platformio run

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Step 3: Uploading firmware...
echo ========================================
echo.

python -m platformio run --target upload --upload-port !PORT!

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Upload failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo ✓ SUCCESS! 
echo ========================================
echo.
echo The NVS fix has been applied!
echo.
echo Next steps:
echo   1. Open serial monitor at 115200 baud
echo   2. You should see: "✓ NVS initialized successfully"
echo   3. NO "nvs_open failed" errors
echo.
echo To view logs: Open Device Manager, find COM port, right-click Properties
echo (Or use Arduino IDE Serial Monitor)
echo.
pause
