# Frontend & Backend Startup Guide

## Issue
Frontend shows connection errors because the backend is not running:
```
Failed to load resource: net::ERR_SOCKET_NOT_CONNECTED
WebSocket connection to 'ws://localhost:8000/ws' failed
```

## Solution

### 1. Start the Backend First

#### Option A: Using Docker (Recommended)
```bash
# Start all services (backend, MQTT broker, MongoDB)
cd C:\Users\yahya\projects\IOT-TileNodeCoordinator
docker-compose up -d

# Check logs
docker-compose logs -f backend

# Stop services
docker-compose down
```

#### Option B: Run Backend Directly
```bash
cd IOT-Backend-main\IOT-Backend-main

# Build (if not already built)
go build -o iot.exe cmd/iot/main.go

# Run
.\iot.exe
```

**Expected output**:
```
2024/11/24 14:30:00 Starting IoT Backend...
2024/11/24 14:30:00 HTTP server listening on :8000
2024/11/24 14:30:00 Connecting to MQTT broker...
2024/11/24 14:30:00 MQTT connected
2024/11/24 14:30:00 WebSocket broadcaster started
```

### 2. Verify Backend is Running

**Check health endpoint**:
```bash
curl http://localhost:8000/health
```

**Expected response**:
```json
{
  "status": "healthy",
  "service": "iot-backend",
  "database": true,
  "mqtt": true,
  "coordinator": false,
  "timestamp": 1700000000
}
```

### 3. Start the Frontend

```bash
cd IOT-Frontend-main\IOT-Frontend-main

# Install dependencies (first time only)
npm install

# Start dev server
npm start
```

**Frontend will open**: `http://localhost:4200`

### 4. Verify Connection

**In browser console**, you should see:
```
[WebSocket] Service initialized
[MQTT] Service initialized
[WebSocket] Connecting to: ws://localhost:8000/ws
[WebSocket] Connected
[MQTT] Connecting to: ws://localhost:8000/mqtt
[MQTT] Connected
[DataService] Initialized
```

## Configuration

### Backend Port (Default: 8000)

**Change via environment variable**:
```bash
# Windows PowerShell
$env:HTTP_ADDR=":8080"
.\iot.exe

# Linux/Mac
HTTP_ADDR=":8080" ./iot
```

**Change via config file** (`config.yaml`):
```yaml
http:
  addr: ":8080"
```

### Frontend Environment

**File**: `IOT-Frontend-main/IOT-Frontend-main/src/app/core/services/environment.service.ts`

**Current settings**:
```typescript
apiUrl: 'http://localhost:8000'      // REST API
wsUrl: 'ws://localhost:8000/ws'      // WebSocket for telemetry
mqttWsUrl: 'ws://localhost:8000/mqtt' // MQTT WebSocket
```

**To change** (if backend runs on different port):
```typescript
apiUrl: 'http://localhost:8080'      // Change port
wsUrl: 'ws://localhost:8080/ws'      
mqttWsUrl: 'ws://localhost:8080/mqtt'
```

Or use **environment variables** (build time):
```bash
# Windows
$env:API_URL="http://localhost:8080"
npm start

# Linux/Mac
API_URL=http://localhost:8080 npm start
```

## Required Services

### 1. MQTT Broker (Mosquitto)

**Docker** (easiest):
```bash
docker run -d --name mosquitto -p 1883:1883 eclipse-mosquitto
```

**Or install locally**:
- Windows: Download from https://mosquitto.org/download/
- Start service: `net start mosquitto`

**Verify**:
```bash
mosquitto_sub -h localhost -t "#" -v
```

### 2. MongoDB

**Docker** (easiest):
```bash
docker run -d --name mongodb -p 27017:27017 mongo:latest
```

**Or install locally**:
- Windows: Download from https://www.mongodb.com/try/download/community
- Start service: `net start MongoDB`

**Verify**:
```bash
mongo --eval "db.version()"
```

## Full Stack Startup Script

**Windows** (`start-all.bat`):
```batch
@echo off
echo Starting IoT Stack...

REM Start Docker services
echo [1/4] Starting Docker services...
docker-compose up -d

REM Wait for services to be ready
echo [2/4] Waiting for services to start...
timeout /t 10

REM Start backend
echo [3/4] Starting backend...
cd IOT-Backend-main\IOT-Backend-main
start cmd /k "go run cmd/iot/main.go"
cd ..\..

REM Wait for backend to start
timeout /t 5

REM Start frontend
echo [4/4] Starting frontend...
cd IOT-Frontend-main\IOT-Frontend-main
start cmd /k "npm start"
cd ..\..

echo.
echo ============================================
echo IoT Stack Started!
echo ============================================
echo Backend:  http://localhost:8000
echo Frontend: http://localhost:4200
echo MQTT:     localhost:1883
echo MongoDB:  localhost:27017
echo ============================================
```

**Linux/Mac** (`start-all.sh`):
```bash
#!/bin/bash
echo "Starting IoT Stack..."

# Start Docker services
echo "[1/4] Starting Docker services..."
docker-compose up -d

# Wait for services to be ready
echo "[2/4] Waiting for services to start..."
sleep 10

# Start backend
echo "[3/4] Starting backend..."
cd IOT-Backend-main/IOT-Backend-main
go run cmd/iot/main.go &
BACKEND_PID=$!
cd ../..

# Wait for backend to start
sleep 5

# Start frontend
echo "[4/4] Starting frontend..."
cd IOT-Frontend-main/IOT-Frontend-main
npm start &
FRONTEND_PID=$!
cd ../..

echo ""
echo "============================================"
echo "IoT Stack Started!"
echo "============================================"
echo "Backend:  http://localhost:8000"
echo "Frontend: http://localhost:4200"
echo "MQTT:     localhost:1883"
echo "MongoDB:  localhost:27017"
echo "============================================"
echo "Backend PID:  $BACKEND_PID"
echo "Frontend PID: $FRONTEND_PID"
```

## Troubleshooting

### Error: `ERR_SOCKET_NOT_CONNECTED`
**Cause**: Backend not running  
**Solution**: Start backend first (see above)

### Error: `WebSocket connection failed`
**Cause**: Backend WebSocket endpoint not available  
**Solution**: 
1. Verify backend is running
2. Check backend logs for errors
3. Verify port 8000 is not blocked by firewall

### Error: `Failed to fetch /sites`
**Cause**: Backend API not responding  
**Solution**:
```bash
# Check if backend is listening
curl http://localhost:8000/health

# Check backend logs
docker-compose logs backend
```

### Error: `MongoDB connection failed`
**Cause**: MongoDB not running  
**Solution**:
```bash
# Start MongoDB
docker-compose up -d mongodb

# Or start local MongoDB service
net start MongoDB  # Windows
sudo systemctl start mongod  # Linux
brew services start mongodb-community  # Mac
```

### Error: `MQTT connection failed`
**Cause**: MQTT broker not running  
**Solution**:
```bash
# Start Mosquitto
docker-compose up -d mosquitto

# Or start local Mosquitto service
net start mosquitto  # Windows
sudo systemctl start mosquitto  # Linux
brew services start mosquitto  # Mac
```

### Frontend shows blank page
**Cause**: Node modules not installed  
**Solution**:
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm install
npm start
```

### Backend shows `database connection failed`
**Cause**: MongoDB URI incorrect or MongoDB not running  
**Solution**:
1. Check MongoDB is running
2. Verify connection string in backend config
3. Check `DB_URI` environment variable

### Port already in use
**Cause**: Another process using port 8000  
**Solution**:
```bash
# Find process using port 8000 (Windows)
netstat -ano | findstr :8000
taskkill /PID <PID> /F

# Find process using port 8000 (Linux/Mac)
lsof -i :8000
kill -9 <PID>

# Or change backend port
$env:HTTP_ADDR=":8001"  # Windows
HTTP_ADDR=":8001" ./iot  # Linux/Mac
```

## Logs to Monitor

### Backend Logs
```bash
# Docker
docker-compose logs -f backend

# Direct run
# Logs to stdout
```

### MQTT Broker Logs
```bash
# Subscribe to all topics
mosquitto_sub -h localhost -t "#" -v
```

### Frontend Logs
- Open browser console (F12)
- Look for `[WebSocket]`, `[MQTT]`, `[DataService]` prefixed messages

## Quick Health Check

Run this to verify all services:

```bash
# Backend
curl http://localhost:8000/health

# MQTT
mosquitto_sub -h localhost -t '$SYS/#' -C 1

# MongoDB
mongo --eval "db.version()"

# Frontend
# Navigate to http://localhost:4200
# Check browser console for errors
```

## Next Steps After Startup

1. **Open frontend**: http://localhost:4200
2. **Check dashboard**: Should show no errors
3. **Connect coordinator**: Power on ESP32-S3 coordinator
4. **Verify MQTT**: `mosquitto_sub -h localhost -t "site/#" -v`
5. **Pair nodes**: Power on ESP32-C3 nodes, they should auto-pair
6. **Check Live Monitor**: Navigate to Live Monitor page

## Summary

**Minimum required**:
1. ✅ MQTT broker (Mosquitto) running on port 1883
2. ✅ MongoDB running on port 27017
3. ✅ Backend running on port 8000
4. ✅ Frontend dev server on port 4200

**Quick start**:
```bash
# Terminal 1: Start services
docker-compose up -d

# Terminal 2: Start backend
cd IOT-Backend-main/IOT-Backend-main && go run cmd/iot/main.go

# Terminal 3: Start frontend
cd IOT-Frontend-main/IOT-Frontend-main && npm start
```

**Verify**: Open http://localhost:4200 - no errors in console!

