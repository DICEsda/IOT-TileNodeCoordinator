@echo off
echo ========================================
echo Final Coordinator Fix - Complete Rebuild
echo ========================================
echo.

echo Backend is working - coordinator telemetry is being saved!
echo Now we need to rebuild frontend with the latest fixes.
echo.

pause

echo.
echo [1/2] Rebuilding frontend with coordinator discovery fix...
docker-compose build frontend
if errorlevel 1 (
    echo ERROR: Frontend build failed!
    pause
    exit /b 1
)

echo.
echo [2/2] Restarting frontend container...
docker-compose up -d frontend

echo.
echo ========================================
echo Done!
echo ========================================
echo.
echo Now do this:
echo 1. Open http://localhost:4200
echo 2. Hard refresh: Ctrl + Shift + R
echo 3. Open console (F12)
echo 4. Go to Settings tab
echo.
echo You should see:
echo   [Settings] Discovered coordinator ID: 74:4D:BD:AB:A9:F4
echo.
echo If you still see "No coordinator telemetry received":
echo   - Check MQTT is publishing to WebSocket port 9001
echo   - Restart Mosquitto: docker-compose restart mosquitto
echo.
pause
