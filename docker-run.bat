@echo off
REM Docker Run Script for Windows

echo ================================
echo IOT Smart Tile System - Starting Services
echo ================================

REM Check if .env exists
if not exist .env (
    echo ERROR: .env file not found!
    echo Please run docker-build.bat first or copy .env.example to .env
    pause
    exit /b 1
)

echo.
echo Starting all services...
docker-compose up -d

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Failed to start services!
    pause
    exit /b 1
)

echo.
echo ================================
echo Services started successfully!
echo ================================
echo.
echo Access points:
echo - Frontend:  http://localhost:4200
echo - Backend:   http://localhost:8000
echo - MQTT:      mqtt://localhost:1883
echo - MongoDB:   mongodb://localhost:27017
echo.
echo To view logs: docker-compose logs -f
echo To stop:      docker-compose down
echo.
pause
