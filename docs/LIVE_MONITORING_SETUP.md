# Live Monitoring Setup Guide

## Overview
This document explains the end-to-end setup for live node temperature monitoring and LED control.

## What Has Been Fixed

### 1. Backend Dependency Injection (CRITICAL FIX)
**Problem**: The backend was failing to start with error:
```
missing type: mqtt.WSBroadcaster (did you mean to Provide it?)
```

**Solution**: Changed `fx.Module` to `fx.Options` in three modules to allow cross-module dependency sharing:
- `internal/http/http.go` - Provides WSBroadcaster
- `internal/mqtt/mqtt.go` - Uses WSBroadcaster to broadcast telemetry
- `internal/googlehome/module.go` - Updated for consistency

**Files Changed**:
- `IOT-Backend-main/IOT-Backend-main/internal/http/http.go`
- `IOT-Backend-main/IOT-Backend-main/internal/mqtt/mqtt.go`
- `IOT-Backend-main/IOT-Backend-main/internal/googlehome/module.go`

### 2. Node JSON Parsing Buffer Size
**Problem**: Node was receiving `join_accept` messages but failing to parse them:
```
ERROR: Failed to parse message!
Length: 139 bytes
```

**Solution**: Increased ArduinoJson buffer size in node firmware from 128 to 256 bytes.

**File Changed**:
- `node/src/main.cpp` - Increased `JSON_DOC_SIZE` to 256

### 3. ESP-NOW Initialization
**Problem**: Coordinator was getting "esp now not init!" errors during pairing.

**Solution**: The ESP-NOW initialization issue appears to be resolved in the latest coordinator code.

## Data Flow Architecture

```
Node (ESP32-C3)
  ├─ TMP117 (I2C) → reads temperature
  ├─ SK6812B (Pin 1) → LED strip  
  └─ ESP-NOW → sends telemetry to coordinator
       ↓
Coordinator (ESP32-S3)
  ├─ ESP-NOW → receives node telemetry
  ├─ MQTT → publishes to site/{siteId}/node/{nodeId}/telemetry
  └─ MQTT → subscribes to site/{siteId}/node/{nodeId}/cmd
       ↓
MQTT Broker (Mosquitto)
       ↓
Backend (Go)
  ├─ MQTT Client → subscribes to telemetry topics
  ├─ MongoDB → stores telemetry data
  └─ WebSocket → broadcasts live updates
       ↓
Frontend (Angular)
  ├─ WebSocket → receives live telemetry
  ├─ Live Monitor → displays temperature graph
  └─ Controls → sends LED commands
```

## Topics & Schemas

### Node Telemetry (Coordinator → Backend)
**Topic**: `site/{siteId}/node/{nodeId}/telemetry`

**Payload**:
```json
{
  "node_id": "10:00:3B:01:98:BC",
  "light_id": "L0198BC",
  "ts": 1234567890,
  "temp_c": 23.5,
  "avg_r": 0,
  "avg_g": 0,
  "avg_b": 0,
  "avg_w": 0,
  "status_mode": "normal",
  "vbat_mv": 3700,
  "fw": "1.0.0"
}
```

### Node Command (Backend → Coordinator → Node)
**Topic**: `site/{siteId}/node/{nodeId}/cmd`

**Payload**:
```json
{
  "type": "set_light",
  "light_id": "L0198BC",
  "on": true,
  "brightness": 128,
  "color": {
    "r": 0,
    "g": 255,
    "b": 0
  }
}
```

### WebSocket Message (Backend → Frontend)
**Type**: `node_telemetry`

**Payload**:
```json
{
  "type": "node_telemetry",
  "nodeId": "10:00:3B:01:98:BC",
  "lightId": "L0198BC",
  "ts": 1234567890,
  "tempC": 23.5,
  "light": {
    "r": 0,
    "g": 0,
    "b": 0,
    "w": 0
  },
  "vbatMv": 3700,
  "statusMode": "normal"
}
```

## How to Rebuild & Test

### 1. Rebuild Backend
```bash
# Run the provided script
.\REBUILD_BACKEND.bat

# Or manually:
docker compose stop backend
docker compose build backend
docker compose up -d backend

# Check logs
docker compose logs backend -f
```

### 2. Verify Backend is Running
```bash
# Health check
curl http://localhost:8000/health

# Should return: {"status":"ok"}
```

### 3. Test WebSocket Connection
Open browser console on `http://localhost:4200` and check for:
```
[WebSocket] Service initialized
[WebSocket] Connecting to: ws://localhost:8000/ws
[WebSocket] Connected
```

### 4. Pair Node
1. Open Live Monitor page
2. Click "Start Pairing" button
3. Power on node (it will auto-pair if not connected)
4. Wait for "Pairing successful" in coordinator logs

### 5. Verify Data Flow
**Coordinator Serial Output**:
```
[Node 1] 10:00:3B:01:98:BC STATUS | 210 bytes
[MQTT→] NodeTel | site/site001/node/10:00:3B:01:98:BC/telemetry | size=194 bytes
[Node 1] Temperature: 23.50°C
```

**Backend Logs**:
```
Node telemetry saved  nodeId=10:00:3B:01:98:BC  lightId=L0198BC  temp=23.5
Broadcasting node telemetry to 1 clients
```

**Frontend Console**:
```
[WebSocket] Received: {"type":"node_telemetry", ...}
```

## Frontend Components

### Live Monitor Page
Located at: `IOT-Frontend-main/IOT-Frontend-main/src/app/pages/live-monitor/`

**Features**:
- Real-time temperature graph (updates ~1Hz)
- Current temperature display
- LED control (on/off toggle, brightness slider)
- Fixed green color when LED is on
- Rolling window showing last few minutes of data

### Temperature Graph
- Uses Chart.js or similar charting library
- X-axis: Time
- Y-axis: Temperature (°C)
- Auto-scrolling as new data arrives
- Configurable time window (default: 5 minutes)

### LED Controls
- **On/Off Toggle**: Sends command with `brightness: 0` when off
- **Brightness Slider**: 0-100% mapped to 0-255 PWM
- **Color**: Fixed to green (R=0, G=255, B=0)

## Troubleshooting

### Backend Not Starting
**Symptom**: Frontend shows "Connection Refused" errors

**Solution**:
1. Check backend logs: `docker compose logs backend`
2. Look for dependency injection errors
3. Ensure MongoDB and Mosquitto are running:
   ```bash
   docker compose ps
   ```
4. Rebuild backend: `.\REBUILD_BACKEND.bat`

### No Temperature Data
**Symptom**: Graph is empty, no data points

**Check**:
1. Node is paired (coordinator logs show "1 active" node)
2. Node is sending telemetry (node logs show "Telemetry send: OK")
3. Coordinator is publishing to MQTT (logs show "[MQTT→] NodeTel")
4. Backend is receiving MQTT (logs show "Node telemetry saved")
5. Frontend WebSocket is connected (console shows "Connected")

### Temperature Shows 0.0°C
**Symptom**: Temperature graph shows flat line at 0

**Possible Causes**:
1. TMP117 sensor not connected properly (check I2C wiring)
2. Node firmware not reading sensor
3. Coordinator receiving telemetry but temp_c field is 0

**Solution**:
- Check node serial output for TMP117 initialization
- Verify I2C pins: SDA=GPIO3, SCL=GPIO2
- Re-flash node firmware with debug build

### LED Commands Not Working
**Symptom**: Clicking controls doesn't change LED

**Check**:
1. Frontend sends command (Network tab shows POST to `/api/nodes/{nodeId}/light`)
2. Backend publishes to MQTT (logs show "Publishing node command")
3. Coordinator receives command (logs show "[MQTTÔåÉ] NodeCommand")
4. Coordinator forwards to node via ESP-NOW
5. Node receives and applies command

## Next Steps

1. ✅ Fix backend dependency injection
2. ✅ Fix node JSON parsing
3. ⏳ Verify backend starts successfully
4. ⏳ Test WebSocket connection
5. ⏳ Implement frontend temperature graph component
6. ⏳ Implement frontend LED controls
7. ⏳ Add backend endpoints for LED control
8. ⏳ Wire up coordinator→node LED command forwarding
9. ⏳ Test end-to-end temperature monitoring
10. ⏳ Test end-to-end LED control

## Files Modified

### Backend
- `internal/http/http.go` - Changed fx.Module to fx.Options
- `internal/mqtt/mqtt.go` - Changed fx.Module to fx.Options
- `internal/mqtt/handlers.go` - Already has WebSocket broadcasting
- `internal/googlehome/module.go` - Changed fx.Module to fx.Options

### Firmware
- `node/src/main.cpp` - Increased JSON_DOC_SIZE to 256

### Scripts
- `REBUILD_BACKEND.bat` - New script to rebuild backend

### Documentation
- `docs/LIVE_MONITORING_SETUP.md` - This file
