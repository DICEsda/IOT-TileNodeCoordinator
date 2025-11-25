# Auto-Pairing Feature - Quick Summary

## What Changed

Nodes now **automatically enter pairing mode** when:
1. **On boot** if they have no stored configuration (unpaired)
2. **After 30 seconds** of no connection to coordinator (connection lost)

## User Benefits

- ✅ **Zero-touch setup**: Flash firmware → power on → node pairs automatically
- ✅ **Self-healing**: Node reconnects after coordinator reboot/crash
- ✅ **No button press needed** for most scenarios
- ✅ Manual pairing still available (hold button 2s)

## Code Changes

**File**: `node/src/main.cpp`

### Change 1: Auto-pair on boot (line ~381)
```cpp
} else {
    // No configuration - automatically enter pairing mode
    currentState = NodeState::PAIRING;
    startPairing();  // ← NEW
    Serial.println("Node: unpaired. Auto-entering pairing mode.");
}
```

### Change 2: Auto-pair on connection loss (line ~414)
```cpp
// Auto-enter pairing if operational but link is dead for >30 seconds
if (currentState == NodeState::OPERATIONAL && !inPairingMode) {
    static uint32_t linkLostTime = 0;
    
    if (!isLinkAlive()) {
        if (linkLostTime == 0) {
            linkLostTime = millis();
        } else if (millis() - linkLostTime > 30000) { // 30 seconds
            Serial.println("Link dead for 30s - auto-entering pairing mode");
            currentState = NodeState::PAIRING;
            startPairing();
            linkLostTime = 0;
        }
    } else {
        linkLostTime = 0; // Reset when link is alive
    }
}
```

## Testing

### Test Fresh Node
```bash
cd node
pio run -e esp32-c3-mini-1 -t upload -t monitor

# Expected: LED pulses blue immediately
# Serial: "Node: unpaired. Auto-entering pairing mode."
```

### Test Connection Loss
1. Have paired node operational
2. Power off coordinator
3. Wait 30 seconds
4. **Expected**: Node LED starts pulsing blue
5. **Serial**: "Link dead for 30s - auto-entering pairing mode"

## LED Status Guide

| LED Pattern | Meaning |
|-------------|---------|
| Pulsing Blue | Pairing mode (searching for coordinator) |
| Solid Green | Connected to coordinator |
| Off/Dim | Idle (no connection, not pairing) |

## Configuration

### Timeouts
- **Link considered dead**: 10 seconds (no activity)
- **Auto-pairing triggers**: 30 seconds (consecutive dead time)
- **Pairing window duration**: 120 seconds (2 minutes)

### To Modify
Edit `node/src/main.cpp`:
```cpp
} else if (millis() - linkLostTime > 30000) { // Change this value
```

Recommended:
- 15s: Aggressive (faster recovery)
- 30s: Balanced (default)
- 60s: Conservative (fewer false positives)

## Backward Compatibility

✅ **Fully compatible** with existing coordinators
- No coordinator changes needed
- Same pairing protocol
- Same ESP-NOW messages
- Manual pairing still works

## Quick Reference

### Scenarios

| Scenario | Old Behavior | New Behavior |
|----------|-------------|--------------|
| Fresh node boot | Waits idle, needs button | **Auto-pairs immediately** |
| Coordinator offline 30s+ | Stays disconnected | **Auto-pairs automatically** |
| Want to re-pair manually | Hold button 2s | Same (still works) |

### Serial Messages

| Message | Meaning |
|---------|---------|
| `"Node: unpaired. Auto-entering pairing mode."` | Fresh boot, no config |
| `"Link dead for 30s - auto-entering pairing mode"` | Connection lost |
| `"Pairing mode started"` | Pairing window active |
| `"Node: OPERATIONAL (awaiting link)"` | Paired, waiting for coordinator |

## Documentation

- **Full guide**: `docs/AUTO_PAIRING_FEATURE.md`
- **ESP-NOW messages**: `shared/src/EspNowMessage.h`
- **Pairing protocol**: `docs/development/ESP_NOW_V2_CONVERSION_COMPLETE.md`
