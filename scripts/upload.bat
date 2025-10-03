@echo off
REM Smart Tile Lighting System Upload Script for Windows
REM This script uploads firmware to ESP32 devices

if "%1"=="" (
    echo Usage: %0 [coordinator^|node] [port]
    echo.
    echo Arguments:
    echo   coordinator  Upload coordinator firmware to ESP32-S3
    echo   node         Upload node firmware to ESP32-C3
    echo   port         Serial port (optional, auto-detected if not specified)
    echo.
    echo Examples:
    echo   %0 coordinator
    echo   %0 node COM3
    echo   %0 coordinator COM4
    exit /b 1
)

set DEVICE_TYPE=%1
set PORT=%2

REM Detect port if not specified
if "%PORT%"=="" (
    echo [INFO] Auto-detecting serial port...
    set PORT=auto
    echo [INFO] Using auto-detection
) else (
    echo [INFO] Using specified port: %PORT%
)

REM Upload based on device type
if "%DEVICE_TYPE%"=="coordinator" (
    echo [INFO] Uploading coordinator firmware...
    cd ..\coordinator
    
    if "%PORT%"=="auto" (
        pio run --target upload
    ) else (
        pio run --target upload --upload-port %PORT%
    )
    
    if %ERRORLEVEL% EQU 0 (
        echo [INFO] Coordinator firmware uploaded successfully!
        echo [INFO] Opening serial monitor...
        if "%PORT%"=="auto" (
            pio device monitor
        ) else (
            pio device monitor --port %PORT%
        )
    ) else (
        echo [ERROR] Coordinator upload failed!
        exit /b 1
    )
) else if "%DEVICE_TYPE%"=="node" (
    echo [INFO] Uploading node firmware...
    cd ..\node
    
    if "%PORT%"=="auto" (
        pio run --target upload
    ) else (
        pio run --target upload --upload-port %PORT%
    )
    
    if %ERRORLEVEL% EQU 0 (
        echo [INFO] Node firmware uploaded successfully!
        echo [INFO] Opening serial monitor...
        if "%PORT%"=="auto" (
            pio device monitor
        ) else (
            pio device monitor --port %PORT%
        )
    ) else (
        echo [ERROR] Node upload failed!
        exit /b 1
    )
) else (
    echo [ERROR] Invalid device type: %DEVICE_TYPE%
    echo Use 'coordinator' or 'node'
    exit /b 1
)

pause



