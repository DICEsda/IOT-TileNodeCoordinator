@echo off
echo ============================================
echo Starting IoT Backend
echo ============================================
echo.

cd IOT-Backend-main\IOT-Backend-main

echo Checking if backend binary exists...
if not exist iot.exe (
    echo Building backend...
    go build -o iot.exe cmd\iot\main.go
    if errorlevel 1 (
        echo.
        echo ERROR: Build failed!
        echo Please ensure Go is installed and dependencies are available.
        pause
        exit /b 1
    )
    echo Build successful!
)

echo.
echo Starting backend on port 8000...
echo Backend will be available at: http://localhost:8000
echo Health check: http://localhost:8000/health
echo WebSocket: ws://localhost:8000/ws
echo.
echo Press Ctrl+C to stop
echo.

iot.exe

cd ..\..
