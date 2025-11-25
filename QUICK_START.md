# Quick Start Guide

## Fix Frontend Connection Errors

Your frontend shows these errors because **the backend is not running**:
```
Failed to load resource: net::ERR_SOCKET_NOT_CONNECTED
WebSocket connection to 'ws://localhost:8000/ws' failed
```

## Solution (3 Steps)

### Step 1: Start Docker Services (if using Docker)

```bash
docker-compose up -d
```

This starts:
- MQTT broker (Mosquitto) on port 1883
- MongoDB on port 27017

**Skip this step if you have MQTT and MongoDB running locally.**

### Step 2: Start Backend

**Option A - Use the startup script**:
```bash
# Double-click this file, or run in terminal:
start-backend.bat
```

**Option B - Manual start**:
```bash
cd IOT-Backend-main\IOT-Backend-main
go run cmd\iot\main.go
```

**Expected output**:
```
Starting IoT Backend...
HTTP server listening on :8000
Connecting to MQTT broker...
MQTT connected
WebSocket broadcaster started
```

### Step 3: Start Frontend

**Option A - Use the startup script**:
```bash
# Double-click this file, or run in terminal:
start-frontend.bat
```

**Option B - Manual start**:
```bash
cd IOT-Frontend-main\IOT-Frontend-main
npm install  # First time only
npm start
```

**Frontend opens at**: http://localhost:4200

### Step 4: Verify

**Browser console should show** (F12 to open):
```
[WebSocket] Service initialized
[MQTT] Service initialized
[WebSocket] Connecting to: ws://localhost:8000/ws
[WebSocket] Connected ✓
[MQTT] Connected ✓
[DataService] Initialized
```

**No more errors!** ✅

## Full System Startup Order

If starting everything from scratch:

1. **Docker services** (MQTT + MongoDB):
   ```bash
   docker-compose up -d
   ```

2. **Backend** (in new terminal):
   ```bash
   start-backend.bat
   ```
   OR
   ```bash
   cd IOT-Backend-main\IOT-Backend-main && go run cmd\iot\main.go
   ```

3. **Frontend** (in new terminal):
   ```bash
   start-frontend.bat
   ```
   OR
   ```bash
   cd IOT-Frontend-main\IOT-Frontend-main && npm start
   ```

4. **Coordinator** (flash ESP32-S3):
   ```bash
   cd coordinator
   pio run -e esp32-s3-devkitc-1 -t upload -t monitor
   ```

5. **Nodes** (flash ESP32-C3):
   ```bash
   cd node
   pio run -e esp32-c3-mini-1 -t upload -t monitor
   ```

## Verify Everything is Working

### Backend Health Check
```bash
curl http://localhost:8000/health
```

Should return:
```json
{
  "status": "healthy",
  "database": true,
  "mqtt": true
}
```

### MQTT Test
```bash
mosquitto_sub -h localhost -t "#" -v
```

Should show messages when coordinator/nodes are active.

### Frontend
Open http://localhost:4200 and check browser console - no errors!

## Troubleshooting

### Backend won't start

**Error**: `go: command not found`
**Fix**: Install Go from https://go.dev/dl/

**Error**: `MongoDB connection failed`
**Fix**: Start MongoDB:
```bash
docker-compose up -d mongodb
# OR
net start MongoDB
```

**Error**: `MQTT connection failed`
**Fix**: Start Mosquitto:
```bash
docker-compose up -d mosquitto
# OR
net start mosquitto
```

### Frontend won't start

**Error**: `npm: command not found`
**Fix**: Install Node.js from https://nodejs.org/

**Error**: `Module not found`
**Fix**: Install dependencies:
```bash
cd IOT-Frontend-main\IOT-Frontend-main
npm install
```

### Port already in use

**Backend port 8000 in use**:
```bash
# Find and kill process (Windows)
netstat -ano | findstr :8000
taskkill /PID <PID> /F

# Or change port
$env:HTTP_ADDR=":8001"
start-backend.bat
```

**Frontend port 4200 in use**:
```bash
# Kill Angular dev server
taskkill /F /IM node.exe
```

## What Each Service Does

| Service | Port | Purpose |
|---------|------|---------|
| **Backend** | 8000 | REST API, WebSocket, MQTT bridge |
| **Frontend** | 4200 | Angular UI (dev server) |
| **MQTT** | 1883 | Message broker for IoT devices |
| **MongoDB** | 27017 | Database for telemetry/config |

## Files Created to Help You

- ✅ `start-backend.bat` - Starts backend easily
- ✅ `start-frontend.bat` - Starts frontend easily
- ✅ `QUICK_START.md` - This guide
- ✅ `docs/FRONTEND_BACKEND_STARTUP.md` - Detailed startup guide

## Summary

**To fix your current errors**:
1. Run `start-backend.bat` (or manually start backend)
2. Refresh your browser
3. Errors gone! ✅

**For complete system**:
1. `docker-compose up -d` (services)
2. `start-backend.bat` (backend)
3. `start-frontend.bat` (frontend)
4. Flash coordinator & nodes
5. Open http://localhost:4200

## Need More Help?

See detailed guides in `docs/`:
- `docs/FRONTEND_BACKEND_STARTUP.md` - Complete startup guide
- `docs/SESSION_SUMMARY.md` - All features implemented
- `CHANGES.md` - Recent changes and features

