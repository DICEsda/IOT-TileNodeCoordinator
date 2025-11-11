@echo off
setlocal enabledelayedexpansion
REM Quick Start Script for IOT Smart Tile System

echo ========================================
echo IOT SMART TILE SYSTEM - QUICK START
echo ========================================
echo.

REM Prefer Docker Compose v2 (`docker compose`); fall back to v1 (`docker-compose`)
set COMPOSE_CMD=docker compose
docker compose version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    docker-compose version >nul 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Docker Compose is not installed!
        echo Install Docker Desktop ^(which bundles Compose v2^) from https://www.docker.com/products/docker-desktop
        pause
        exit /b 1
    ) else (
        set COMPOSE_CMD=docker-compose
    )
)

REM Check if Docker is installed
docker --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Docker is not installed!
    echo Please install Docker Desktop from https://www.docker.com/products/docker-desktop
    pause
    exit /b 1
)

REM Check if Docker is running
docker ps >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Docker is not running!
    echo Please start Docker Desktop and try again.
    pause
    exit /b 1
)

echo [1/4] Checking environment configuration...
if not exist .env (
    echo Creating .env from template...
    copy .env.example .env
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Failed to create .env file!
        pause
        exit /b 1
    )
    echo.
    echo IMPORTANT: .env file created with default values.
    echo Edit .env file if you need to customize settings.
    echo.
) else (
    echo .env file already exists
)

echo.
echo [2/4] Building Docker images...
echo This may take 5-10 minutes on first run...
echo.
%COMPOSE_CMD% build
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    echo.
    echo Common solutions:
    echo   1. Check Docker Desktop is running
    echo   2. Ensure you have internet connection
    echo   3. Try: docker-compose down -v ^(clean volumes^)
    echo   4. Try: docker system prune ^(clean Docker cache^)
    echo.
    pause
    exit /b 1
)

echo.
echo [3/4] Starting services...
REM Do not recreate existing containers; proceed even if compose warns about unhealthy deps
set UP_FLAGS=--no-recreate
%COMPOSE_CMD% up -d %UP_FLAGS%
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: docker compose reported a non-zero exit ^(possibly due to health checks^). Continuing to wait for readiness...
)

echo.
echo [4/4] Waiting for services to be ready...
REM Wait for backend health (host + in-container fallback)
set RETRIES=30
set DELAY=3
REM Define PowerShell health check command outside loop to avoid parenthesis escaping issues
set "PS_HEALTH=try { $resp = Invoke-WebRequest -UseBasicParsing http://localhost:8000/health -TimeoutSec 2; if ($resp.StatusCode -eq 200) { exit 0 } else { exit 1 } } catch { exit 1 }"
for /L %%i in (1,1,%RETRIES%) do (
    powershell -NoLogo -NoProfile -Command "!PS_HEALTH!" >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        goto :ready
    )
    REM Fallback: check from inside the backend container
    %COMPOSE_CMD% exec -T backend sh -c "wget -q --tries=1 --spider http://localhost:8000/health" >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        goto :ready
    )
    echo Waiting for backend... ^(%%i/%RETRIES%^)
    timeout /t %DELAY% /nobreak >nul
)
echo ERROR: Backend didn't become healthy in time.
%COMPOSE_CMD% logs backend
pause
exit /b 1

:ready

echo.
echo ========================================
echo SYSTEM IS READY!
echo ========================================
echo.
echo Access Points:
echo   Frontend:  http://localhost:4200
echo   Backend:   http://localhost:8000
echo   MQTT:      mqtt://localhost:1883
echo   MongoDB:   mongodb://localhost:27017
echo.
echo Credentials (default):
echo   MQTT User: user1
echo   MQTT Pass: user1
echo   Mongo User: admin
echo   Mongo Pass: admin123
echo.
echo Useful Commands:
echo   View logs:      %COMPOSE_CMD% logs -f
echo   Stop system:    docker-stop.bat
echo   Restart:        %COMPOSE_CMD% restart
echo.
echo Opening frontend in browser...
timeout /t 3 /nobreak >nul
start http://localhost:4200

echo.
echo Press any key to view logs (CTRL+C to exit logs)...
pause >nul
%COMPOSE_CMD% logs -f
