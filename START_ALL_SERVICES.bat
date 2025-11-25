@echo off
cls
echo ============================================
echo STARTING ALL SERVICES
echo ============================================
echo.

REM Check if Docker is available
where docker >nul 2>&1
if errorlevel 1 (
    echo ERROR: Docker is not installed or not in PATH
    echo Please install Docker Desktop from: https://www.docker.com/products/docker-desktop
    pause
    exit /b 1
)

echo [1/5] Starting MongoDB...
docker ps | findstr mongodb >nul
if errorlevel 1 (
    echo Starting new MongoDB container...
    docker run -d --name mongodb -p 27017:27017 mongo:latest
) else (
    echo MongoDB container already running
)
timeout /t 3 /nobreak >nul

echo.
echo [2/5] Starting MQTT Broker...
docker ps | findstr mosquitto >nul
if errorlevel 1 (
    echo Starting new Mosquitto container...
    docker run -d --name mosquitto -p 1883:1883 -p 9001:9001 eclipse-mosquitto:latest
) else (
    echo Mosquitto container already running
)
timeout /t 3 /nobreak >nul

echo.
echo [3/5] Waiting for services to be ready...
timeout /t 5 /nobreak >nul

echo.
echo [4/5] Starting Backend in new window...
cd IOT-Backend-main\IOT-Backend-main
start "IoT Backend" cmd /k "echo Starting Backend... && go run cmd\iot\main.go"
cd ..\..
echo Backend starting in separate window...
timeout /t 5 /nobreak >nul

echo.
echo [5/5] Starting Frontend in new window...
cd IOT-Frontend-main\IOT-Frontend-main
start "IoT Frontend" cmd /k "echo Starting Frontend... && npm start"
cd ..\..
echo Frontend starting in separate window...

echo.
echo ============================================
echo ALL SERVICES STARTED!
echo ============================================
echo.
echo Services:
echo   MongoDB:  mongodb://localhost:27017
echo   MQTT:     mqtt://localhost:1883
echo   Backend:  http://localhost:8000
echo   Frontend: http://localhost:4200
echo.
echo Check the opened windows for service logs.
echo Wait ~30 seconds for all services to be ready.
echo.
echo Then open: http://localhost:4200
echo.
pause
