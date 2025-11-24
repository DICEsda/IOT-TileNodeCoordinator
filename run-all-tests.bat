@echo off
setlocal enabledelayedexpansion

echo ==================================
echo IOT-TileNodeCoordinator Test Suite
echo ==================================
echo.

set BACKEND_PASSED=0
set FRONTEND_PASSED=0
set FIRMWARE_PASSED=0

REM Backend Tests
echo === Running Backend Tests ===
cd IOT-Backend-main\IOT-Backend-main
go test -v -race -coverprofile=coverage.txt -covermode=atomic ./...
if %ERRORLEVEL% EQU 0 (
    echo [32m✓ Backend tests passed[0m
    set BACKEND_PASSED=1
    go tool cover -func=coverage.txt | findstr /R "total:"
) else (
    echo [31m✗ Backend tests failed[0m
)
cd ..\..
echo.

REM Frontend Tests
echo === Running Frontend Tests ===
cd IOT-Frontend-main\IOT-Frontend-main
call npm test -- --watch=false --browsers=ChromeHeadless --code-coverage
if %ERRORLEVEL% EQU 0 (
    echo [32m✓ Frontend tests passed[0m
    set FRONTEND_PASSED=1
) else (
    echo [31m✗ Frontend tests failed[0m
)
cd ..\..
echo.

REM Coordinator Firmware Tests
echo === Running Coordinator Firmware Tests ===
cd coordinator
pio test -e native
if %ERRORLEVEL% EQU 0 (
    echo [32m✓ Coordinator tests passed[0m
    set FIRMWARE_PASSED=1
) else (
    echo [31m✗ Coordinator tests failed[0m
)
cd ..
echo.

REM Node Firmware Tests
echo === Running Node Firmware Tests ===
cd node
pio test -e native
if %ERRORLEVEL% EQU 0 (
    echo [32m✓ Node tests passed[0m
) else (
    echo [31m✗ Node tests failed[0m
    set FIRMWARE_PASSED=0
)
cd ..
echo.

REM Summary
echo ==================================
echo Test Summary
echo ==================================
if %BACKEND_PASSED% EQU 1 (
    echo [32m✓ Backend: PASSED[0m
) else (
    echo [31m✗ Backend: FAILED[0m
)

if %FRONTEND_PASSED% EQU 1 (
    echo [32m✓ Frontend: PASSED[0m
) else (
    echo [31m✗ Frontend: FAILED[0m
)

if %FIRMWARE_PASSED% EQU 1 (
    echo [32m✓ Firmware: PASSED[0m
) else (
    echo [31m✗ Firmware: FAILED[0m
)
echo.

REM Exit with appropriate code
if %BACKEND_PASSED% EQU 1 if %FRONTEND_PASSED% EQU 1 if %FIRMWARE_PASSED% EQU 1 (
    echo [32m🎉 All tests passed![0m
    exit /b 0
) else (
    echo [31m❌ Some tests failed[0m
    exit /b 1
)
