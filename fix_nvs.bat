@echo off
REM Complete NVS Fix Script for ESP32-S3
REM This script erases the flash and re-uploads the firmware

setlocal enabledelayedexpansion

echo ========================================
echo ESP32-S3 NVS Complete Fix
echo ========================================
echo.

REM Try to list available COM ports using PowerShell (WMIC is deprecated)
echo Detecting available COM ports...
powershell -NoProfile -Command " $ports = Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue; if ($ports) { Write-Host ''; Write-Host 'Available COM ports (check Device Manager if yours isn''t listed):'; foreach ($p in $ports) { Write-Host ('  {0} - {1}' -f $p.DeviceID, $p.Name) } } else { exit 1 } "

if errorlevel 1 (
    echo.
    echo *** Could not auto-detect COM ports. ***
    echo *** Open "Device Manager" -> "Ports (COM & LPT)" to find yours. ***
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

if errorlevel 1 (
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

if errorlevel 1 (
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

if errorlevel 1 (
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
