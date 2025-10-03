@echo off
REM Smart Tile Lighting System Build Script for Windows
REM This script builds both coordinator and node firmware

echo Smart Tile Lighting System Build Script
echo =======================================

REM Check if PlatformIO is installed
where pio >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] PlatformIO is not installed. Please install it first.
    exit /b 1
)

REM Build coordinator
echo [INFO] Building coordinator firmware...
cd ..\coordinator
pio run
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Coordinator build failed
    exit /b 1
)
echo [INFO] Coordinator build successful

REM Build node
echo [INFO] Building node firmware...
cd ..\node
pio run
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Node build failed
    exit /b 1
)
echo [INFO] Node build successful

echo [INFO] All builds completed successfully!
echo [INFO] Firmware files are located in:
echo [INFO]   Coordinator: coordinator\.pio\build\esp32-s3-devkitc-1\firmware.bin
echo [INFO]   Node: node\.pio\build\esp32-c3-devkitm-1\firmware.bin

REM Optional: Create release package
if "%1"=="--package" (
    echo [INFO] Creating release package...
    cd ..
    set RELEASE_DIR=smart_tile_release_%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%
    set RELEASE_DIR=%RELEASE_DIR: =0%
    mkdir "%RELEASE_DIR%"
    
    copy "coordinator\.pio\build\esp32-s3-devkitc-1\firmware.bin" "%RELEASE_DIR%\coordinator_firmware.bin"
    copy "node\.pio\build\esp32-c3-devkitm-1\firmware.bin" "%RELEASE_DIR%\node_firmware.bin"
    copy "ProductRequirementDocument.md" "%RELEASE_DIR%\"
    
    echo [INFO] Release package created: %RELEASE_DIR%
)

pause



