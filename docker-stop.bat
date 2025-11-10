@echo off
REM Docker Stop Script for Windows

echo ================================
echo IOT Smart Tile System - Stopping Services
echo ================================

echo.
REM Prefer docker compose v2
set COMPOSE_CMD=docker compose
docker compose version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    docker-compose version >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        set COMPOSE_CMD=docker-compose
    ) else (
        echo ERROR: Docker Compose not found. Install Docker Desktop.
        pause
        exit /b 1
    )
)

echo Stopping all services...
%COMPOSE_CMD% down

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Failed to stop services!
    pause
    exit /b 1
)

echo.
echo ================================
echo All services stopped successfully!
echo ================================
echo.
pause
