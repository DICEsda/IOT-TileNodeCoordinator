# MQTT/ESP-NOW Logging - Quick Summary

## What Was Added

### 1. Coordinator MQTT Logger
**File**: `coordinator/src/comm/MqttLogger.h` (385 lines)

**Features**:
- ✅ Connection/disconnection tracking
- ✅ Publish logging with payload inspection
- ✅ Receive logging with topic parsing
- ✅ Auto message type detection
- ✅ Statistics collection
- ✅ Heartbeat logging (every 60s)
- ✅ Latency tracking
- ✅ Topic ID extraction (site/coord/node)

**Integration**: `coordinator/src/comm/Mqtt.cpp`
- Added `#include "MqttLogger.h"`
- Logging in `connectMqtt()`, `publishNodeStatus()`, `handleMqttMessage()`, `loop()`

### 2. Node ESP-NOW Logger
**File**: `node/src/utils/EspNowLogger.h` (355 lines)

**Features**:
- ✅ Send/receive logging
- ✅ Pairing flow tracking
- ✅ Command processing tracking
- ✅ Link health monitoring
- ✅ Statistics collection
- ✅ Heartbeat logging (every 30s)
- ✅ LED control logging
- ✅ Temperature sensor logging

**Integration**: Ready to use in `node/src/main.cpp`

## Log Format Examples

### Coordinator
```
[MQTT] ✓ Connected to broker: 192.168.1.100:1883 as 'coord-AA11BB22CC33'
[MQTT→] NodeTelemetry | topic=site/site001/node/node-abc123/telemetry | size=245 bytes
[MQTT←] NodeCommand | topic=site/site001/node/node-abc123/cmd | size=87 bytes
[MQTT⚙] Command processed | topic=site/site001/node/node-abc123/cmd
[MQTT💓] Alive | pub=245 recv=12 errors=0
```

### Node
```
[ESP→] NodeStatus | dest=AA:BB:CC:DD:EE:FF | size=245
[ESP←] SetLight | src=AA:BB:CC:DD:EE:FF | size=87
[ESP🔗] ✓ PAIRED successfully!
[ESP⚙] ✓ Command: set_light
[ESP💡] LED set: R=0 G=255 B=0 W=0 brightness=128
[ESP💓] Connected | sent=150 recv=75 errors=0
```

## Quick Usage

### View Statistics (Coordinator)
```cpp
// In serial command handler or debugging
MqttLogger::printStats();
```

Output:
```
========== MQTT Statistics ==========
Messages Published:     245
  - Node Telemetry:     200
  - Coord Telemetry:    40
Messages Received:      12
  - Node Commands:      8
Publish Errors:         0
Last Publish:           1234 ms ago
====================================
```

### View Statistics (Node)
```cpp
// In serial command handler
EspNowLogger::printStats();
```

Output:
```
========== ESP-NOW Statistics ==========
Paired:                 YES
Messages Sent:          150
  - Join Requests:      3
  - Status Messages:    147
Messages Received:      75
  - Light Commands:     5
Send Errors:            0
Last Send:              1234 ms ago
========================================
```

## Control Log Verbosity

```cpp
// Show everything (payloads, IDs, timings)
Logger::setMinLevel(Logger::DEBUG);

// Show operations only (default)
Logger::setMinLevel(Logger::INFO);

// Show warnings and errors only
Logger::setMinLevel(Logger::WARN);
```

## Integration Steps

### Coordinator (DONE ✅)
1. Created `MqttLogger.h`
2. Included in `Mqtt.cpp`
3. Added logging to:
   - `connectMqtt()` - connection events
   - `publishNodeStatus()` - telemetry publishing
   - `handleMqttMessage()` - incoming messages
   - `processMessage()` - command processing
   - `loop()` - heartbeat logging

### Node (TODO)
1. Created `EspNowLogger.h`
2. Include in `main.cpp`:
   ```cpp
   #include "utils/EspNowLogger.h"
   ```
3. Add logging to:
   - ESP-NOW send callback
   - ESP-NOW receive callback
   - Pairing start/success/failure
   - Command processing
   - LED control operations
   - Temperature readings
   - `loop()` - heartbeat

**Example integration snippets provided in `docs/MQTT_LOGGING_GUIDE.md`**

## Testing

### Coordinator
```bash
cd coordinator
pio run -e esp32-s3-devkitc-1 -t upload -t monitor

# Expected output on boot:
# [MQTT] ✓ Connected to broker...
# [MQTT] ✓ Subscribed to: site/site001/coord/coord001/cmd
# [MQTT→] CoordTelemetry | topic=... | size=...
```

### Node
```bash
cd node
pio run -e esp32-c3-mini-1 -t upload -t monitor

# Expected output on boot:
# [ESP🔗] ▶ Pairing mode STARTED
# [ESP→] JoinRequest | dest=FF:FF:FF:FF:FF:FF | size=245
# [ESP←] JoinAccept | src=...
# [ESP🔗] ✓ PAIRED successfully!
```

## Benefits

1. **Debugging**: Clear view of message flow
2. **Monitoring**: Real-time system health (heartbeat, stats)
3. **Diagnostics**: Identify bottlenecks (latency tracking)
4. **Troubleshooting**: Trace issues end-to-end
5. **Performance**: Track throughput and errors
6. **Development**: Understand system behavior

## Troubleshooting Use Cases

### Messages not reaching backend?
Check coordinator:
```
[MQTT→] NodeTelemetry | ... ✓
```
If present, MQTT publish succeeded. Check backend subscription.

### Node not pairing?
Check node:
```
[ESP→] JoinRequest | ...
[ESP←] JoinAccept | ...
```
If JoinRequest sent but no JoinAccept received, coordinator pairing window not open.

### Commands not reaching node?
Check sequence:
1. Coordinator: `[MQTT←] NodeCommand | ...`
2. Coordinator: `[MQTT→ESP] Forwarded set_light to node=...`
3. Node: `[ESP←] SetLight | ...`
4. Node: `[ESP⚙] ✓ Command: set_light`

Missing any step? That's where the issue is.

### High latency?
Check:
```
[MQTT⏱] ⚠ High latency: NodeStatus took 1234 ms
[ESP⏱] ⚠ High latency: Send took 567 ms
```

## Log Symbols Quick Reference

| Symbol | Meaning |
|--------|---------|
| `✓` | Success |
| `✗` | Failure |
| `→` | Outgoing |
| `←` | Incoming |
| `⚙` | Processing |
| `💓` | Heartbeat |
| `🔗` | Pairing |
| `💡` | LED |
| `🌡` | Temperature |
| `📊` | Telemetry |
| `⏱` | Timing |

## Files Created

1. `coordinator/src/comm/MqttLogger.h` - Coordinator MQTT logger
2. `node/src/utils/EspNowLogger.h` - Node ESP-NOW logger
3. `docs/MQTT_LOGGING_GUIDE.md` - Complete guide (400+ lines)
4. `docs/LOGGING_SUMMARY.md` - This summary

## Files Modified

1. `coordinator/src/comm/Mqtt.cpp` - Integrated MqttLogger

## Documentation

- **Full Guide**: `docs/MQTT_LOGGING_GUIDE.md`
- **This Summary**: `docs/LOGGING_SUMMARY.md`

## Next Steps

1. **Test coordinator logging**: Flash and verify MQTT logs
2. **Integrate node logging**: Add log calls to `node/src/main.cpp`
3. **Run end-to-end test**: Observe full message flow
4. **Adjust log levels**: Set to INFO for production

---

**Status**: Coordinator logging integrated and ready to test. Node logging created and documented, ready to integrate.
