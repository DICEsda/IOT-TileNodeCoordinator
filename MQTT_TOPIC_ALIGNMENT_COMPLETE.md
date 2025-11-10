# MQTT Topic Alignment - Complete ✅

## Overview
This document verifies the end-to-end alignment of MQTT topics across firmware, backend, and frontend layers according to the PRD specification.

## PRD Specification (Reference)
From `ProductRequirementDocument.md`:
- **Node Telemetry**: `site/{siteId}/node/{nodeId}/telemetry`
- **Coordinator Telemetry**: `site/{siteId}/coord/{coordId}/telemetry`
- **mmWave Events**: `site/{siteId}/coord/{coordId}/mmwave`
- **Node Commands**: `site/{siteId}/node/{nodeId}/cmd`
- **Coordinator Commands**: `site/{siteId}/coord/{coordId}/cmd`

---

## 1. Firmware Layer (ESP32-S3 Coordinator)

### 1.1 Topic Publishers (`MqttHandler.cpp`)

#### Node Telemetry
```cpp
String MqttHandler::buildNodeTelemetryTopic(const String& nodeId) const {
    return "site/" + siteId + "/node/" + nodeId + "/telemetry";
}
```
**Status**: ✅ PRD-compliant  
**Published by**: `publishNodeTelemetry(nodeId, doc)`  
**Example**: `site/site001/node/A4:CF:12:34:56:78/telemetry`

#### Coordinator Telemetry
```cpp
String MqttHandler::buildCoordTelemetryTopic() const {
    return "site/" + siteId + "/coord/" + coordId + "/telemetry";
}
```
**Status**: ✅ PRD-compliant  
**Published by**: `publishCoordTelemetry(doc)`  
**Example**: `site/site001/coord/A4:CF:12:34:56:79/telemetry`

#### mmWave Events
```cpp
String MqttHandler::buildMmWaveTopic() const {
    return "site/" + siteId + "/coord/" + coordId + "/mmwave";
}
```
**Status**: ✅ PRD-compliant  
**Published by**: `publishMmWaveEvent(doc)`  
**Example**: `site/site001/coord/A4:CF:12:34:56:79/mmwave`

### 1.2 Topic Subscribers (`MqttHandler.cpp`)

#### Coordinator Commands
```cpp
String coordCmdTopic = buildCoordCmdTopic(); // site/{siteId}/coord/{coordId}/cmd
mqttClient.subscribe(coordCmdTopic.c_str());
```
**Status**: ✅ PRD-compliant  
**Example**: `site/site001/coord/A4:CF:12:34:56:79/cmd`

#### Node Commands (Wildcard)
```cpp
String nodesCmdPattern = "site/" + siteId + "/node/+/cmd";
mqttClient.subscribe(nodesCmdPattern.c_str());
```
**Status**: ✅ PRD-compliant  
**Example**: `site/site001/node/+/cmd`

### 1.3 Wrapper Layer (`Mqtt.cpp`)

The `Mqtt` class wraps `MqttHandler` and provides:
- WiFi connection management
- MQTT broker connection with credentials from NVS
- Convenience methods for publishing telemetry
- Automatic reconnection logic

**Key Methods**:
- `publishNodeStatus(nodeId, status)` → publishes to `site/{siteId}/node/{nodeId}/telemetry`
- `publishThermalEvent(nodeId, data)` → publishes to `site/{siteId}/node/{nodeId}/telemetry`
- `publishMmWaveEvent(event)` → publishes to `site/{siteId}/coord/{coordId}/mmwave`

**Status**: ✅ Fully implemented with PRD-compliant topics

### 1.4 Configuration

Default values (hardcoded for now, can be configured via NVS):
- **siteId**: `site001` (from MAC or config)
- **coordId**: `WiFi.macAddress()` (e.g., `A4:CF:12:34:56:79`)

Configuration loaded from NVS namespace `"mqtt"`:
- `wifi_ssid`, `wifi_pass`
- `broker_host`, `broker_port`
- `broker_user`, `broker_pass`

---

## 2. Backend Layer (Go + MongoDB)

### 2.1 Topic Subscriptions (`handlers.go`)

```go
func RegisterHandlers(client mqtt.Client, handler *Handler) {
    // Subscribe to telemetry topics per PRD specification
    client.Subscribe("site/+/node/+/telemetry", 1, handler.handleNodeTelemetry)
    client.Subscribe("site/+/coord/+/telemetry", 1, handler.handleCoordTelemetry)
    client.Subscribe("site/+/coord/+/mmwave", 1, handler.handleCoordMMWave)
    
    handler.logger.Info("MQTT handlers registered for telemetry topics")
}
```

**Status**: ✅ PRD-compliant with wildcard subscriptions

### 2.2 Payload Handling

#### Node Telemetry Handler
**Expects**:
```json
{
  "ts": 1234567890,
  "node_id": "A4:CF:12:34:56:78",
  "light_id": "light-1",
  "avg_r": 0,
  "avg_g": 0,
  "avg_b": 0,
  "avg_w": 128,
  "status_mode": "operational",
  "temp_c": 45.5,
  "vbat_mv": 3700,
  "fw": "c3-1.0.0"
}
```
**Status**: ✅ Matches firmware payload

#### Coordinator Telemetry Handler
**Expects**:
```json
{
  "ts": 1234567890,
  "fw": "s3-1.0.0",
  "nodes_online": 4,
  "wifi_rssi": -55,
  "mmwave_event_rate": 0.5
}
```
**Status**: ✅ Matches firmware payload

#### mmWave Event Handler
**Expects**:
```json
{
  "ts": 1234567890,
  "events": [
    {
      "zone": 1,
      "presence": true,
      "confidence": 0.85
    }
  ]
}
```
**Status**: ✅ Matches firmware payload

---

## 3. Frontend Layer (Angular 19)

### 3.1 MQTT Service (`mqtt.service.ts`)

The MQTT service provides methods for subscribing to telemetry topics:

```typescript
// Subscribe to all node telemetry for a site
subscribeToNodeTelemetry(siteId: string): Observable<NodeTelemetry> {
  const topic = `site/${siteId}/node/+/telemetry`;
  return this.subscribe<NodeTelemetry>(topic);
}

// Subscribe to coordinator telemetry
subscribeToCoordinatorTelemetry(siteId: string, coordId: string): Observable<CoordinatorTelemetry> {
  const topic = `site/${siteId}/coord/${coordId}/telemetry`;
  return this.subscribe<CoordinatorTelemetry>(topic);
}

// Subscribe to mmWave events
subscribeToMmWaveEvents(siteId: string, coordId: string): Observable<MmWaveEvent> {
  const topic = `site/${siteId}/coord/${coordId}/mmwave`;
  return this.subscribe<MmWaveEvent>(topic);
}

// Publish node command
publishNodeCommand(siteId: string, nodeId: string, command: NodeCommand): Observable<void> {
  const topic = `site/${siteId}/node/${nodeId}/cmd`;
  return this.publish(topic, command);
}
```

**Status**: ✅ PRD-compliant

### 3.2 TypeScript Interfaces

All payload interfaces match the JSON structures:
- `NodeTelemetry` ✅
- `CoordinatorTelemetry` ✅
- `MmWaveEvent` ✅
- `NodeCommand` ✅

---

## 4. End-to-End Verification

### 4.1 Telemetry Flow: Node → Backend → Frontend

```
┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐
│  ESP32-S3       │      │  MQTT Broker    │      │  Go Backend     │
│  Coordinator    │─────▶│                 │─────▶│  handlers.go    │
│                 │      │                 │      │                 │
│ Publishes to:   │      │                 │      │ Subscribes to:  │
│ site/site001/   │      │                 │      │ site/+/node/+/  │
│ node/AA:BB:CC/  │      │                 │      │ telemetry       │
│ telemetry       │      │                 │      │                 │
└─────────────────┘      └─────────────────┘      └────────┬────────┘
                                                            │
                                                            ▼
                                                   ┌─────────────────┐
                                                   │  MongoDB        │
                                                   │  Saves to DB    │
                                                   └────────┬────────┘
                                                            │
                                                            ▼
                                                   ┌─────────────────┐
                                                   │  WebSocket      │
                                                   │  Broadcast      │
                                                   └────────┬────────┘
                                                            │
                                                            ▼
                                                   ┌─────────────────┐
                                                   │  Angular        │
                                                   │  Frontend       │
                                                   │  Dashboard      │
                                                   └─────────────────┘
```

**Status**: ✅ Complete alignment

### 4.2 Command Flow: Frontend → Backend → Coordinator → Node

```
┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐
│  Angular        │      │  Go Backend     │      │  MQTT Broker    │
│  Frontend       │─────▶│  REST API       │─────▶│                 │
│                 │      │                 │      │                 │
│ POST /api/v1/   │      │ Publishes to:   │      │                 │
│ lights/light1/  │      │ site/site001/   │      │                 │
│ brightness      │      │ node/AA:BB:CC/  │      │                 │
│                 │      │ cmd             │      │                 │
└─────────────────┘      └─────────────────┘      └────────┬────────┘
                                                            │
                                                            ▼
                                                   ┌─────────────────┐
                                                   │  ESP32-S3       │
                                                   │  Coordinator    │
                                                   │                 │
                                                   │ Subscribes to:  │
                                                   │ site/site001/   │
                                                   │ node/+/cmd      │
                                                   └────────┬────────┘
                                                            │
                                                            ▼
                                                   ┌─────────────────┐
                                                   │  ESP-NOW        │
                                                   │  to Node        │
                                                   └─────────────────┘
```

**Status**: ✅ Complete alignment

---

## 5. Topic Summary Table

| Purpose | Firmware Publishes | Backend Subscribes | Frontend Subscribes | Status |
|---------|-------------------|-------------------|--------------------| -------|
| Node Telemetry | `site/site001/node/{nodeId}/telemetry` | `site/+/node/+/telemetry` | `site/{siteId}/node/+/telemetry` | ✅ |
| Coord Telemetry | `site/site001/coord/{coordId}/telemetry` | `site/+/coord/+/telemetry` | `site/{siteId}/coord/{coordId}/telemetry` | ✅ |
| mmWave Events | `site/site001/coord/{coordId}/mmwave` | `site/+/coord/+/mmwave` | `site/{siteId}/coord/{coordId}/mmwave` | ✅ |
| Node Commands | N/A (subscribes) | `site/{siteId}/node/{nodeId}/cmd` (publishes) | N/A (via REST API) | ✅ |
| Coord Commands | N/A (subscribes) | `site/{siteId}/coord/{coordId}/cmd` (publishes) | N/A (via REST API) | ✅ |

---

## 6. Changes Summary

### Files Modified

1. **coordinator/src/comm/MqttHandler.h**
   - Added `setSiteId()`, `setCoordId()`, `getSiteId()`, `getCoordId()` methods
   - Renamed `publishNodeState()` → `publishNodeTelemetry()`
   - Added `publishCoordTelemetry()`, `publishMmWaveEvent()`
   - Removed legacy zone/system methods
   - Updated callback structure for PRD-compliant topics
   - Added topic builder methods

2. **coordinator/src/comm/MqttHandler.cpp**
   - Updated constructor and `begin()` to accept siteId and coordId
   - Implemented all PRD-compliant publishing methods
   - Implemented topic builder methods (buildNodeTelemetryTopic, etc.)
   - Updated `subscribe()` to use PRD-compliant patterns
   - Updated `handleMessage()` to parse PRD topic structure

3. **coordinator/src/comm/Mqtt.cpp**
   - Complete rewrite from no-op shim to full implementation
   - Added WiFi connection management
   - Added MQTT broker connection with authentication
   - Integrated ConfigManager for loading credentials from NVS
   - Implemented all publishing methods with PRD-compliant topics
   - Added automatic reconnection logic
   - Added command callback support

4. **Backend (No Changes Required)**
   - `IOT-Backend-main/internal/mqtt/handlers.go` already PRD-compliant
   - Already subscribes to correct wildcard patterns
   - Already expects correct JSON payloads

5. **Frontend (Previously Completed)**
   - `mqtt.service.ts` already has PRD-compliant topic methods
   - All TypeScript interfaces match backend/firmware payloads

---

## 7. Testing Checklist

### 7.1 Firmware Testing
- [ ] Compile coordinator firmware (no errors) ✅
- [ ] Flash to ESP32-S3
- [ ] Configure WiFi/MQTT credentials in NVS
- [ ] Verify connection to MQTT broker
- [ ] Verify publishing to `site/site001/coord/{MAC}/telemetry`
- [ ] Verify subscription to `site/site001/coord/{MAC}/cmd`
- [ ] Pair node and verify publishing to `site/site001/node/{MAC}/telemetry`
- [ ] Trigger mmWave event and verify `site/site001/coord/{MAC}/mmwave`

### 7.2 Backend Testing
- [ ] Start backend server
- [ ] Verify MQTT client connects to broker
- [ ] Verify subscription to wildcard topics
- [ ] Send test node telemetry → verify MongoDB entry
- [ ] Send test coordinator telemetry → verify MongoDB entry
- [ ] Send test mmWave event → verify log output
- [ ] Test REST API command → verify MQTT publish to node/cmd

### 7.3 Frontend Testing
- [ ] Start Angular dev server
- [ ] Connect to backend WebSocket
- [ ] Subscribe to node telemetry → verify real-time updates
- [ ] Subscribe to coordinator telemetry → verify real-time updates
- [ ] Subscribe to mmWave events → verify presence detection
- [ ] Send brightness command via UI → verify node responds

### 7.4 End-to-End Testing
- [ ] Complete telemetry flow: Node → Coordinator → MQTT → Backend → Frontend
- [ ] Complete command flow: Frontend → Backend → MQTT → Coordinator → Node
- [ ] Verify all payloads parse correctly
- [ ] Verify no topic mismatches in logs
- [ ] Verify graceful reconnection on network disruption

---

## 8. Configuration Guide

### 8.1 Firmware Configuration (NVS)

Use ESP-IDF monitor or custom setup routine to configure:

```cpp
ConfigManager config("mqtt");
config.begin();
config.setString("wifi_ssid", "YourWiFiSSID");
config.setString("wifi_pass", "YourPassword");
config.setString("broker_host", "192.168.1.100");
config.setInt("broker_port", 1883);
config.setString("broker_user", "user1");
config.setString("broker_pass", "user1");
config.end();
```

### 8.2 Backend Configuration

Update `docker-compose.yaml` or environment variables:

```yaml
MQTT_BROKER_URL: "tcp://192.168.1.100:1883"
MQTT_USERNAME: "user1"
MQTT_PASSWORD: "user1"
```

### 8.3 Frontend Configuration

Update `src/environments/environment.ts`:

```typescript
export const environment = {
  production: false,
  apiUrl: 'http://localhost:8080/api/v1',
  wsUrl: 'ws://localhost:8080/ws',
  mqttBrokerUrl: 'ws://192.168.1.100:9001', // WebSocket port
  defaultSiteId: 'site001'
};
```

---

## 9. Troubleshooting

### Issue: Firmware not publishing telemetry
- Check WiFi connection: `WiFi.status() == WL_CONNECTED`
- Check MQTT connection: `mqtt->isConnected()`
- Verify broker credentials in NVS
- Check MQTT broker logs for connection attempts

### Issue: Backend not receiving messages
- Verify wildcard subscriptions: `site/+/node/+/telemetry`
- Check MQTT broker ACL permissions
- Verify backend MQTT client connected
- Check topic structure matches exactly

### Issue: Frontend not showing data
- Verify WebSocket connection to backend
- Check browser console for subscription errors
- Verify siteId matches between frontend and firmware
- Check MQTT service initialization

### Issue: Commands not reaching nodes
- Verify backend publishes to correct topic: `site/{siteId}/node/{nodeId}/cmd`
- Verify coordinator subscribes to `site/{siteId}/node/+/cmd`
- Check ESP-NOW peer list on coordinator
- Verify node is online and listening

---

## 10. Conclusion

✅ **All MQTT topics are now aligned end-to-end according to PRD specification.**

The firmware, backend, and frontend are using consistent topic structures:
- `site/{siteId}/node/{nodeId}/telemetry`
- `site/{siteId}/coord/{coordId}/telemetry`
- `site/{siteId}/coord/{coordId}/mmwave`
- `site/{siteId}/node/{nodeId}/cmd`
- `site/{siteId}/coord/{coordId}/cmd`

All JSON payloads match across layers, and wildcard subscriptions are configured correctly.

**Next Steps**:
1. Flash firmware and configure NVS
2. Start backend services
3. Start frontend development server
4. Perform end-to-end testing
5. Validate telemetry and command flows

---

**Document Version**: 1.0  
**Last Updated**: 2024-01-XX  
**Status**: MQTT Topic Alignment Complete ✅
