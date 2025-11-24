# Quick Fix: Coordinator Not Showing in Frontend

## Problem
- Coordinator is online and publishing MQTT messages
- Frontend shows "HTTP 404: Not Found" for coordinator
- Coordinator uses MAC address `74:4D:BD:AB:A9:F4` but frontend looks for `coord001`

## Fastest Fix (5 minutes)

### Step 1: Flash Updated Coordinator Firmware
```bash
cd coordinator
pio run -e esp32-s3-devkitc-1 -t upload -t monitor
```

Wait for coordinator to boot and connect to MQTT. You should see:
```
✓ MQTT connected!
Published mmWave frame (1 targets)
========== Coordinator Snapshot ==========
```

### Step 2: Rebuild and Restart Frontend
```bash
# From project root
docker-compose build frontend
docker-compose up -d frontend
```

### Step 3: Verify
1. Open http://localhost:4200
2. Go to Settings tab
3. Browser console should show:
   ```
   [Settings] Discovered coordinator ID: 74:4D:BD:AB:A9:F4
   ```
4. Coordinator status should show as "Online"

## What Changed?

### Firmware (coordinator/src/comm/Mqtt.cpp)
Added `coord_id` and `site_id` to telemetry payload so frontend can discover coordinator ID dynamically.

### Frontend (settings.component.ts)
Changed from hardcoded `coordinatorId = 'coord001'` to dynamic discovery from MQTT telemetry.

## Alternative: Set Coordinator ID to coord001

If you prefer to use `coord001` as the coordinator ID:

1. **Connect serial monitor** (115200 baud)
2. **Erase NVS**:
   ```bash
   cd coordinator
   pio run -t erase
   pio run -t upload
   ```
3. **During MQTT setup**, enter:
   ```
   Coordinator ID (blank = use MAC): coord001
   ```
4. **Restart** - coordinator will now use `coord001` as its ID

## Troubleshooting

### Frontend still shows 404
- Check browser console for `[Settings] Discovered coordinator ID: ...`
- If not shown, check MQTT subscription:
  ```bash
  docker exec iot-mosquitto mosquitto_sub -h localhost -t "site/+/coord/+/telemetry" -v
  ```
- Payload should contain `"coord_id":"74:4D:BD:AB:A9:F4"` field

### Backend not saving coordinator
Check backend logs:
```bash
docker logs iot-backend --tail 50 | grep -i coord
```
Should see: `"Coordinator telemetry saved"`

If not, check database:
```bash
docker exec iot-mongodb mongosh iot_smarttile --username admin --password admin123 --authenticationDatabase admin --eval "db.coordinators.find().pretty()"
```

### Coordinator offline after reboot
- Check Wi-Fi credentials in NVS
- Check MQTT broker IP (should be LAN IP, not localhost)
- Run MQTT setup wizard again via serial console
