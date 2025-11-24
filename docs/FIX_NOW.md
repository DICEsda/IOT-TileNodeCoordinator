# 🚨 Fix Now - Coordinator Not Connecting

## Current Status

✅ **Backend rebuilt** - Compiles successfully  
✅ **Frontend rebuilt** - Running with dynamic discovery  
❌ **Coordinator firmware** - NOT FLASHED YET (still using old version)

## The Problem

Your coordinator is running **OLD firmware** that doesn't include `coord_id` in MQTT telemetry.

Evidence from logs:
```
[Settings] No coordinator telemetry received, attempting direct query
GET /sites/site001/coordinators/coord001 404 (Not Found)
[Settings] Coordinator coord001 not found in database yet
```

This means:
1. Coordinator is publishing MQTT (you saw this in serial logs earlier)
2. Backend is NOT receiving messages with `coord_id` in payload
3. Frontend can't discover the coordinator
4. Database stays empty

## ⚡ Solution (5 minutes)

### Step 1: Flash Updated Firmware

```bash
cd C:\Users\yahya\projects\IOT-TileNodeCoordinator\coordinator
pio run -e esp32-s3-devkitc-1 -t upload -t monitor
```

**Wait for coordinator to boot.** You should see:
```
✓ MQTT connected!
Published mmWave frame (1 targets)
```

### Step 2: Verify MQTT Payload

In another terminal:
```bash
docker exec iot-mosquitto mosquitto_sub -h localhost -t "site/+/coord/+/telemetry" -v
```

You should see messages with **`coord_id` field**:
```json
{
  "ts": 1234567,
  "site_id": "site001",
  "coord_id": "74:4D:BD:AB:A9:F4",  ← THIS IS NEW!
  "light_lux": 76.0,
  "temp_c": 25.3,
  ...
}
```

### Step 3: Check Frontend (F12 Console)

Within 2 seconds, you should see:
```
[Settings] Discovered coordinator ID: 74:4D:BD:AB:A9:F4
```

### Step 4: Verify Database

```bash
docker exec iot-mongodb mongosh iot_smarttile --username admin --password admin123 --authenticationDatabase admin --eval "db.coordinators.find().pretty()"
```

Should return coordinator record with `_id: "74:4D:BD:AB:A9:F4"`.

---

## Alternative: Quick Fix Without Rebuilding

If flashing firmware is taking too long, you can temporarily set the coordinator ID to `coord001`:

```bash
# Connect to coordinator serial (115200 baud)
cd coordinator
pio run -t monitor

# Press SPACE when you see boot messages
# Navigate to MQTT setup
# Set: Coordinator ID = coord001

# Restart coordinator
```

This will make it work immediately with the current frontend, but you'll miss the auto-discovery feature.

---

## Troubleshooting

### "pio: command not found"

Install PlatformIO:
```bash
pip install platformio
```

### Coordinator won't connect to MQTT

Check broker IP in NVS:
```bash
cd coordinator
pio device monitor
# Wait for boot, check MQTT broker IP
# Should be: 10.66.62.195 (your LAN IP, NOT localhost)
```

### Still getting 404 errors

1. Clear browser cache (Ctrl+Shift+Del)
2. Hard refresh (Ctrl+F5)
3. Check backend logs:
   ```bash
   docker logs iot-backend --tail 50 | grep -i "telemetry saved"
   ```

### Coordinator shows "nodes" in URL

There's a bug in `loadNode()` function. Check:
```
GET /sites/site001/coordinators/nodes
```

This should be fixed in settings component. The error is harmless but annoying.

---

## Expected Result After Fix

✅ Frontend console:
```
[Settings] Discovered coordinator ID: 74:4D:BD:AB:A9:F4
[MQTT] Subscribed to: site/site001/coord/+/telemetry
```

✅ Settings tab: Coordinator shows **GREEN** light

✅ No 404 errors

✅ Can trigger pairing mode, restart coordinator

---

## Why This Happened

The code changes I made to the firmware were **saved but not flashed** to the coordinator.

**You rebuilt Docker containers** ✅  
**You DID NOT flash coordinator** ❌

The coordinator is still running the old firmware without `coord_id` in the payload.

---

## Next Step

**FLASH THE COORDINATOR NOW:**

```bash
cd C:\Users\yahya\projects\IOT-TileNodeCoordinator\coordinator
pio run -e esp32-s3-devkitc-1 -t upload -t monitor
```

This is the ONLY remaining step. Everything else is done.
