# Connectivity Fixes

## Issues Found and Fixed

### 1. Node Not Processing JOIN_ACCEPT Messages
**Problem:** Node was searching for `"\"join_accept\""` (with escaped quotes) instead of just `"join_accept"` in the JSON message.

**Fix:** Changed string detection to use simple `indexOf("join_accept")` and pre-compute the check before filtering logic.

**Files Modified:**
- `node/src/main.cpp` (lines 701, 722, 728)

### 2. Node Not Auto-Starting Pairing Mode
**Problem:** When unpaired, node set `currentState = PAIRING` but never set `inPairingMode = true`, so it wouldn't send JOIN_REQUEST messages.

**Fix:** Automatically call `startPairing()` when node boots up unpaired.

**Files Modified:**
- `node/src/main.cpp` (line 368)

### 3. Improved Debug Logging
**Problem:** Hard to diagnose message parsing issues without visibility.

**Fix:** Added debug prints to show:
- When JOIN_ACCEPT is received
- Message content being processed
- Message type after parsing

**Files Modified:**
- `node/src/main.cpp` (lines 731, 763, 771)

## Testing Instructions

1. **Flash the node with updated code**
2. **Clear node NVS** (if previously paired):
   - Use `pio run -t erase` OR
   - Let it auto-clear after timeout OR
   - Hold button 10s on coordinator to clear all
3. **Power on node** - should see:
   ```
   Node: unpaired - auto-starting pairing mode
   Pairing mode: active
   ```
4. **Press pairing button on coordinator** - should see blue pulse
5. **Watch node logs** - should see:
   ```
   RX pairing beacon -> responding
   [green flashes on node]
   Coordinator MAC: 74:4D:BD:AB:A9:F4
   RX JOIN_ACCEPT from coordinator
   Processing message: {"msg":"join_accept",...}
   Message type: 1
   Paired successfully!
   ```
6. **Watch coordinator logs** - should see:
   ```
   JOIN_REQUEST from 10:00:3B:01:98:BC
   Registered node 10:00:3B:01:98:BC with light L0198BC
   Sent join_accept to 10:00:3B:01:98:BC
   Pairing successful - OK confirmation shown
   ```
7. **Verify LEDs:**
   - Node: Bright green flash → Green trailing wave (connected)
   - Coordinator: Green "OK" pulse → Per-node LED turns green

## Expected Behavior After Fix

### Node Side:
1. Boots up unpaired → Auto-starts pairing mode (blue breathing)
2. Sends JOIN_REQUEST every 600-6000ms (with green flash)
3. Responds to pairing beacons (green flash)
4. Receives JOIN_ACCEPT → Logs "RX JOIN_ACCEPT from coordinator"
5. Processes message → Logs "Paired successfully!"
6. Shows bright green flash
7. Switches to operational mode with green trailing wave
8. Sends telemetry every 5 seconds

### Coordinator Side:
1. Pairing button pressed → Blue pulse (500ms)
2. Receives JOIN_REQUEST → Logs "JOIN_REQUEST from [MAC]"
3. Registers node → Logs "Registered node [MAC] with light [ID]"
4. Sends JOIN_ACCEPT → Logs "Sent join_accept to [MAC]"
5. Shows green "OK" pulse (300ms)
6. Per-node LED group turns green
7. Logs "[Node N] [MAC] CONNECTED"

## Common Issues and Solutions

### Issue: Node still not pairing
**Symptom:** No green flashes on node
**Solution:** Node not in pairing mode - hold button for 2 seconds

### Issue: Coordinator not seeing JOIN_REQUEST
**Symptom:** Coordinator logs silent, no "JOIN_REQUEST from..." message
**Solution:** 
- Check both devices on channel 1
- Verify pairing window is open (press button on coordinator)
- Check distance (should be < 10m)

### Issue: Node receiving JOIN_ACCEPT but not processing
**Symptom:** Node logs "RX JOIN_ACCEPT" but never "Paired successfully!"
**Solution:** Message parsing error - check ArduinoJson library version

### Issue: Connection drops after 6 seconds
**Symptom:** Coordinator logs "[Node N] DISCONNECTED (timeout)"
**Solution:** 
- Node not sending telemetry - check if paired correctly
- Increase stale timeout (modify line 498 in Coordinator.cpp: `> 12000U`)

## Key Timing Values

| Parameter | Value | Location |
|-----------|-------|----------|
| Pairing window | 60s | Coordinator.cpp:312 |
| JOIN_REQUEST interval (initial) | 600ms | node main.cpp:460 |
| JOIN_REQUEST interval (later) | 1200-6000ms | node main.cpp:462-466 |
| Telemetry interval | 5s | node main.cpp:372 |
| Stale connection timeout | 6s | Coordinator.cpp:498 |
| Link alive window | 10s | node main.cpp:843 |
| Pairing beacon interval | 800-2000ms | EspNow.cpp:257 |

## Next Steps if Still Not Working

1. **Enable verbose ESP-NOW logs:**
   ```cpp
   esp_log_level_set("*", ESP_LOG_VERBOSE);
   ```

2. **Check message size:**
   - JOIN_REQUEST should be ~153 bytes
   - JOIN_ACCEPT should be < 200 bytes

3. **Verify ESP-NOW initialization:**
   - Both on channel 1
   - Both in STA mode
   - PMK matches
   - Broadcast peer added

4. **Check ArduinoJson:**
   - Version should be 6.x or 7.x
   - Buffer sizes sufficient (256-512 bytes)
