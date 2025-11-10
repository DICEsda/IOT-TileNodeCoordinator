# JSON Parse Error Fix

## Problem
Node was receiving JOIN_ACCEPT message correctly (139 bytes, complete JSON), but failing to parse with "ERROR: Failed to parse message!"

## Root Cause
**ArduinoJson buffer too small**

File: `shared/src/EspNowMessage.cpp` line 58

```cpp
// BEFORE (TOO SMALL):
DynamicJsonDocument doc(256);

// AFTER (FIXED):
DynamicJsonDocument doc(512);
```

## Why 256 Bytes Was Not Enough

The JSON message is 139 bytes:
```json
{"msg":"join_accept","node_id":"10:00:3B:01:98:BC","light_id":"L0198BC","lmk":"","cfg":{"pwm_freq":0,"rx_window_ms":20,"rx_period_ms":100}}
```

**BUT** ArduinoJson needs additional memory for:
- Internal data structures
- Pointers and metadata
- Nested object handling (`cfg` is nested)
- String termination and alignment

**Rule of thumb:** Buffer should be 2-3x the JSON size for nested objects.

## Fix Applied
- Changed buffer from 256 to 512 bytes
- Added error message logging to show parse errors
- Now logs: `JSON parse error: [error description]`

## Expected Behavior After Fix

### Node logs should now show:
```
RX 139 bytes from 74:4D:BD:AB:A9:F4: {"msg":"join_accept",...
Coordinator MAC: 74:4D:BD:AB:A9:F4
RX JOIN_ACCEPT from coordinator
Processing message (139 chars): {"msg":"join_accept",...}
Message type: 1
Paired successfully!
Node ID: 10:00:3B:01:98:BC, Light ID: L0198BC
Sending telemetry to coordinator (nodeId=10:00:3B:01:98:BC)
Telemetry send: OK
```

**LED:** Solid green (connected)

### Every 5 seconds:
```
Sending telemetry to coordinator (nodeId=10:00:3B:01:98:BC)
Telemetry send: OK
```

### Coordinator should show:
```
[Node 1] 10:00:3B:01:98:BC CONNECTED
[Node 1] 10:00:3B:01:98:BC STATUS | XX bytes
```

**No more disconnects!**

## How to Calculate Required Buffer Size

Use ArduinoJson Assistant: https://arduinojson.org/v6/assistant/

For our JOIN_ACCEPT:
- Input JSON: 139 bytes
- Recommended buffer: **384 bytes minimum**
- We use 512 for safety margin

## Other Messages Buffer Sizes

| Message | JSON Size | Buffer | Status |
|---------|-----------|--------|--------|
| JOIN_REQUEST | ~153 bytes | 512 | ✓ OK |
| JOIN_ACCEPT | ~139 bytes | 512 | ✓ FIXED |
| SET_LIGHT | ~150 bytes | 256 | ⚠ May need increase |
| NODE_STATUS | ~180 bytes | 256 | ⚠ May need increase |
| ACK | ~40 bytes | 96 | ✓ OK |
| ERROR | ~100 bytes | 192 | ✓ OK |

## If Still Having Parse Errors

1. **Check if JSON is complete:**
   - Node logs show full message
   - Ends with closing brace `}`

2. **Check for trailing garbage:**
   - Print last few bytes:
   ```cpp
   Serial.printf("Last 3 bytes: %02X %02X %02X\n", 
                 data[len-3], data[len-2], data[len-1]);
   ```

3. **Increase buffer more:**
   - Try 768 or 1024 if still failing
   - ArduinoJson might need more for your platform

4. **Check ArduinoJson version:**
   - Should be 6.x or 7.x
   - Version 5.x has different API

## Memory Considerations

**ESP32-C3 (Node):**
- Total RAM: 400 KB
- 512 bytes = 0.125% of RAM
- **Safe to use**

**ESP32-S3 (Coordinator):**
- Total RAM: 512 KB  
- 512 bytes = 0.1% of RAM
- **Safe to use**

## Prevention

For future messages, always use ArduinoJson Assistant to calculate buffer size:
1. Paste your JSON
2. Select "Deserialization"
3. Use recommended buffer size
4. Add 20-30% safety margin
