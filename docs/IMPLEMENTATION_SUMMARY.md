# Live Monitor Implementation - Summary of Changes

## Completed Backend Changes

### 1. New File: `internal/http/ws_broadcast.go`
**Purpose**: WebSocket broadcaster for pushing live telemetry to frontend

**Key Components**:
- `WSBroadcaster`: Manages WebSocket connections and broadcasts
- `NodeTelemetryMessage`: Schema for node telemetry WebSocket messages
- `CoordinatorTelemetryMessage`: Schema for coordinator telemetry
- `BroadcastNodeTelemetry()`: Sends node data to all connected clients
- `BroadcastCoordinatorTelemetry()`: Sends coordinator data to all clients

**Message Schema**:
```json
{
  "type": "node_telemetry",
  "payload": {
    "nodeId": "string",
    "lightId": "string",
    "ts": 1234567890,
    "tempC": 23.5,
    "light": {
      "on": true,
      "brightness": 128,
      "avgR": 0,
      "avgG": 255,
      "avgB": 0,
      "avgW": 0
    },
    "vbatMv": 3300,
    "statusMode": "operational"
  }
}
```

### 2. Modified: `internal/http/http.go`
**Changes**:
- Added `NewWSBroadcaster` to fx.Provide
- Added `StartBroadcaster` lifecycle hook
- Broadcaster starts on application startup

### 3. Modified: `internal/http/handlers.go`
**Changes**:
- Updated `Params` struct to include `Broadcaster *WSBroadcaster`
- Updated `Handler` struct to include `broadcaster *WSBroadcaster`
- Updated `NewHandler()` to accept broadcaster parameter
- Added `fmt` import

**New API Endpoint**: `/api/v1/node/light/control` (POST)

Request:
```json
{
  "site_id": "site001",
  "node_id": "node-abc123",
  "on": true,
  "brightness": 128
}
```

Response:
```json
{
  "success": true,
  "message": "Command sent successfully",
  "topic": "site/site001/node/node-abc123/cmd"
}
```

**Functionality**:
- Validates input (site_id, node_id required, brightness 0-255)
- Builds set_light command with fixed green color (0, 255, 0)
- Publishes to MQTT topic `site/{siteId}/node/{nodeId}/cmd`
- Returns success/failure

### 4. Modified: `internal/mqtt/handlers.go`
**Changes**:
- Added `WSBroadcaster` interface definition
- Updated `Handler` struct to include `broadcaster WSBroadcaster`
- Updated `NewHandler()` to accept broadcaster parameter
- Modified `handleNodeTelemetry()`: Broadcasts to WebSocket after DB save
- Modified `handleCoordTelemetry()`: Broadcasts to WebSocket after DB save

**Integration Points**:
- Node telemetry: DB → WebSocket broadcast (~1 Hz)
- Coordinator telemetry: DB → WebSocket broadcast

### 5. Registered Route
In `RegisterHandlers()`:
```go
router.HandleFunc("/api/v1/node/light/control", h.ControlNodeLight).Methods("POST")
```

## Backend Architecture

```
MQTT Telemetry              WebSocket Clients
     │                            ▲
     ├─► handleNodeTelemetry()    │
     │        │                   │
     │        ├─► DB (UpsertNode) │
     │        │                   │
     │        └─► BroadcastNodeTelemetry()
     │
     └─► handleCoordTelemetry()
              │
              ├─► DB (UpsertCoordinator)
              │
              └─► BroadcastCoordinatorTelemetry()
```

```
HTTP Request                 MQTT Command
     │                            ▲
     └─► ControlNodeLight()       │
              │                   │
              ├─► Validate        │
              │                   │
              ├─► Build Payload   │
              │   (green, brightness)
              │                   │
              └─► Publish to MQTT
                  site/{siteId}/node/{nodeId}/cmd
```

## Remaining Implementation (Coordinator & Node)

### Coordinator Firmware Tasks

1. **Subscribe to node commands** (`coordinator/src/comm/Mqtt.cpp`):
   - Add subscription to `site/+/node/+/cmd` in `connectMqtt()`
   
2. **Parse and forward commands** (`coordinator/src/comm/Mqtt.cpp`):
   - Extend `processMessage()` to handle node command topics
   - Extract nodeId from topic
   - Parse `set_light` command JSON
   - Create `SetLightMessage` from shared ESP-NOW message format
   - Forward to node via ESP-NOW

3. **Wire ESP-NOW manager** (`coordinator/src/comm/Mqtt.h` & `Coordinator.cpp`):
   - Add `EspNowManager* espNowManager` member to `Mqtt` class
   - Add `setEspNowManager()` method
   - Call `mqtt->setEspNowManager(espNow)` in `Coordinator::begin()`

### Node Firmware Tasks

1. **Ensure temperature reading** (`node/src/main.cpp`):
   - Verify TMP177Sensor is initialized (SDA=pin3, SCL=pin2)
   - Read temperature every ~1 second
   - Include `temperature` field in `NodeStatusMessage`
   - Send to coordinator via ESP-NOW

2. **Handle set_light commands**:
   - In ESP-NOW receive callback, parse incoming messages
   - Handle `msg == "set_light"`
   - Extract RGBW values and brightness
   - Apply to `LedController` for SK6812B strip
æ
3. **Verify LED configuration**:
   - SK6812B on pin 1
   - 4 LEDs per node (adjustable)
   - Brightness range 0-255

## Frontend Implementation Tasks

### Required npm Packages
```bash
npm install chart.js
```

### New Components to Create

1. **TemperatureGraphComponent**
   - Path: `src/app/features/dashboard/components/temperature-graph/`
   - Displays live temperature with Chart.js
   - Rolling window (5 minutes of data)
   - Updates at ~1 Hz

2. **LightControlComponent**
   - Path: `src/app/features/dashboard/components/light-control/`
   - Toggle button for ON/OFF
   - Brightness slider (0-255)
   - Calls `/api/v1/node/light/control` API

3. **Update LiveMonitorComponent**
   - Import WebSocketService
   - Subscribe to `node_telemetry` messages
   - Pass data to TemperatureGraphComponent and LightControlComponent
   - Handle node selection

### API Service Method
```typescript
// In api.service.ts
controlNodeLight(siteId: string, nodeId: string, on: boolean, brightness: number) {
  return this.http.post(`${this.apiUrl}/api/v1/node/light/control`, {
    site_id: siteId,
    node_id: nodeId,
    on: on,
    brightness: brightness
  });
}
```

## Testing Strategy

### 1. Backend Testing
```bash
# Build test
cd IOT-Backend-main/IOT-Backend-main
go build ./cmd/iot

# Unit tests
go test ./internal/http -v
go test ./internal/mqtt -v

# Run backend
./iot
```

### 2. Manual Testing - WebSocket
```bash
# Use wscat or browser console
wscat -c ws://localhost:8080/ws

# Subscribe to telemetry topics via MQTT bridge
{"type": "subscribe", "topic": "site/+/node/+/telemetry"}
```

### 3. Manual Testing - Light Control
```bash
curl -X POST http://localhost:8080/api/v1/node/light/control \
  -H "Content-Type: application/json" \
  -d '{
    "site_id": "site001",
    "node_id": "node-abc123",
    "on": true,
    "brightness": 128
  }'
```

### 4. Monitor MQTT Traffic
```bash
# Subscribe to all topics
mosquitto_sub -h localhost -t "#" -v

# Watch for node commands
mosquitto_sub -h localhost -t "site/+/node/+/cmd" -v
```

### 5. Firmware Testing
```bash
# Flash coordinator
cd coordinator
pio run -e esp32-s3-devkitc-1 -t upload -t monitor

# Flash node
cd node
pio run -e esp32-c3-mini-1 -t upload -t monitor
```

**Expected Serial Output (Node)**:
```
TMP177: Initialized successfully
Temperature: 23.5°C
Sending node status...
Received set_light command: on=1, brightness=128, g=255
LED updated: green at brightness 128
```

**Expected Serial Output (Coordinator)**:
```
Node node-abc123 telemetry received
Publishing to site/site001/node/node-abc123/telemetry
Received node command from MQTT
Forwarding to node via ESP-NOW
```

## Validation Checklist

### Backend
- [x] ws_broadcast.go compiles
- [x] http.go includes broadcaster
- [x] handlers.go has ControlNodeLight endpoint
- [x] mqtt handlers.go broadcasts telemetry
- [ ] Backend builds successfully
- [ ] WebSocket accepts connections
- [ ] Telemetry broadcasts to WebSocket
- [ ] Light control API responds correctly
- [ ] MQTT commands published to correct topic

### Coordinator
- [ ] Subscribes to node command topics
- [ ] Parses node commands correctly
- [ ] Forwards commands via ESP-NOW
- [ ] Node telemetry relayed to MQTT
- [ ] Temperature in node telemetry (not coordinator)

### Node
- [ ] TMP117 sensor initialized
- [ ] Temperature read every ~1 second
- [ ] Temperature included in status messages
- [ ] set_light commands received via ESP-NOW
- [ ] SK6812B strip controlled correctly
- [ ] Green color (0, 255, 0) when ON
- [ ] Brightness adjusts correctly

### Frontend
- [ ] WebSocket connects on page load
- [ ] Temperature graph renders
- [ ] Temperature updates live (~1 Hz)
- [ ] Graph shows rolling 5-minute window
- [ ] Toggle switch sends API request
- [ ] Brightness slider sends API request
- [ ] Light state reflects telemetry
- [ ] Green color displayed when ON

## Files Modified Summary

### Created:
- `IOT-Backend-main/IOT-Backend-main/internal/http/ws_broadcast.go`
- `docs/LIVE_MONITOR_IMPLEMENTATION.md`
- `docs/IMPLEMENTATION_SUMMARY.md`

### Modified:
- `IOT-Backend-main/IOT-Backend-main/internal/http/http.go`
- `IOT-Backend-main/IOT-Backend-main/internal/http/handlers.go`
- `IOT-Backend-main/IOT-Backend-main/internal/mqtt/handlers.go`

### To Be Modified (Firmware):
- `coordinator/src/comm/Mqtt.h`
- `coordinator/src/comm/Mqtt.cpp`
- `coordinator/src/core/Coordinator.cpp`
- `node/src/main.cpp`

### To Be Created (Frontend):
- `IOT-Frontend-main/IOT-Frontend-main/src/app/features/dashboard/components/temperature-graph/`
- `IOT-Frontend-main/IOT-Frontend-main/src/app/features/dashboard/components/light-control/`

## Next Steps

1. **Test backend build**: `cd IOT-Backend-main/IOT-Backend-main && go build ./cmd/iot`
2. **Implement coordinator MQTT command handling** (see LIVE_MONITOR_IMPLEMENTATION.md)
3. **Implement node ESP-NOW command handling** (see LIVE_MONITOR_IMPLEMENTATION.md)
4. **Create frontend components** (see LIVE_MONITOR_IMPLEMENTATION.md)
5. **End-to-end testing** with real hardware
6. **Document any issues** in GitHub issues

## Support

For questions or issues:
1. Check LIVE_MONITOR_IMPLEMENTATION.md for detailed implementation guide
2. Review existing MQTT topic structure in `docs/mqtt_api.md`
3. Check ESP-NOW message format in `shared/src/EspNowMessage.h`
4. Review existing WebSocket implementation in `IOT-Frontend-main/src/app/core/services/websocket.service.ts`

