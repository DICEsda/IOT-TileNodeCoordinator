@echo off
REM Quick fix script for coordinator not showing in frontend

echo ========================================
echo Coordinator Discovery Fix
echo ========================================
echo.

echo This script will:
echo 1. Rebuild coordinator firmware with telemetry payload fix
echo 2. Rebuild frontend with dynamic coordinator discovery
echo 3. Restart Docker services
echo.

set /p CHOICE="Continue? (y/n): "
if /i not "%CHOICE%"=="y" (
    echo Cancelled.
    exit /b 0
)

echo.
echo [1/4] Building coordinator firmware...
cd coordinator
call pio run -e esp32-s3-devkitc-1
if errorlevel 1 (
    echo ERROR: Coordinator build failed!
    cd ..
    pause
    exit /b 1
)
cd ..

echo.
echo [2/4] Coordinator firmware built successfully.
echo         To flash: cd coordinator && pio run -t upload -t monitor
echo.

echo [3/4] Rebuilding Docker services...
docker-compose build frontend backend

if errorlevel 1 (
    echo ERROR: Docker build failed!
    pause
    exit /b 1
)

echo.
echo [4/4] Restarting services...
docker-compose down
docker-compose up -d

echo.
echo ========================================
echo Fix Applied Successfully!
echo ========================================
echo.
echo Next steps:
echo 1. Flash coordinator: cd coordinator && pio run -t upload -t monitor
echo 2. Wait for coordinator to connect to MQTT
echo 3. Open frontend: http://localhost:4200
echo 4. Check Settings tab - coordinator should be discovered
echo.
echo To monitor:
echo   - Backend logs:  docker logs -f iot-backend
echo   - Frontend logs: docker logs -f iot-frontend
echo   - MQTT messages: docker exec iot-mosquitto mosquitto_sub -h localhost -t "#" -v
echo.
pause
