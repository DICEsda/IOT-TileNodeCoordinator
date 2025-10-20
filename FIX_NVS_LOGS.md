# Fix for NVS Error and Missing Logs

## Problem
Your ESP32-S3 is showing these errors:
```
[ 11262][E][Preferences.cpp:47] begin(): nvs_open failed: NOT_FOUND
[ 11772][E][Preferences.cpp:47] begin(): nvs_open failed: NOT_FOUND
```

And logs aren't showing up because the system is failing during initialization.

## Root Cause
The **custom partition table wasn't being used** by PlatformIO, so the NVS partition didn't exist at the expected location.

## ✅ Fixes Applied

### 1. **Added Partition Table to platformio.ini**
Added this line to tell PlatformIO to use your custom partition table:
```ini
board_build.partitions = partitions.csv
```

### 2. **Improved NVS Initialization**
- Now handles `ESP_ERR_NOT_FOUND` error (missing partition)
- Erases and reinitializes if partition is corrupted or missing
- Better error messages to help diagnose issues

### 3. **Reordered Initialization**
- Logger is initialized FIRST (before anything else)
- Then NVS is initialized
- Then Coordinator starts
- This ensures you always see logs, even if NVS fails

### 4. **Added ESP_ERR_NOT_FOUND Handling**
The code now specifically handles the NOT_FOUND error you were seeing:
```cpp
if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
    ret == ESP_ERR_NVS_NEW_VERSION_FOUND ||
    ret == ESP_ERR_NOT_FOUND) {
    // Erase and reinitialize
}
```

## 🔧 How to Apply the Fix

### Step 1: Clean Build
You MUST do a clean build to pick up the new partition table:

**Using VS Code:**
1. Open PlatformIO sidebar
2. Click "Clean" under Project Tasks
3. Then click "Build"

**Using Command Line:**
```bash
cd c:\Users\jahy0\Desktop\IOT-TileNodeCoordinator\coordinator
# Clean
python -m platformio run --target clean
# Build
python -m platformio run
# Upload
python -m platformio run --target upload
```

### Step 2: Flash with Partition Table
After clean build, upload the firmware. This will write:
- New partition table
- NVS partition at correct location
- Your firmware

### Step 3: Verify
After uploading, you should see:
```
===========================================
ESP32-S3 SMART TILE COORDINATOR
===========================================

[LOGGER] started
       572 | INFO  | *** BOOT START ***
       580 | INFO  | Initializing NVS flash...
Initializing NVS...
✓ NVS initialized successfully
       650 | INFO  | ✓ NVS initialized successfully
       652 | INFO  | *** SETUP START ***
```

## 🐛 If It Still Fails

### Check 1: Verify Partition Table is Loaded
Look for this in build output:
```
Partition table binary generated. Contents:
...
nvs              data  nvs      00009000 00006000
...
```

If you don't see this, the partition table isn't being used.

### Check 2: Try Full Erase
Sometimes the flash needs to be completely erased:

**Using esptool:**
```bash
python -m esptool --chip esp32s3 --port COM3 erase_flash
```
Replace `COM3` with your actual port.

Then upload firmware again.

### Check 3: Manual Flash
If automatic upload doesn't work, manually flash:
```bash
python -m esptool --chip esp32s3 --port COM3 write_flash 0x0 .pio\build\esp32-s3-devkitc-1\firmware.bin
```

## 📊 What Changed in Files

### coordinator/platformio.ini
```diff
+ ; Partition table
+ board_build.partitions = partitions.csv
```

### coordinator/src/main.cpp
- Logger initialized FIRST
- Better NVS error handling
- Handles ESP_ERR_NOT_FOUND
- More verbose error messages
- Serial.flush() calls to ensure messages appear

## 🎯 Expected Result

After applying these fixes and doing a clean build, you should see:

✅ **Complete boot logs** from the very start  
✅ **NVS initialized successfully**  
✅ **No more "NOT_FOUND" errors**  
✅ **Preferences working correctly**  
✅ **NodeRegistry able to save/load data**  
✅ **ESP-NOW pairing data persists across reboots**  

## 📝 Notes

- The partition table is now at `coordinator/partitions.csv`
- NVS partition is 24KB (0x6000 bytes) at offset 0x9000
- This is standard for ESP32-S3 and should work fine
- If you see "System will work but data won't persist", NVS initialization failed but the system continues anyway (for debugging)

## ⚠️ Important
**You MUST do a clean build!** Just rebuilding won't update the partition table. The old flash layout will remain until you do a full clean + upload.

---

**Need help?** Check the serial monitor output and compare it to the "Expected Result" section above.
