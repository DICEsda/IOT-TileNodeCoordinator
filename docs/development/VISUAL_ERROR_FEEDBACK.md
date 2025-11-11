# Visual Error Feedback

## Overview
Added red LED flash on coordinator when ESP-NOW message sending fails, providing immediate visual feedback for communication errors.

## Changes Made

### 1. Added Send Error Callback to EspNow
**Files:** 
- `coordinator/src/comm/EspNow.h`
- `coordinator/src/comm/EspNow.cpp`

**New API:**
```cpp
void setSendErrorCallback(std::function<void(const String& nodeId)> callback);
```

**Behavior:**
- Triggered in `staticSendCallback()` when `status != ESP_NOW_SEND_SUCCESS`
- Passes the node MAC address that failed
- Called from ESP-NOW's internal callback context

### 2. Wired Up Red Flash in Coordinator
**File:** `coordinator/src/core/Coordinator.cpp`

**Implementation:**
```cpp
espNow->setSendErrorCallback([this](const String& nodeId) {
    statusLed.pulse(180, 0, 0, 200); // Red flash for 200ms
    Logger::warn("ESP-NOW send failed to node %s - showing red flash", nodeId.c_str());
});
```

## Visual Feedback

### Error Indication:
- **Color:** Red (RGB: 180, 0, 0)
- **Duration:** 200ms
- **Trigger:** Any ESP-NOW send failure (status != 0)
- **Affects:** All coordinator LEDs (whole strip)

### Error Codes:
| Status | Meaning |
|--------|---------|
| 0 | Success (no flash) |
| 1 | Send failed (red flash) |
| Other | Various errors (red flash) |

## When You'll See Red Flashes

### Common Scenarios:

1. **Node Out of Range**
   - Node too far from coordinator (>10m)
   - Physical obstruction blocking signal
   - **Fix:** Move devices closer

2. **Node Powered Off**
   - Trying to send to disconnected node
   - Node rebooted or crashed
   - **Fix:** Check node power and logs

3. **Channel Mismatch**
   - Devices on different WiFi channels
   - Should not happen (both on channel 1)
   - **Fix:** Restart both devices

4. **Peer Not Registered**
   - ESP-NOW peer table full
   - Node MAC not added to coordinator
   - **Fix:** Clear peers (10s hold) and re-pair

5. **Buffer/Queue Full**
   - Coordinator sending too fast
   - ESP-NOW internal queue overflow
   - **Fix:** Temporary, should recover

## Other Visual Indicators

For comparison, here are all the coordinator LED patterns:

| Event | Color | Duration | Trigger |
|-------|-------|----------|---------|
| Pairing window open | Blue | 500ms | Button short press |
| Node paired | Green | 300ms | Successful pairing |
| Send error | **Red** | **200ms** | **ESP-NOW send fails** |
| Per-node connected | Green | Continuous | Node sending telemetry |
| Per-node activity | Bright green | 150ms | Message received |
| Per-node disconnected | Red | Continuous | Node timeout |

## Troubleshooting

### Too Many Red Flashes:
**Symptom:** Constant red flashing
**Cause:** Node consistently unreachable
**Solution:**
1. Check node is powered on and paired
2. Move node closer to coordinator
3. Check node logs for issues
4. Re-pair the node

### Red Flash During Normal Operation:
**Symptom:** Occasional red flash while node seems connected
**Cause:** Intermittent packet loss (normal in RF)
**Solution:** 
- If rare (<1/minute): Normal, ignore
- If frequent (>5/minute): Check distance/obstacles

### Red Flash Right After Pairing:
**Symptom:** Red flash immediately after green "paired" confirmation
**Cause:** First telemetry send failed
**Solution:** 
- Node may still be processing pairing
- Should recover on next telemetry (5s)
- If persists, re-pair the node

## Debug Information

When red flash occurs, coordinator logs:
```
ESP-NOW V2: send_cb to 10:00:3B:01:98:BC FAILED (status=1)
ESP-NOW send failed to node 10:00:3B:01:98:BC - showing red flash
```

This helps correlate the visual indication with the specific node having issues.

## Benefits

1. **Immediate Feedback:** See communication problems instantly
2. **No Need to Check Logs:** Visual indication is faster
3. **Multiple Node Support:** See which node is having issues (via logs)
4. **Non-Intrusive:** Brief 200ms flash doesn't disrupt other indicators

## Configuration

To change flash behavior, edit `Coordinator.cpp` line 62:

```cpp
// Change color (RGB values 0-255):
statusLed.pulse(180, 0, 0, 200);  // Current: Red
statusLed.pulse(255, 100, 0, 200); // Orange
statusLed.pulse(180, 0, 180, 200); // Purple

// Change duration (milliseconds):
statusLed.pulse(180, 0, 0, 300);  // Longer (300ms)
statusLed.pulse(180, 0, 0, 100);  // Shorter (100ms)
```

## Performance Impact

- **Minimal:** Callback is lightweight
- **No blocking:** LED pulse is non-blocking
- **No retry impact:** Error callback doesn't affect retry logic
- **Memory:** Single function pointer (~4 bytes)
