# MQTT Pipeline Logging Guide

## Overview
Enhanced logging for MQTT communication pipeline (coordinator) and ESP-NOW communication (nodes) to facilitate debugging, monitoring, and performance analysis.

## Features

### Coordinator MQTT Logger (`MqttLogger`)

**Location**: `coordinator/src/comm/MqttLogger.h`

#### Capabilities
- Connection lifecycle tracking (connect/disconnect)
- Message publish logging with payload inspection
- Message receive logging with topic parsing
- Automatic message type detection (telemetry, commands, events)
- Topic ID extraction (site, coordinator, node)
- Telemetry flow tracking
- Command flow tracking
- Statistics collection
- Performance metrics (latency, throughput)
- Periodic heartbeat logging

#### Log Format Examples

**Connection**:
```
[MQTT] ✓ Connected to broker: 192.168.1.100:1883 as 'coord-AA:BB:CC:DD:EE:FF'
[MQTT] ✓ Subscribed to: site/site001/coord/coord001/cmd
```

**Publishing**:
```
[MQTT→] NodeTelemetry | topic=site/site001/node/node-abc123/telemetry | size=245 bytes
[MQTT→] payload: {"ts":12345,"node_id":"node-abc123","temp_c":23.5...}
[MQTT→] site=site001 node=node-abc123
[MQTT⏱] Latency: NodeStatus took 12 ms
```

**Receiving**:
```
[MQTT←] NodeCommand | topic=site/site001/node/node-abc123/cmd | size=87 bytes
[MQTT←] payload: {"cmd":"set_light","on":true,"brightness":128...}
[MQTT←] site=site001 node=node-abc123
```

**Processing**:
```
[MQTT⚙] Command processed | topic=site/site001/coord/coord001/cmd
[MQTT⏱] Latency: ProcessMessage took 5 ms
```

**Heartbeat** (every 60 seconds):
```
[MQTT💓] Alive | pub=245 recv=12 errors=0
```

### Node ESP-NOW Logger (`EspNowLogger`)

**Location**: `node/src/utils/EspNowLogger.h`

#### Capabilities
- Message transmission logging
- Message reception logging
- Join/pairing flow tracking
- Command reception tracking
- Telemetry submission tracking
- Link health monitoring
- Statistics collection
- Pairing status tracking
- LED control logging
- Temperature sensor logging

#### Log Format Examples

**Sending**:
```
[ESP→] NodeStatus | dest=AA:BB:CC:DD:EE:FF | size=245
[ESP→]   {"msg":"node_status","node_id":"node-abc123"...}
```

**Receiving**:
```
[ESP←] SetLight | src=AA:BB:CC:DD:EE:FF | size=87
[ESP←]   {"msg":"set_light","r":0,"g":255,"b":0...}
```

**Pairing**:
```
[ESP🔗] ▶ Pairing mode STARTED
[ESP🔗]   reason: No configuration found
[ESP🔗] ✓ PAIRED successfully!
[ESP🔗]   node_id:  node-abc123
[ESP🔗]   light_id: light-xyz
[ESP🔗]   coord:    AA:BB:CC:DD:EE:FF
```

**Link Status**:
```
[ESP💓] Connected | sent=150 recv=75 errors=0
[ESP💓] Link ALIVE | last activity 2345 ms ago
```

**Command Processing**:
```
[ESP⚙] ✓ Command: set_light
[ESP⚙]   R=0 G=255 B=0 W=0 brightness=128
[ESP💡] LED set: R=0 G=255 B=0 W=0 brightness=128
```

**Temperature**:
```
[ESP🌡] Temperature: 23.5°C
[ESP📊] Telemetry: temp=23.5°C RGBW=(0,255,0,0) vbat=3300mV
```

## Usage

### Coordinator (MQTT Logger)

The MQTT logger is automatically integrated into `coordinator/src/comm/Mqtt.cpp`.

#### Enable/Disable Logging
```cpp
// Set log level in main.cpp or Coordinator::begin()
Logger::setMinLevel(Logger::DEBUG);  // Show all logs including payloads
Logger::setMinLevel(Logger::INFO);   // Show operations (default)
Logger::setMinLevel(Logger::WARN);   // Show warnings and errors only
```

#### View Statistics
```cpp
// In serial commands or debugging
MqttLogger::printStats();
```

**Output**:
```
========== MQTT Statistics ==========
Messages Published:     245
  - Node Telemetry:     200
  - Coord Telemetry:    40
  - MmWave Events:      5
Messages Received:      12
  - Node Commands:      8
  - Coord Commands:     4
Publish Errors:         0
Parse Errors:           0
Last Publish:           1234 ms ago
Last Receive:           5678 ms ago
====================================
```

#### Reset Statistics
```cpp
MqttLogger::resetStats();
```

#### Manual Logging Calls

**Log custom publish**:
```cpp
String topic = "site/site001/custom/topic";
String payload = "{\"data\":\"value\"}";
bool success = mqttClient.publish(topic.c_str(), payload.c_str());
MqttLogger::logPublish(topic, payload, success);
```

**Log custom processing**:
```cpp
MqttLogger::logProcess(topic, "Custom action", true, "additional detail");
```

**Log ESP-NOW forwarding**:
```cpp
MqttLogger::logForward(nodeId, "set_light", success, "brightness=128");
```

### Node (ESP-NOW Logger)

Integration examples for `node/src/main.cpp`:

#### Log Message Send
```cpp
// In ESP-NOW send callback
void SmartTileNode::sendMessage(const EspNowMessage& msg, const uint8_t* destMac) {
    String json = msg.toJson();
    esp_err_t result = esp_now_send(destMac, (uint8_t*)json.c_str(), json.length());
    
    bool success = (result == ESP_OK);
    EspNowLogger::logSend(destMac, json, success, 
                          success ? nullptr : "ESP-NOW send failed");
}
```

#### Log Message Receive
```cpp
// In ESP-NOW receive callback
void onDataRecv(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    EspNowLogger::logReceive(recv_info->src_addr, data, len);
    
    // Process message...
    EspNowLogger::updateLinkActivity();
}
```

#### Log Pairing Events
```cpp
void SmartTileNode::startPairing() {
    inPairingMode = true;
    EspNowLogger::logPairing(true, "Manual button press");
    // Start pairing...
}

void SmartTileNode::onJoinAccept(const JoinAcceptMessage& msg) {
    EspNowLogger::logPairSuccess(msg.node_id, msg.light_id, coordinatorMac);
    // Save config...
}
```

#### Log Command Processing
```cpp
void SmartTileNode::handleSetLight(const SetLightMessage& msg) {
    EspNowLogger::logCommandProcess("set_light", true, 
                                    "R=" + String(msg.r) + " G=" + String(msg.g));
    
    ledController->setColor(msg.r, msg.g, msg.b, msg.w);
    EspNowLogger::logLedControl(msg.r, msg.g, msg.b, msg.w, msg.value);
}
```

#### Log Temperature Reading
```cpp
void SmartTileNode::readTemperature() {
    float temp = tempSensor.readTemperature();
    bool valid = tempSensor.isInitialized();
    EspNowLogger::logTemperature(temp, valid);
}
```

#### View Statistics
```cpp
// In serial command handler
if (command == "stats") {
    EspNowLogger::printStats();
}
```

#### Periodic Heartbeat
```cpp
void SmartTileNode::loop() {
    // ... existing loop code ...
    
    // Log heartbeat every 30 seconds
    EspNowLogger::logHeartbeat(isPaired(), isLinkAlive(), 30000);
}
```

## Log Symbols Reference

| Symbol | Meaning |
|--------|---------|
| `✓` | Success |
| `✗` | Failure/Error |
| `→` | Outgoing message |
| `←` | Incoming message |
| `⚙` | Processing |
| `💓` | Heartbeat/Status |
| `🔗` | Pairing |
| `💡` | LED control |
| `🌡` | Temperature |
| `📊` | Telemetry |
| `⏱` | Timing/Latency |
| `🔄` | Retry |
| `🔒` | Encryption |
| `⚡` | Power |

## Configuration

### Heartbeat Interval

**Coordinator** (`Mqtt.cpp`):
```cpp
// In loop() method
MqttLogger::logHeartbeat(mqttClient.connected(), 60000); // 60 seconds
```

**Node** (`main.cpp`):
```cpp
// In loop() method
EspNowLogger::logHeartbeat(isPaired(), isLinkAlive(), 30000); // 30 seconds
```

### Payload Truncation

Both loggers automatically truncate long payloads for readability:
- **Display limit**: 100 characters
- **Full payload**: Available at DEBUG log level
- **Hex dump**: Available for binary data at DEBUG level

### Statistics Collection

Statistics are collected automatically and can be accessed anytime:

**Coordinator**:
```cpp
auto& stats = MqttLogger::getStats();
Serial.printf("Total published: %u\n", stats.messagesPublished);
```

**Node**:
```cpp
auto& stats = EspNowLogger::getStats();
Serial.printf("Total sent: %u\n", stats.messagesSent);
```

## Performance Impact

### Coordinator
- **Minimal**: Logging only at INFO level (operations, no payloads)
- **Low**: Logging at DEBUG level (includes truncated payloads)
- **Recommendation**: INFO level for production, DEBUG for troubleshooting

### Node
- **Minimal**: All logging includes buffer size limits
- **Low memory**: No dynamic allocation in log functions
- **Safe**: Logging can be disabled by not calling log functions

## Troubleshooting Use Cases

### Case 1: Messages Not Reaching Backend

**Check coordinator logs**:
```
[MQTT→] NodeTelemetry | topic=site/site001/node/node-abc123/telemetry | size=245 bytes
```

If you see this, message was published successfully. Check backend subscription.

If you don't see this, check:
```
[MQTT💓] Alive | pub=245 recv=12 errors=0
```

If `pub=0`, coordinator isn't receiving node data via ESP-NOW.

### Case 2: Node Not Pairing

**Check node logs**:
```
[ESP🔗] ▶ Pairing mode STARTED
[ESP→] JoinRequest | dest=FF:FF:FF:FF:FF:FF | size=245
```

If you see JoinRequests being sent but no JoinAccept:
```
[ESP←] JoinAccept | src=AA:BB:CC:DD:EE:FF | size=180
```

Check coordinator pairing window is open.

### Case 3: Commands Not Reaching Node

**Check coordinator receives command**:
```
[MQTT←] NodeCommand | topic=site/site001/node/node-abc123/cmd | size=87 bytes
```

**Check coordinator forwards to ESP-NOW**:
```
[MQTT→ESP] Forwarded set_light to node=node-abc123 | brightness=128
```

**Check node receives**:
```
[ESP←] SetLight | src=AA:BB:CC:DD:EE:FF | size=87
[ESP⚙] ✓ Command: set_light
[ESP💡] LED set: R=0 G=255 B=0 W=0 brightness=128
```

### Case 4: High Latency

**Check coordinator**:
```
[MQTT⏱] ⚠ High latency: NodeStatus took 1234 ms
```

Indicates slow MQTT broker or network issues.

**Check node**:
```
[ESP⏱] ⚠ High latency: Send took 567 ms
```

Indicates ESP-NOW congestion or poor RF conditions.

## Best Practices

1. **Production**: Use INFO level logging
2. **Development**: Use DEBUG level logging
3. **Troubleshooting**: Enable DEBUG temporarily, analyze logs, revert to INFO
4. **Statistics**: Check periodically (every few hours) to monitor health
5. **Heartbeat**: Keep enabled to detect silent failures
6. **Reset stats**: After making changes to get clean baseline

## Integration Checklist

### Coordinator
- [x] Include `MqttLogger.h` in `Mqtt.cpp`
- [x] Log connection/disconnection events
- [x] Log all publish operations
- [x] Log all receive operations
- [x] Log processing results
- [x] Add heartbeat to `loop()`
- [ ] Add command forwarding logs (when implemented)

### Node
- [ ] Include `EspNowLogger.h` in `main.cpp`
- [ ] Log all ESP-NOW sends
- [ ] Log all ESP-NOW receives
- [ ] Log pairing events
- [ ] Log command processing
- [ ] Log LED control operations
- [ ] Log temperature readings
- [ ] Add heartbeat to `loop()`

## Example Serial Output

### Coordinator Boot Sequence
```
[LOGGER] initialized and ready
[INFO] Initializing MQTT client...
[INFO] MQTT config loaded from NVS
[MQTT] ✓ Connected to broker: 192.168.1.100:1883 as 'coord-AA11BB22CC33'
[MQTT] ✓ Subscribed to: site/site001/coord/coord001/cmd
[MQTT→] CoordTelemetry | topic=site/site001/coord/coord001/telemetry | size=156 bytes
[MQTT💓] Alive | pub=1 recv=0 errors=0
```

### Node Boot Sequence
```
Smart Tile Node starting...
TMP177 temperature sensor initialized
[ESP🔗] ▶ Pairing mode STARTED
[ESP🔗]   reason: No configuration found
[ESP→] JoinRequest | dest=FF:FF:FF:FF:FF:FF | size=245
[ESP←] JoinAccept | src=AA:11:BB:22:CC:33 | size=180
[ESP🔗] ✓ PAIRED successfully!
[ESP🔗]   node_id:  node-abc123
[ESP🔗]   light_id: light-xyz
[ESP🔗]   coord:    AA:11:BB:22:CC:33
[ESP💓] Connected | sent=1 recv=1 errors=0
```

### Live Operation (Coordinator)
```
[MQTT←] NodeCommand | topic=site/site001/node/node-abc123/cmd | size=87 bytes
[MQTT←] payload: {"cmd":"set_light","on":true,"brightness":128...}
[MQTT⚙] Command processed | topic=site/site001/node/node-abc123/cmd
[MQTT→ESP] Forwarded set_light to node=node-abc123 | brightness=128
[MQTT→] NodeTelemetry | topic=site/site001/node/node-abc123/telemetry | size=245 bytes
[MQTT⏱] Latency: NodeStatus took 8 ms
```

### Live Operation (Node)
```
[ESP←] SetLight | src=AA:11:BB:22:CC:33 | size=87
[ESP←]   {"msg":"set_light","r":0,"g":255,"b":0,"w":0,"value":128}
[ESP⚙] ✓ Command: set_light
[ESP💡] LED set: R=0 G=255 B=0 W=0 brightness=128
[ESP🌡] Temperature: 23.5°C
[ESP📊] Telemetry: temp=23.5°C RGBW=(0,255,0,0) vbat=3300mV
[ESP→] NodeStatus | dest=AA:11:BB:22:CC:33 | size=245
```

## Future Enhancements

Potential additions:
- JSON schema validation logging
- Message rate limiting warnings
- Circular buffer for last N messages
- Export logs to file (SD card)
- Remote log streaming over MQTT
- Performance profiling mode
- Message replay for debugging

