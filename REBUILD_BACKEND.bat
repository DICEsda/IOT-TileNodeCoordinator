@echo off
echo ========================================
echo Rebuilding Backend with Fixed Dependencies
echo ========================================

cd /d "%~dp0"

echo.
echo [1/3] Stopping backend container...
docker compose stop backend

echo.
echo [2/3] Rebuilding backend...
docker compose build backend

echo.
echo [3/3] Starting backend...
docker compose up -d backend

echo.
echo Waiting for backend to start...
timeout /t 5 /nobreak > nul

echo.
echo Backend logs:
docker compose logs backend --tail 30

echo.
echo ========================================
echo Done! Backend should now be running on http://localhost:8000
echo Check if it's healthy: curl http://localhost:8000/health
echo ========================================
pause
