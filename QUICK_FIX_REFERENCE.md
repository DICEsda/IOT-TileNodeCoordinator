# Quick Fix Reference - Logger Not Working

## The Problem in 10 Seconds
- Logger messages don't appear on Serial
- Pairing mode works (so basic board is OK)
- Nothing logged from your code

## The Root Cause in 20 Seconds
```cpp
// BAD: Each .cpp file gets its OWN copy
namespace { volatile uint8_t gMinLevel = INFO; }  // In header!

// GOOD: All files share ONE copy
static volatile uint8_t& getMinLevel() {  // In header
    static volatile uint8_t gMinLevel = INFO;
    return gMinLevel;
}
```

## What Was Fixed
1. ✅ Changed anonymous namespace to static function
2. ✅ Removed duplicate Serial.begin() call  
3. ✅ Added Serial.flush() to all log outputs
4. ✅ Updated all 40+ logger calls to use getMinLevel()

## Files Changed
- `coordinator/src/Logger.h` (main fix)
- `coordinator/src/main.cpp` (added flush calls)

## How to Test

```bash
# Build
cd coordinator && pio run -e esp32-s3-devkitc-1

# Upload
pio run -e esp32-s3-devkitc-1 -t upload

# See the logs!
pio run -e esp32-s3-devkitc-1 -t monitor
```

## Expected to See
```
[LOGGER] initialized and ready
     1234 | INFO  | *** BOOT START ***
     1235 | INFO  | Initializing NVS flash...
     1345 | INFO  | ✓ NVS initialized successfully
     ...
```

## If Still Not Working
1. Check USB cable
2. Try different COM port in Device Manager
3. Try baud rate 921600 instead of 115200
4. Add manual test: `Serial.println("WORKS");` before `Logger::begin()`

