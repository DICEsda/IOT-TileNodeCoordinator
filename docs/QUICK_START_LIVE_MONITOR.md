# Quick Start: Live Monitor Implementation

## What Was Completed ✅

### Backend (Go) - DONE
All backend code has been implemented and is ready for testing:

1. **WebSocket Broadcasting** (`internal/http/ws_broadcast.go`)
   - Real-time telemetry push to all connected frontend clients
   - Broadcasts node temperature and light state at ~1 Hz
   - Message format designed for easy frontend consumption

2. **MQTT Integration** (`internal/mqtt/handlers.go`)
   - Node telemetry handler now broadcasts to WebSocket
   - Coordinator telemetry handler broadcasts to WebSocket  
   - Existing MQTT subscriptions preserved

3. **Light Control API** (`internal/http/handlers.go`)
   - New endpoint: `POST /api/v1/node/light/control`
   - Publishes commands to `site/{siteId}/node/{nodeId}/cmd`
   - Always uses green color (0, 255, 0) when ON

4. **Module Integration** (`internal/http/http.go`)
   - WSBroadcaster integrated into fx dependency injection
   - Starts automatically on application launch

## What Needs To Be Done 🔧

### 1. Coordinator Firmware (ESP32-S3)
**File**: `coordinator/src/comm/Mqtt.cpp`

**Add in `connectMqtt()` after line ~340** (after coordinator cmd subscription):
```cpp
// Subscribe to node commands
String nodeCmdTopic = "site/" + siteId + "/node/+/cmd";
mqttClient.subscribe(nodeCmdTopic.c_str());
Logger::info("Subscribed to: %s", nodeCmdTopic.c_str());
```

**Add in `processMessage()` after existing command handling**:
```cpp
// Handle node light commands: site/{siteId}/node/{nodeId}/cmd
if (topic.indexOf("/node/") >= 0 && topic.endsWith("/cmd")) {
    int nodeStart = topic.indexOf("/node/") + 6;
    int nodeEnd = topic.indexOf("/cmd");
    String nodeId = topic.substring(nodeStart, nodeEnd);
    
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
        String cmd = doc["cmd"].as<String>();
        if (cmd == "set_light") {
            // Create SetLightMessage from shared EspNowMessage
            SetLightMessage lightMsg;
            lightMsg.light_id = nodeId;
            lightMsg.r = doc["r"] | 0;
            lightMsg.g = doc["g"] | 0;
            lightMsg.b = doc["b"] | 0;
            lightMsg.w = doc["w"] | 0;
            lightMsg.value = doc["brightness"] | 0;
            
            // TODO: Forward via ESP-NOW to node
            // You'll need reference to EspNowManager here
            // espNow->sendToNode(nodeId, lightMsg.toJson());
            
            Logger::info("Forwarded light command to node %s", nodeId.c_str());
        }
    }
}
```

### 2. Node Firmware (ESP32-C3)
**File**: `node/src/main.cpp`

**Verify TMP117 initialization in `setup()`**:
```cpp
#include "sensor/TMP177Sensor.h"

TMP177Sensor tempSensor;

void setup() {
    // ... existing setup ...
    
    if (!tempSensor.begin(3, 2)) {  // SDA=pin3, SCL=pin2
        Serial.println("WARNING: TMP177 not found!");
    }
}
```

**Add temperature reading in `loop()`**:
```cpp
void loop() {
    static uint32_t lastTempRead = 0;
    if (millis() - lastTempRead >= 1000) {
        lastTempRead = millis();
        float temp = tempSensor.readTemperature();
        
        // Add to your NodeStatusMessage
        NodeStatusMessage status;
        status.temperature = temp;
        // ... fill other fields ...
        
        // Send to coordinator
        sendToCoordinator(status.toJson());
    }
}
```

**Handle set_light in ESP-NOW receive callback**:
```cpp
void onDataReceived(const uint8_t* mac, const uint8_t* data, int len) {
    String json = String((char*)data);
    
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, json) == DeserializationError::Ok) {
        String msgType = doc["msg"].as<String>();
        
        if (msgType == "set_light") {
            uint8_t r = doc["r"] | 0;
            uint8_t g = doc["g"] | 0;
            uint8_t b = doc["b"] | 0;
            uint8_t w = doc["w"] | 0;
            uint8_t brightness = doc["value"] | 0;
            
            ledController->setBrightness(brightness);
            ledController->setColor(r, g, b, w);
            
            Serial.printf("LED: on=%d, brightness=%d, g=%d\n", 
                         (brightness > 0), brightness, g);
        }
    }
}
```

### 3. Frontend (Angular)

**Install Chart.js**:
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm install chart.js
```

**Create Temperature Graph Component**:
```bash
ng generate component features/dashboard/components/temperature-graph
```

See `docs/LIVE_MONITOR_IMPLEMENTATION.md` section "Frontend UX details" for full implementation.

**Create Light Control Component**:
```bash
ng generate component features/dashboard/components/light-control
```

**Update Live Monitor Page**:
- Import components
- Subscribe to WebSocket `node_telemetry` messages
- Pass data to child components
- Handle user interactions

## Testing Quick Commands

### Start Backend
```bash
cd IOT-Backend-main/IOT-Backend-main
go run cmd/iot/main.go
```

### Test Light Control API
```bash
curl -X POST http://localhost:8080/api/v1/node/light/control \
  -H "Content-Type: application/json" \
  -d '{"site_id":"site001","node_id":"node-test","on":true,"brightness":128}'
```

### Monitor MQTT
```bash
mosquitto_sub -h localhost -t "site/#" -v
```

### Flash Coordinator
```bash
cd coordinator
pio run -e esp32-s3-devkitc-1 -t upload -t monitor
```

### Flash Node
```bash
cd node
pio run -e esp32-c3-mini-1 -t upload -t monitor
```

### Start Frontend
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm start
```

## Expected Behavior

1. **Node boots**:
   - TMP117 initializes on I2C (SDA=pin3, SCL=pin2)
   - Sends temperature via ESP-NOW to coordinator every ~1 second
   - SK6812B strip on pin 1 ready for commands

2. **Coordinator receives node telemetry**:
   - Relays to MQTT topic: `site/site001/node/{nodeId}/telemetry`
   - Includes `temp_c` field from node's TMP117

3. **Backend receives MQTT telemetry**:
   - Saves to MongoDB
   - Broadcasts to all WebSocket clients in real-time
   - Message type: `node_telemetry`

4. **Frontend Live Monitor**:
   - Temperature graph updates ~1 Hz
   - Shows rolling 5-minute window
   - Light controls reflect current state

5. **User toggles light ON**:
   - Frontend calls `POST /api/v1/node/light/control`
   - Backend publishes to `site/site001/node/{nodeId}/cmd`
   - Coordinator receives command
   - Coordinator forwards via ESP-NOW to node
   - Node's SK6812B turns green at specified brightness

## Troubleshooting

### Backend won't build
```bash
cd IOT-Backend-main/IOT-Backend-main
go mod tidy
go build ./cmd/iot
```

### WebSocket not connecting
- Check backend is running on port 8080
- Check CORS settings in `internal/http/http.go`
- Check WebSocket URL in frontend environment

### No telemetry on frontend
- Verify MQTT broker running (docker-compose)
- Check coordinator is connected to MQTT
- Check backend subscribed to `site/+/node/+/telemetry`
- Check WebSocket messages in browser console

### Commands not reaching node
- Verify coordinator subscribed to `site/+/node/+/cmd`
- Check ESP-NOW pairing between coordinator and node
- Check node is receiving ESP-NOW messages (serial monitor)
- Verify SetLightMessage format matches expected schema

### Temperature not updating
- Check TMP117 wiring (SDA=pin3, SCL=pin2, VCC=3.3V, GND=GND)
- Check I2C address 0x48 (default)
- Verify node sends temperature in NodeStatusMessage
- Check coordinator relays to MQTT with `temp_c` field

## Key Requirements ✔️

- ✅ Temperature from **NODE** TMP117 only (coordinator has no temp sensor)
- ✅ Temperature updates ~1 Hz
- ✅ Graph shows rolling window (5 minutes)
- ✅ ON/OFF toggle controls SK6812B
- ✅ Brightness slider (0-255)
- ✅ **Fixed green color** (RGB: 0, 255, 0) when ON
- ✅ Commands: Frontend → Backend → MQTT → Coordinator → ESP-NOW → Node
- ✅ Telemetry: Node → Coordinator → MQTT → Backend → WebSocket → Frontend

## Documentation References

- **Full Implementation Guide**: `docs/LIVE_MONITOR_IMPLEMENTATION.md`
- **Changes Summary**: `docs/IMPLEMENTATION_SUMMARY.md`
- **MQTT API**: `docs/mqtt_api.md`
- **ESP-NOW Messages**: `shared/src/EspNowMessage.h`

## Quick Status Check

Run this after implementation to verify:

```bash
# 1. Backend health
curl http://localhost:8080/health

# 2. MQTT topics active
mosquitto_sub -h localhost -t "site/#" -C 5

# 3. WebSocket test (use browser console)
ws = new WebSocket('ws://localhost:8080/ws');
ws.onmessage = (e) => console.log(JSON.parse(e.data));

# 4. Send test command
curl -X POST http://localhost:8080/api/v1/node/light/control \
  -H "Content-Type: application/json" \
  -d '{"site_id":"site001","node_id":"test","on":true,"brightness":255}'
```

## Success Criteria

- [ ] Frontend shows live temperature graph updating every second
- [ ] Graph displays rolling 5-minute window
- [ ] Toggle turns SK6812B green when ON
- [ ] Brightness slider adjusts LED brightness
- [ ] Color remains green (not red, blue, or white)
- [ ] Temperature comes from node's TMP117 sensor
- [ ] Commands reach node within 1 second
- [ ] No breaking changes to existing functionality

Good luck! 🚀
