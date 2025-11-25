@echo off
echo ============================================
echo Starting IoT Frontend
echo ============================================
echo.

cd IOT-Frontend-main\IOT-Frontend-main

echo Checking node_modules...
if not exist node_modules (
    echo Installing dependencies...
    call npm install
    if errorlevel 1 (
        echo.
        echo ERROR: npm install failed!
        pause
        exit /b 1
    )
)

echo.
echo Starting Angular dev server...
echo Frontend will be available at: http://localhost:4200
echo.
echo Press Ctrl+C to stop
echo.

call npm start

cd ..\..
