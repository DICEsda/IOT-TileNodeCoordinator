# Build and Test Instructions

## Quick Build

```bash
cd coordinator
pio run -e esp32-s3-devkitc-1
```

## Upload

```bash
pio run -e esp32-s3-devkitc-1 -t upload
```

## Monitor Serial Output

```bash
pio run -e esp32-s3-devkitc-1 -t monitor
```

---

## What Changed (Logger Fix)

### Problem
Logger output wasn't appearing because:
1. **Anonymous namespace `gMinLevel`** created separate copies in each compilation unit
2. **Logging level not globally consistent** - setting DEBUG in main.cpp didn't affect other files
3. **Serial.begin() called twice** - Logger::begin() was resetting the connection

### Solution Applied

**File: `coordinator/src/Logger.h`**

1. **Replaced anonymous namespace with static function** (line 14-16):
   ```cpp
   // Before: namespace { volatile uint8_t gMinLevel = INFO; }
   // After: static volatile uint8_t& getMinLevel() { static volatile uint8_t gMinLevel = INFO; return gMinLevel; }
   ```
   - Now all files share the **same** `gMinLevel` variable
   - Thread-safe singleton pattern

2. **Removed redundant Serial.begin()** (line 19-20):
   - main.cpp already calls `Serial.begin(115200)`
   - Logger::begin() no longer calls it again
   - Just waits for Serial readiness

3. **Added Serial.flush() to every log output** (line 35):
   - Forces buffered data to output immediately
   - Prevents messages from being held in buffer

4. **Updated all references** from `gMinLevel` to `getMinLevel()`

---

## Expected Serial Output After Fix

```
==========================================
ESP32-S3 SMART TILE COORDINATOR
===========================================

Initializing Logger...
[LOGGER] initialized and ready
    12345 | INFO  | *** BOOT START ***
    12456 | INFO  | Initializing NVS flash...
    ...
    [your timestamp] | INFO  | Smart Tile Coordinator starting...
    [your timestamp] | INFO  | Objects created, starting initialization...
    [your timestamp] | INFO  | Initializing ESP-NOW...
    [your timestamp] | INFO  | ==========================================
    [your timestamp] | INFO  | ESP-NOW V2.0 INITIALIZATION CHECKLIST
    ...
```

---

## If Still Not Working

### Check 1: USB CDC Port
- Ensure ESP32-S3 is recognized by system
- Try different USB cable
- Check Device Manager for COM port

### Check 2: Monitor at Different Baud Rates
```bash
# Try 115200
pio run -e esp32-s3-devkitc-1 -t monitor -b 115200

# Or 921600 if that doesn't work
pio run -e esp32-s3-devkitc-1 -t monitor -b 921600
```

### Check 3: Manual Test
Add a simple diagnostic to main.cpp before coordinator.begin():
```cpp
Serial.println("TEST: This should appear");
for (int i = 0; i < 5; i++) {
    Logger::info("TEST LOG %d", i);
    delay(500);
}
```

### Check 4: Enable DEBUG Logging
```cpp
Logger::setMinLevel(Logger::DEBUG);
```

---

## Pairing Test (Already Working)

You mentioned pairing mode works (S3 button enters pairing). This is good!

Next: Try pressing node button to send a join_request. You should now see:
```
*** JOIN_REQUEST detected from [MAC]
Pairing is enabled - processing JOIN_REQUEST
```

