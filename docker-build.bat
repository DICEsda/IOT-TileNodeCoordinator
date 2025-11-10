@echo off
REM Docker Build and Run Script for Windows

echo ================================
echo IOT Smart Tile System - Docker Build
echo ================================

REM Check if .env exists
if not exist .env (
    echo Creating .env from .env.example...
    copy .env.example .env
)

echo.
echo Building Docker images...
docker-compose build

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Docker build failed!
    pause
    exit /b 1
)

echo.
echo ================================
echo Build completed successfully!
echo ================================
echo.
echo To start the services, run: docker-run.bat
echo.
pause
