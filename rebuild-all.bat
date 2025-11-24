@echo off
echo ========================================
echo IOT SmartTile System - Rebuild All
echo ========================================
echo.

echo [1/3] Stopping existing containers...
docker-compose down
echo.

echo [2/3] Building new containers...
docker-compose build --no-cache
echo.

echo [3/3] Starting all services...
docker-compose up -d
echo.

echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Services running:
docker-compose ps
echo.
echo Frontend: http://localhost:4200
echo Backend:  http://localhost:8000
echo MQTT:     tcp://localhost:1883
echo.
echo View logs: docker-compose logs -f
echo.
pause
