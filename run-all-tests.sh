#!/bin/bash
set -e

echo "=================================="
echo "IOT-TileNodeCoordinator Test Suite"
echo "=================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Track test results
BACKEND_PASSED=0
FRONTEND_PASSED=0
FIRMWARE_PASSED=0

# Backend Tests
echo "${YELLOW}=== Running Backend Tests ===${NC}"
cd IOT-Backend-main/IOT-Backend-main
if go test -v -race -coverprofile=coverage.txt -covermode=atomic ./...; then
    echo "${GREEN}✓ Backend tests passed${NC}"
    BACKEND_PASSED=1
    go tool cover -func=coverage.txt | tail -1
else
    echo "${RED}✗ Backend tests failed${NC}"
fi
cd ../..
echo ""

# Frontend Tests
echo "${YELLOW}=== Running Frontend Tests ===${NC}"
cd IOT-Frontend-main/IOT-Frontend-main
if npm test -- --watch=false --browsers=ChromeHeadless --code-coverage; then
    echo "${GREEN}✓ Frontend tests passed${NC}"
    FRONTEND_PASSED=1
else
    echo "${RED}✗ Frontend tests failed${NC}"
fi
cd ../..
echo ""

# Coordinator Firmware Tests
echo "${YELLOW}=== Running Coordinator Firmware Tests ===${NC}"
cd coordinator
if pio test -e native; then
    echo "${GREEN}✓ Coordinator tests passed${NC}"
    FIRMWARE_PASSED=1
else
    echo "${RED}✗ Coordinator tests failed${NC}"
fi
cd ..
echo ""

# Node Firmware Tests
echo "${YELLOW}=== Running Node Firmware Tests ===${NC}"
cd node
if pio test -e native; then
    echo "${GREEN}✓ Node tests passed${NC}"
else
    echo "${RED}✗ Node tests failed${NC}"
    FIRMWARE_PASSED=0
fi
cd ..
echo ""

# Summary
echo "=================================="
echo "Test Summary"
echo "=================================="
if [ $BACKEND_PASSED -eq 1 ]; then
    echo "${GREEN}✓ Backend: PASSED${NC}"
else
    echo "${RED}✗ Backend: FAILED${NC}"
fi

if [ $FRONTEND_PASSED -eq 1 ]; then
    echo "${GREEN}✓ Frontend: PASSED${NC}"
else
    echo "${RED}✗ Frontend: FAILED${NC}"
fi

if [ $FIRMWARE_PASSED -eq 1 ]; then
    echo "${GREEN}✓ Firmware: PASSED${NC}"
else
    echo "${RED}✗ Firmware: FAILED${NC}"
fi
echo ""

# Exit with error if any tests failed
if [ $BACKEND_PASSED -eq 1 ] && [ $FRONTEND_PASSED -eq 1 ] && [ $FIRMWARE_PASSED -eq 1 ]; then
    echo "${GREEN}🎉 All tests passed!${NC}"
    exit 0
else
    echo "${RED}❌ Some tests failed${NC}"
    exit 1
fi
