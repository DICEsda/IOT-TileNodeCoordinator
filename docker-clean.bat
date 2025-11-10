@echo off
REM Docker Cleanup Script for IOT Smart Tile System

echo ========================================
echo IOT SMART TILE SYSTEM - DOCKER CLEANUP
echo ========================================
echo.

echo WARNING: This will remove all containers, volumes, and cached images!
echo Press CTRL+C to cancel or any key to continue...
pause >nul

echo.
echo [1/4] Stopping all containers...
docker-compose down

echo.
echo [2/4] Removing volumes...
docker-compose down -v

echo.
echo [3/4] Cleaning Docker system...
docker system prune -f

echo.
echo [4/4] Removing dangling images...
docker image prune -f

echo.
echo ========================================
echo CLEANUP COMPLETE!
echo ========================================
echo.
echo You can now run quick-start.bat again
pause
