@echo off
echo ========================================
echo Starting IOT Smart Tile Docker Services
echo ========================================
echo.

REM Check if Docker is running
docker ps >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Docker is not running!
    echo Please start Docker Desktop and try again.
    pause
    exit /b 1
)

echo [1/4] Checking .env file...
if not exist .env (
    echo Creating .env from template...
    copy .env.example .env
    echo .env file created!
    echo.
    echo IMPORTANT: Edit .env if you need to customize settings.
    echo.
)

echo [2/4] Building Docker images...
echo This may take 5-10 minutes on first run...
echo.
docker-compose build
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo [3/4] Starting all services...
docker-compose up -d
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to start services!
    pause
    exit /b 1
)

echo.
echo [4/4] Checking service status...
timeout /t 5 /nobreak >nul
docker-compose ps

echo.
echo ========================================
echo SERVICES STARTED!
echo ========================================
echo.
echo Access Points:
echo   Frontend:  http://localhost:4200
echo   Backend:   http://localhost:8000
echo   MQTT:      mqtt://localhost:1883
echo   MongoDB:   mongodb://localhost:27017
echo.
echo Credentials:
echo   MQTT:    user1 / user1
echo   MongoDB: admin / admin123
echo.
echo Useful Commands:
echo   View logs:  docker-compose logs -f
echo   Stop:       docker-compose down
echo   Restart:    docker-compose restart
echo.
echo Opening frontend in browser...
timeout /t 3 /nobreak >nul
start http://localhost:4200

echo.
echo Press any key to view logs (CTRL+C to exit)...
pause >nul
docker-compose logs -f
