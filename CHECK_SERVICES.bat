@echo off
cls
echo ============================================
echo CHECKING REQUIRED SERVICES
echo ============================================
echo.

echo [1/4] Checking MongoDB (port 27017)...
netstat -ano | findstr :27017 >nul
if errorlevel 1 (
    echo ✗ MongoDB is NOT running on port 27017
    echo   Start with: docker run -d -p 27017:27017 mongo
) else (
    echo ✓ MongoDB is running
)

echo.
echo [2/4] Checking MQTT Broker (port 1883)...
netstat -ano | findstr :1883 >nul
if errorlevel 1 (
    echo ✗ MQTT is NOT running on port 1883
    echo   Start with: docker run -d -p 1883:1883 eclipse-mosquitto
) else (
    echo ✓ MQTT Broker is running
)

echo.
echo [3/4] Checking Backend (port 8000)...
netstat -ano | findstr :8000 >nul
if errorlevel 1 (
    echo ✗ Backend is NOT running on port 8000
    echo   Start with: START_BACKEND_NOW.bat
) else (
    echo ✓ Backend is running
)

echo.
echo [4/4] Checking Frontend (port 4200)...
netstat -ano | findstr :4200 >nul
if errorlevel 1 (
    echo ✗ Frontend is NOT running on port 4200
    echo   Start with: start-frontend.bat
) else (
    echo ✓ Frontend is running
)

echo.
echo ============================================
echo SUMMARY
echo ============================================
echo.
echo If MongoDB or MQTT are not running, start them with:
echo   docker run -d -p 27017:27017 --name mongodb mongo
echo   docker run -d -p 1883:1883 --name mosquitto eclipse-mosquitto
echo.
echo Then start the backend with:
echo   START_BACKEND_NOW.bat
echo.
pause
