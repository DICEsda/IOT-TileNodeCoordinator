# Auto-Pairing Feature for Nodes

## Overview
Nodes now automatically enter pairing mode when they are not connected to a coordinator, eliminating the need for manual button presses in most scenarios.

## Behavior

### On Boot (Unpaired Node)
**Previous Behavior:**
- Node boots and waits idle
- User must hold button for 2 seconds to enter pairing mode
- LED shows idle status

**New Behavior:**
- Node boots and detects it has no stored configuration (no `NODE_ID` or `LIGHT_ID`)
- **Automatically enters pairing mode** immediately
- LED starts showing pairing animation (pulsing blue)
- Sends join requests to find coordinator
- Serial output: `"Node: unpaired. Auto-entering pairing mode."`

### During Operation (Connection Lost)
**Previous Behavior:**
- If coordinator goes offline or is powered off
- Node continues in operational state
- LED shows idle/disconnected status
- Node remains disconnected indefinitely
- User must manually hold button for 2s to re-pair

**New Behavior:**
- Node monitors link health via `isLinkAlive()` (checks for activity within last 10 seconds)
- If link is dead for **30 consecutive seconds**:
  - Node automatically switches from `OPERATIONAL` to `PAIRING` state
  - Enters pairing mode automatically
  - LED starts pairing animation
  - Sends join requests to find coordinator
  - Serial output: `"Link dead for 30s - auto-entering pairing mode"`
- If link recovers before 30 seconds, timer resets

### Manual Pairing (Still Supported)
- User can still manually trigger pairing at any time by holding the button for 2 seconds
- This works in both operational and idle states
- Useful for re-pairing to a different coordinator

## Implementation Details

### Code Changes in `node/src/main.cpp`

#### 1. Auto-Pairing on Boot (Line ~381)
```cpp
// Determine initial state
if (config.exists(ConfigKeys::NODE_ID) && config.exists(ConfigKeys::LIGHT_ID)) {
    currentState = NodeState::OPERATIONAL;
    leds.setStatus(LedController::StatusMode::Idle);
    Serial.println("Node: OPERATIONAL (awaiting link)");
} else {
    // No configuration - automatically enter pairing mode
    currentState = NodeState::PAIRING;
    startPairing();  // ← NEW: Auto-start pairing
    Serial.println("Node: unpaired. Auto-entering pairing mode.");
}
```

#### 2. Auto-Pairing on Connection Loss (Line ~414)
```cpp
// Auto-enter pairing if operational but link is dead for >30 seconds
if (currentState == NodeState::OPERATIONAL && !inPairingMode) {
    static uint32_t linkLostTime = 0;
    
    if (!isLinkAlive()) {
        if (linkLostTime == 0) {
            linkLostTime = millis();
        } else if (millis() - linkLostTime > 30000) { // 30 seconds without link
            Serial.println("Link dead for 30s - auto-entering pairing mode");
            currentState = NodeState::PAIRING;
            startPairing();
            linkLostTime = 0;
        }
    } else {
        linkLostTime = 0; // Reset timer when link is alive
    }
}
```

## Link Health Monitoring

### How Link Health is Tracked
The node considers a link "alive" if:
- Any ESP-NOW message is received from coordinator, OR
- A successful unicast transmission acknowledgment is received

The `lastLinkActivityMs` timestamp is updated on these events.

### Link Alive Definition
```cpp
bool SmartTileNode::isLinkAlive() const {
    if (lastLinkActivityMs == 0) return false;
    return (millis() - lastLinkActivityMs) < 10000U; // 10 second window
}
```

### Auto-Pairing Threshold
- **10 seconds**: Link is considered "dead" (LED shows idle/disconnected)
- **30 seconds**: Auto-pairing triggers (node actively searches for coordinator)

This two-tier approach prevents false positives from brief network issues while ensuring timely recovery.

## User Experience

### Scenario 1: Fresh Node
1. Flash new firmware to node
2. Power on node
3. **Node automatically starts pairing** (pulsing blue LED)
4. Open coordinator pairing window
5. Node pairs automatically
6. LED shows connected status (green)

**User interaction:** None required!

### Scenario 2: Coordinator Power Loss
1. Node is operational and connected
2. Coordinator is powered off or crashes
3. **After 30 seconds**, node automatically re-enters pairing mode
4. Power on coordinator
5. Open coordinator pairing window
6. Node pairs automatically
7. Connection restored

**User interaction:** None required (assuming coordinator pairing is active)!

### Scenario 3: Manual Re-Pairing
1. Node is operational
2. User wants to pair to different coordinator
3. **Hold button for 2 seconds**
4. Node enters pairing mode
5. Pair to new coordinator

**User interaction:** Manual button press (as before)

## Testing

### Test 1: Unpaired Node Boot
```bash
# Flash node firmware
cd node
pio run -e esp32-c3-mini-1 -t upload -t monitor

# Expected Serial Output:
# "Smart Tile Node starting..."
# "TMP177 temperature sensor initialized"
# "ESP-NOW V2.0 INITIALIZATION (NODE)"
# "Node: unpaired. Auto-entering pairing mode."
# "Pairing mode started"
# "JOIN_REQUEST sent (XXX bytes), attempts=1"
```

**Expected LED:** Pulsing blue (pairing animation)

### Test 2: Connection Loss Recovery
```bash
# With paired node operational:
# 1. Power off coordinator
# 2. Wait 30 seconds
# 3. Observe node serial output

# Expected Serial Output (after 30s):
# "Link dead for 30s - auto-entering pairing mode"
# "Pairing mode started"
# "JOIN_REQUEST sent (XXX bytes), attempts=1"
```

**Expected LED:** Changes from idle to pulsing blue

### Test 3: Manual Pairing Still Works
```bash
# With node in any state:
# 1. Hold button for 2 seconds
# 2. Release button

# Expected Serial Output:
# "Pairing mode started"
# "JOIN_REQUEST sent (XXX bytes), attempts=1"
```

**Expected LED:** Pulsing blue (pairing animation)

## Configuration

### Pairing Window Duration
Default: 120 seconds (2 minutes)

Can be configured via `ConfigKeys::PAIRING_WINDOW_S` in NVS:
```cpp
config.setInt(ConfigKeys::PAIRING_WINDOW_S, 180); // 3 minutes
```

### Link Dead Timeout
Default: 30 seconds

To modify, change the threshold in `loop()`:
```cpp
} else if (millis() - linkLostTime > 30000) { // Change this value
```

Recommended values:
- **15 seconds**: More aggressive (faster recovery, but may trigger on brief outages)
- **30 seconds**: Balanced (current default)
- **60 seconds**: Conservative (less false positives, slower recovery)

### Link Alive Window
Default: 10 seconds

To modify, change in `isLinkAlive()`:
```cpp
return (millis() - lastLinkActivityMs) < 10000U; // Change this value
```

## Coordinator Compatibility

This feature is fully compatible with existing coordinator pairing logic:

1. **Coordinator Pairing Window**: Must be opened manually or via MQTT command
2. **Node Join Requests**: Sent as broadcasts (same as before)
3. **Coordinator Response**: Sends `JOIN_ACCEPT` (same as before)
4. **ESP-NOW Encryption**: Established after pairing (same as before)

No coordinator changes are required for this feature to work.

## Benefits

1. **Zero-touch onboarding**: Flash and power on - node pairs automatically
2. **Automatic recovery**: Node reconnects after coordinator reboot/crash
3. **Better UX**: No need to remember button press procedure
4. **Network resilience**: System self-heals after temporary failures
5. **Still flexible**: Manual pairing available for advanced scenarios

## Troubleshooting

### Node stuck in pairing mode
**Symptoms:** LED pulsing blue indefinitely, no connection
**Causes:**
- Coordinator pairing window not open
- Node and coordinator on different ESP-NOW channels
- Too much distance between node and coordinator
- RF interference

**Solutions:**
1. Open coordinator pairing window: Button press or MQTT command
2. Verify both on channel 1 (default)
3. Move node closer to coordinator
4. Check serial output for ESP-NOW errors

### Node keeps re-entering pairing after successful pair
**Symptoms:** Node pairs successfully but drops back to pairing mode
**Causes:**
- Coordinator not sending periodic messages
- ESP-NOW encryption issues
- Node not updating `lastLinkActivityMs`

**Solutions:**
1. Verify coordinator is sending telemetry requests or status updates
2. Check ESP-NOW LMK encryption is established
3. Monitor `lastLinkActivityMs` value in serial output

### Node doesn't auto-pair on boot
**Symptoms:** Fresh node boots but stays idle
**Causes:**
- NVS has stale configuration from previous pairing
- `startPairing()` function not being called

**Solutions:**
1. Erase flash completely: `pio run -e esp32-c3-mini-1 -t erase`
2. Reflash firmware
3. Check serial output for "Auto-entering pairing mode" message

## Serial Output Reference

### Normal Boot Sequence (Unpaired)
```
Smart Tile Node starting...
TMP177 temperature sensor initialized
ESP-NOW V2.0 INITIALIZATION (NODE)
[1/9] Setting WiFi mode to STA...
[2/9] Getting MAC address...
MAC Address: AA:BB:CC:DD:EE:FF
[3/9] Initializing ESP-NOW...
[4/9] Setting PMK...
[5/9] Adding broadcast peer...
[6/9] ESP-NOW initialization complete
Node: unpaired. Auto-entering pairing mode.
Pairing mode started
JOIN_REQUEST sent (245 bytes), attempts=1
```

### Normal Boot Sequence (Paired)
```
Smart Tile Node starting...
TMP177 temperature sensor initialized
ESP-NOW V2.0 INITIALIZATION (NODE)
[1/9] Setting WiFi mode to STA...
[2/9] Getting MAC address...
MAC Address: AA:BB:CC:DD:EE:FF
[3/9] Initializing ESP-NOW...
[4/9] Setting PMK...
[5/9] Adding broadcast peer...
[6/9] ESP-NOW initialization complete
Node: OPERATIONAL (awaiting link)
```

### Auto-Pairing on Connection Loss
```
[... operational messages ...]
Link dead for 30s - auto-entering pairing mode
Pairing mode started
JOIN_REQUEST sent (245 bytes), attempts=1
```

## Related Documentation

- **ESP-NOW Messages**: `shared/src/EspNowMessage.h`
- **Pairing Protocol**: `docs/development/ESP_NOW_V2_CONVERSION_COMPLETE.md`
- **Node Configuration**: `shared/src/ConfigManager.h`
- **LED Status Modes**: `node/src/led/LedController.h`

