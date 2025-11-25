@echo off
cls
echo ============================================
echo STARTING BACKEND (Direct Method)
echo ============================================
echo.
echo This will start the backend directly without Docker
echo Backend will run on: http://localhost:8000
echo.

REM Kill any existing backend process on port 8000
echo [1/3] Checking for existing processes on port 8000...
for /f "tokens=5" %%a in ('netstat -ano ^| findstr :8000') do (
    echo Found process %%a on port 8000, killing it...
    taskkill /PID %%a /F >nul 2>&1
)

echo [2/3] Building backend...
cd IOT-Backend-main\IOT-Backend-main

REM Check if Go is installed
where go >nul 2>&1
if errorlevel 1 (
    echo.
    echo ERROR: Go is not installed!
    echo Please install Go from: https://go.dev/dl/
    echo.
    pause
    exit /b 1
)

REM Build the backend
go build -o iot.exe cmd\iot\main.go
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    echo Check the error messages above.
    pause
    exit /b 1
)

echo [3/3] Starting backend...
echo.
echo ============================================
echo Backend is starting!
echo ============================================
echo API:       http://localhost:8000
echo Health:    http://localhost:8000/health
echo WebSocket: ws://localhost:8000/ws
echo.
echo Press Ctrl+C to stop the backend
echo ============================================
echo.

REM Set environment variables for local MongoDB and MQTT
set MONGO_URI=mongodb://localhost:27017
set MQTT_BROKER=tcp://localhost:1883

REM Start the backend
iot.exe

cd ..\..
