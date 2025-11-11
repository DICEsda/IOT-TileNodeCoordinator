# Complete NVS Fix Guide

## The Real Problem

The `nvs_open failed: NOT_FOUND` error means **the NVS partition doesn't exist on your flash**. This happens when:
1. Your ESP32-S3 was programmed with a partition table that doesn't include NVS
2. The flash still has the old partition layout

## Solution: Complete Flash Erase

You **MUST erase the entire flash** before uploading the fixed firmware. Just uploading new code won't fix the partition table issue.

### Option 1: Using esptool (Recommended)

**Prerequisites:**
- Have esptool installed: `pip install esptool`
- Know your COM port (e.g., COM3, COM4)

**Steps:**
```bash
# 1. Erase entire flash
python -m esptool --chip esp32s3 --port COM3 erase_flash

# 2. Upload new firmware
cd coordinator
python -m platformio run --target upload --upload-port COM3
```

Replace `COM3` with your actual port.

### Option 2: Using VS Code / PlatformIO

1. Open PlatformIO tasks
2. Under "General" → "Erase Flash"
3. Wait for erase to complete
4. Then "Upload" normally

### Option 3: Using Arduino IDE

1. Tools → Erase All Flash Before Sketch Upload
2. Upload normally

## Verification

After erase + upload, you should see:
```
Initializing Logger...
Initializing NVS...
Erasing NVS partition...
Initializing NVS...
✓ NVS initialized successfully
Starting Coordinator...
```

**NOT** the `nvs_open failed: NOT_FOUND` error.

## What Changed

### platformio.ini
```ini
board_build.partitions = default
```
This tells PlatformIO to use the default partition table which includes NVS.

### src/main.cpp
- Always erase NVS first on boot
- Then initialize it
- Wait 1 second before using Preferences
- More verbose error messages

### src/core/Coordinator.cpp
- Removed duplicate Logger::begin() call (already called in main.cpp)
- Prevents double-initialization

## If It STILL Doesn't Work

Check these things:

### 1. Verify esptool can see your board
```bash
python -m esptool --chip esp32s3 --port COM3 read_mac
```
Should show your MAC address.

### 2. Check what's actually on the flash
```bash
python -m esptool --chip esp32s3 --port COM3 read_flash 0x8000 0x100 flash_content.bin
```
This reads the partition table area.

### 3. Manually write partition table
```bash
python -m esptool --chip esp32s3 --port COM3 write_flash 0x8000 partition_table.bin
```

### 4. Try different USB cable
- Some cables don't support data properly
- Try a known good cable

## Why This Happened

Your ESP32-S3 was originally flashed with a custom or incomplete partition table that didn't have NVS space. When Arduino/PlatformIO tried to use Preferences (which needs NVS), it failed with NOT_FOUND.

The code changes I made:
1. Force NVS erase/reinit on every boot to recover from corruption
2. Use the default partition table which includes NVS
3. Add extra delays and recovery logic
4. More logging so you can see what's happening

## Complete Commands (Copy & Paste)

**For Windows PowerShell:**
```powershell
# Full erase + upload
python -m esptool --chip esp32s3 --port COM3 erase_flash
cd c:\Users\jahy0\Desktop\IOT-TileNodeCoordinator\coordinator
python -m platformio run --target upload --upload-port COM3
```

**For Mac/Linux:**
```bash
# Full erase + upload
python -m esptool --chip esp32s3 --port /dev/ttyUSB0 erase_flash
cd ~/IOT-TileNodeCoordinator/coordinator
platformio run --target upload --upload-port /dev/ttyUSB0
```

Replace `COM3` / `/dev/ttyUSB0` with your actual port.

## Next Steps

1. **Erase flash completely** using esptool or IDE
2. **Upload firmware** normally
3. **Monitor serial** at 115200 baud
4. You should see clean initialization logs with NO `nvs_open failed` errors

If you still see the error after a complete erase, there's a hardware issue with your ESP32-S3 board.
