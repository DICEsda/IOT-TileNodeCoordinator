# LED and Telemetry Fixes

## Changes Made

### 1. Solid Green LED When Connected
**File:** `node/src/led/LedController.cpp`

**Before:**
- Green trailing wave animation (moving dots)
- Could be distracting or hard to see connection status

**After:**
- Solid dim green (30% brightness)
- Clear, steady indication of connected status
- No animation = less power consumption
- Easy to see at a glance

**Color:** RGB(0, 180, 0) at 80 brightness (effective ~30%)

### 2. Improved Telemetry Logging
**File:** `node/src/main.cpp`

**Added:**
- "Sending telemetry to coordinator (nodeId=...)" log before sending
- "Telemetry send: OK/FAILED" log after sending
- "Telemetry: skipped (not paired)" if not paired yet
- Shows node ID and light ID after successful pairing

**Purpose:** Debug visibility - can see if telemetry is actually being sent

### 3. Fixed Telemetry Timer After Pairing
**File:** `node/src/main.cpp`

**Added:** `lastTelemetry = millis();` after pairing succeeds

**Purpose:** Ensures telemetry starts immediately after pairing instead of waiting for next interval

### 4. Removed Telemetry LED Flash
**File:** `node/src/main.cpp`

**Before:** Node would briefly flash green when sending telemetry
**After:** No flash - LED stays solid green

**Purpose:** 
- Less distracting
- Cleaner appearance
- Telemetry happens every 5 seconds, so flashing was too frequent

### 5. Enhanced Message Reception Logging
**File:** `node/src/main.cpp`

**Added:** Shows first 40 characters of every received message

**Example output:**
```
RX 22 bytes from 74:4D:BD:AB:A9:F4: {"msg":"pairing_ping"}
RX 135 bytes from 74:4D:BD:AB:A9:F4: {"msg":"join_accept","node_id":"10:00:...
RX 180 bytes from 74:4D:BD:AB:A9:F4: {"msg":"node_status","node_id":"10:00:...
```

## Expected Behavior After Changes

### During Pairing:
1. **Node LED:** Steady blue breathing (2s cycle)
2. **Green flashes:** When sending JOIN_REQUEST
3. When JOIN_ACCEPT received:
   - Logs: "RX XXX bytes from [coordinator]: {"msg":"join_accept"...
   - Logs: "Paired successfully!"
   - Logs: "Node ID: ..., Light ID: ..."
4. **LED changes to solid green**

### When Connected:
1. **Node LED:** Solid dim green (constant, no animation)
2. **Telemetry every 5 seconds:**
   ```
   Sending telemetry to coordinator (nodeId=10:00:3B:01:98:BC)
   Telemetry send: OK
   ```
3. **No LED flashing** during telemetry

### If Disconnection Occurs:
- Coordinator logs: "[Node N] DISCONNECTED (timeout)"
- **Cause:** Not receiving telemetry from node
- **Check:**
  - Is node logging "Telemetry send: OK"?
  - Is node sending to correct coordinator MAC?
  - Is coordinator receiving messages?

## Troubleshooting

### LED stays blue (pairing mode):
**Problem:** Not receiving JOIN_ACCEPT
**Check:** Node logs for "RX XXX bytes" - should see message with "join_accept"

### LED stays off/idle:
**Problem:** Lost connection or never paired
**Check:** 
- Node logs for "Paired successfully!" 
- Coordinator logs for "Registered node..."

### Solid green but coordinator shows disconnect:
**Problem:** Telemetry not reaching coordinator
**Check:**
- Node logs show "Telemetry send: OK"
- Coordinator should log "JOIN_REQUEST from..." or telemetry messages

### Green flashes constantly:
**Problem:** Still in pairing mode, not connected
**Solution:** Wait for "Paired successfully!" message

## Message Types You Should See

### Node Logs:
```
RX 22 bytes: {"msg":"pairing_ping"}           ← Coordinator beacon
RX 135 bytes: {"msg":"join_accept",...         ← Pairing response
Paired successfully!
Node ID: 10:00:3B:01:98:BC, Light ID: L0198BC
Sending telemetry to coordinator...
Telemetry send: OK
```

### Coordinator Logs:
```
JOIN_REQUEST from 10:00:3B:01:98:BC
Registered node 10:00:3B:01:98:BC with light L0198BC
JOIN_ACCEPT message (135 bytes): {"msg":"join_accept",...}
Sent join_accept to 10:00:3B:01:98:BC
Pairing successful - OK confirmation shown
[Node 1] 10:00:3B:01:98:BC CONNECTED
[Node 1] 10:00:3B:01:98:BC STATUS | XX bytes   ← Telemetry received
```

## Key Timing Values

| Event | Interval | Notes |
|-------|----------|-------|
| Telemetry send | 5s | Configured in main.cpp:372 |
| Stale timeout (coordinator) | 6s | Coordinator.cpp:498 |
| Link alive check (node) | 10s | node main.cpp:843 |

**Important:** Node sends telemetry every 5s, coordinator expects it within 6s. This gives 1 second margin for packet loss.

## If Still Disconnecting

1. **Increase coordinator stale timeout** from 6s to 12s:
   ```cpp
   // File: coordinator/src/core/Coordinator.cpp:498
   if (node.lastSeenMs > 0 && (now - node.lastSeenMs) > 12000U) {
   ```

2. **Check telemetry is actually being sent:**
   - Node should log "Telemetry send: OK" every 5 seconds
   - If not, check `sendMessage()` function

3. **Verify coordinator is processing messages:**
   - Should see "[Node N] STATUS | XX bytes" logs
   - If not, check `handleNodeMessage()` function
