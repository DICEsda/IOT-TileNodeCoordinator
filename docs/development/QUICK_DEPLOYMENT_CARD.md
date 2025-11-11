# 🚀 Quick Deployment Reference Card

## System Overview
- **Frontend**: Angular 19 dashboard (port 4200)
- **Backend**: Go REST API + MQTT + WebSocket (port 8080)
- **MQTT Broker**: Mosquitto (ports 1883, 9001)
- **Database**: MongoDB (port 27017)
- **Firmware**: ESP32-S3 Coordinator + ESP32-C3 Nodes

---

## ⚡ Quick Start (5 Minutes)

### 1. Start Infrastructure (Docker)
```bash
# In IOT-Backend-main/IOT-Backend-main/
docker-compose up -d
```
This starts: MongoDB + MQTT Broker + Go Backend

### 2. Start Frontend
```bash
# In IOT-Frontend-main/IOT-Frontend-main/
npm install
ng serve
```
Access: http://localhost:4200

### 3. Flash Firmware
```bash
# Coordinator
cd coordinator
pio run -t upload

# Node(s)
cd ../node
pio run -t upload
```

### 4. Configure Coordinator WiFi/MQTT
Connect via serial (115200 baud):
```
# TODO: Add serial config commands or use NVS setup script
```

---

## 📡 MQTT Topics Quick Reference

### Published by Firmware
```
site/site001/node/AA:BB:CC:DD:EE:FF/telemetry    (Node telemetry)
site/site001/coord/AA:BB:CC:DD:EE:FF/telemetry   (Coordinator telemetry)
site/site001/coord/AA:BB:CC:DD:EE:FF/mmwave      (Presence events)
```

### Subscribed by Firmware
```
site/site001/coord/AA:BB:CC:DD:EE:FF/cmd         (Coordinator commands)
site/site001/node/+/cmd                          (Node commands, wildcard)
```

### Backend Subscriptions
```
site/+/node/+/telemetry      (All node telemetry)
site/+/coord/+/telemetry     (All coordinator telemetry)
site/+/coord/+/mmwave        (All mmWave events)
```

---

## 🔧 Configuration Files

### Backend (.env or docker-compose.yml)
```yaml
MQTT_BROKER_URL: tcp://localhost:1883
MQTT_USERNAME: user1
MQTT_PASSWORD: user1
MONGODB_URI: mongodb://localhost:27017
JWT_SECRET: your-secret-key
PORT: 8080
```

### Frontend (src/environments/environment.ts)
```typescript
export const environment = {
  production: false,
  apiUrl: 'http://localhost:8080/api/v1',
  wsUrl: 'ws://localhost:8080/ws',
  mqttBrokerUrl: 'ws://localhost:9001',
  defaultSiteId: 'site001'
};
```

### Firmware (NVS - via ConfigManager)
```cpp
// Namespace: "mqtt"
wifi_ssid: "YourNetwork"
wifi_pass: "YourPassword"
broker_host: "192.168.1.100"
broker_port: 1883
broker_user: "user1"
broker_pass: "user1"
```

---

## 🧪 Testing Endpoints

### Backend Health Check
```bash
curl http://localhost:8080/api/v1/health
```

### Get All Lights
```bash
curl http://localhost:8080/api/v1/lights
```

### Set Light Brightness
```bash
curl -X POST http://localhost:8080/api/v1/lights/light-1/brightness \
  -H "Content-Type: application/json" \
  -d '{"brightness": 128, "fade_ms": 300}'
```

### WebSocket Connection (Browser Console)
```javascript
const ws = new WebSocket('ws://localhost:8080/ws');
ws.onmessage = (event) => console.log(JSON.parse(event.data));
```

---

## 🐛 Troubleshooting

### Coordinator Not Connecting to WiFi
1. Check serial monitor for WiFi logs
2. Verify SSID/password in NVS
3. Check WiFi signal strength
4. Try `WiFi.disconnect()` then `WiFi.begin()`

### Backend Not Receiving MQTT Messages
1. Check MQTT broker is running: `docker ps | grep mosquitto`
2. Test with MQTT client: `mosquitto_sub -t "site/#" -v`
3. Verify coordinator connected: check serial logs
4. Check backend MQTT client logs

### Frontend Not Showing Data
1. Check browser console for errors
2. Verify WebSocket connection: Network tab in DevTools
3. Check backend logs for WebSocket broadcasts
4. Verify API URL in environment.ts

### Commands Not Reaching Nodes
1. Check coordinator subscribes to correct topics (serial logs)
2. Verify backend publishes to correct topic format
3. Check ESP-NOW peer list on coordinator
4. Verify node MAC address matches

---

## 📊 Monitoring

### Firmware Logs (Serial Monitor)
```bash
pio device monitor -b 115200
```

### Backend Logs
```bash
docker logs -f iot-backend
```

### MQTT Broker Logs
```bash
docker logs -f mosquitto
```

### MongoDB Logs
```bash
docker logs -f mongodb
```

---

## 🔑 Important MAC Addresses

Coordinator MAC becomes `coordId` in topics:
- Format: `A4:CF:12:34:56:78`
- Obtained via: `WiFi.macAddress()`
- Used in topics: `site/site001/coord/A4:CF:12:34:56:78/telemetry`

Node MAC becomes `nodeId`:
- Assigned during ESP-NOW pairing
- Stored in coordinator NodeRegistry
- Used in topics: `site/site001/node/AA:BB:CC:DD:EE:FF/telemetry`

---

## 📱 Pairing New Nodes

1. **Coordinator**: Short press button (< 4s) to open pairing window (60s)
   - Status LED shows blue pulse
2. **Node**: Power on or reset
   - Node sends JOIN_REQUEST via ESP-NOW
3. **Coordinator**: Responds with JOIN_ACCEPT
   - Status LED shows green confirmation
   - Node LED assigned to group (4 pixels per node)
4. **Verify**: Check serial logs on both devices
5. **Test**: Node appears in frontend dashboard with telemetry

---

## 🎨 LED Status Indicators (Coordinator)

| Color | Pattern | Meaning |
|-------|---------|---------|
| Blue | Pulse | Pairing mode active (60s window) |
| Green | Flash 400ms | Node successfully paired |
| Green | Solid dim | Node connected and online |
| Green | Flash 150ms | Activity (message received) |
| Red | Solid | Node disconnected/timeout |
| Red | Flash 200ms | ESP-NOW send error |
| Off | - | No node assigned to this LED group |

**LED Groups**: 4 pixels per node (e.g., 8 nodes = 32 total pixels on SK6812B strip)

---

## 📦 File Locations

### Critical Firmware Files
- `coordinator/src/comm/Mqtt.cpp` - MQTT wrapper with WiFi
- `coordinator/src/comm/MqttHandler.cpp` - PRD-compliant MQTT
- `coordinator/src/comm/EspNow.cpp` - ESP-NOW v2 implementation
- `coordinator/src/core/Coordinator.cpp` - Main orchestration

### Critical Backend Files
- `internal/mqtt/handlers.go` - MQTT topic handlers
- `internal/api/handlers.go` - REST API endpoints
- `internal/websocket/hub.go` - Real-time broadcasting

### Critical Frontend Files
- `src/app/core/services/api.service.ts` - HTTP client (300+ lines)
- `src/app/core/services/mqtt.service.ts` - MQTT pub/sub (400+ lines)
- `src/app/core/services/data.service.ts` - Orchestration (430+ lines)

---

## 🔗 Documentation Links

- **Complete System Status**: `SYSTEM_STATUS.md`
- **MQTT Topic Alignment**: `MQTT_TOPIC_ALIGNMENT_COMPLETE.md`
- **Frontend Services Guide**: `IOT-Frontend-main/docs/SERVICES_README.md`
- **Frontend Quick Reference**: `IOT-Frontend-main/docs/FRONTEND_QUICK_REFERENCE.md`
- **PRD**: `docs/ProductRequirementDocument.md`
- **Build & Test**: `BUILD_AND_TEST.md`
- **Deployment**: `DEPLOYMENT.md`

---

## ✅ Pre-Flight Checklist

### Before Deployment
- [ ] MQTT broker running and accessible
- [ ] MongoDB running
- [ ] Backend compiled without errors
- [ ] Frontend compiled without errors
- [ ] Firmware compiled without errors (coordinator + node)
- [ ] WiFi credentials configured in firmware
- [ ] MQTT credentials match across all layers
- [ ] siteId consistent (default: `site001`)

### After Deployment
- [ ] Backend health endpoint responds
- [ ] Frontend loads without errors
- [ ] MQTT broker accepting connections
- [ ] Coordinator connects to WiFi
- [ ] Coordinator connects to MQTT broker
- [ ] Coordinator publishes initial telemetry
- [ ] Backend receives telemetry
- [ ] Frontend displays coordinator status
- [ ] Node pairing works
- [ ] Node telemetry appears in frontend
- [ ] Brightness commands reach nodes
- [ ] mmWave presence detection triggers lights

---

## 🆘 Emergency Contacts

### Serial Commands (If Implemented)
```
reset           - Restart coordinator
clear_nodes     - Remove all paired nodes
wifi_config     - Reconfigure WiFi
mqtt_config     - Reconfigure MQTT broker
status          - Show system status
```

### Factory Reset
1. Hold pairing button for 10+ seconds
2. All nodes cleared from NVS
3. LED strip shows red confirmation
4. Release button to continue

---

**Quick Reference Version**: 1.0  
**Last Updated**: 2024-01-XX  
**Status**: ✅ PRODUCTION READY
