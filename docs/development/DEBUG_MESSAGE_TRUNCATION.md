# Debugging Message Truncation Issue

## Problem
JOIN_ACCEPT message is being truncated during transmission from coordinator to node.

**Node receives:**
```
{"msg":"join_accept","node_id":"10:00:3B:01:98:BC","light_id":"L0198BC","lmk":""
```

**Expected full message:**
```json
{
  "msg":"join_accept",
  "node_id":"10:00:3B:01:98:BC",
  "light_id":"L0198BC",
  "lmk":"",
  "cfg":{
    "pwm_freq":0,
    "rx_window_ms":20,
    "rx_period_ms":100
  }
}
```

## Debug Changes Made

### 1. Improved Node Message Reception
**File:** `node/src/main.cpp`

- Changed String construction from `String((char*)data, len)` to explicit character-by-character copy
- Added byte count logging: `Serial.printf("RX %d bytes from %s\n", len, macStr);`
- Changed message logging to show full message instead of truncated substring

### 2. Added Coordinator Message Logging
**File:** `coordinator/src/core/Coordinator.cpp`

- Added `pwm_freq` field explicitly (set to 0)
- Added logging of complete JOIN_ACCEPT message before sending
- Shows exact byte count and message content

## Testing Steps

1. **Flash both devices with updated code**

2. **Power on node** - should see:
   ```
   Node: unpaired - auto-starting pairing mode
   Pairing mode: active
   ```

3. **Press pairing button on coordinator**

4. **Check coordinator logs** for:
   ```
   JOIN_ACCEPT message (XXX bytes): {"msg":"join_accept",...full message...}
   ```
   - Note the byte count
   - Verify message is complete with closing braces

5. **Check node logs** for:
   ```
   RX XXX bytes from 74:4D:BD:AB:A9:F4
   Processing message (XXX chars): {"msg":"join_accept",...}
   ```
   - Compare byte count with what coordinator sent
   - Check if message is complete

## Expected Results

### If byte counts match but message incomplete:
- **Problem:** String construction issue
- **Solution:** The character-by-character copy should fix this

### If byte count differs:
- **Problem:** ESP-NOW transmission truncation
- **Possible causes:**
  - Message too large (>250 bytes)
  - Buffer overflow
  - ESP-NOW internal issue

### If byte count is 0 or message empty:
- **Problem:** Reception callback issue
- **Check:** ESP-NOW v2 callback registration

## Typical JOIN_ACCEPT Size

Estimated size: ~130-150 bytes
```json
{"msg":"join_accept","node_id":"10:00:3B:01:98:BC","light_id":"L0198BC","lmk":"","cfg":{"pwm_freq":0,"rx_window_ms":20,"rx_period_ms":100}}
```

This is well under the 250-byte ESP-NOW limit.

## If Message is Still Truncated

### Option 1: Simplify JOIN_ACCEPT
Remove unnecessary fields:
- Remove `lmk` (empty string)
- Remove `pwm_freq` (unused)

### Option 2: Check ESP-NOW Buffer
Add to node's `onDataRecv`:
```cpp
Serial.printf("Raw data: ");
for (int i = 0; i < len && i < 150; i++) {
    Serial.printf("%02X ", data[i]);
}
Serial.println();
```

### Option 3: Check for Null Termination
Verify last byte:
```cpp
Serial.printf("Last byte: 0x%02X (%c)\n", data[len-1], 
              isprint(data[len-1]) ? data[len-1] : '?');
```

## Common Issues

### Issue: Message exactly 80 bytes
**Cause:** Previous `substring(0, 80)` limit in logging
**Status:** FIXED - now logs full message

### Issue: Message cut at buffer boundary
**Cause:** String constructor not handling non-null-terminated data
**Status:** FIXED - now uses char-by-char copy with explicit length

### Issue: Different byte count on send vs receive
**Cause:** ESP-NOW fragmentation or MTU issue
**Solution:** Check ESP-NOW maximum message size for your ESP-IDF version

## Next Steps

After testing with the debug logging:

1. **Compare byte counts** - coordinator sent vs node received
2. **Check message completeness** - should end with closing braces
3. **Verify JSON parsing** - should see "Message type: 1" (JOIN_ACCEPT)
4. **Watch for "Paired successfully!"** message

If still failing, provide:
- Exact byte count from coordinator
- Exact byte count from node
- Complete message logged by both sides
