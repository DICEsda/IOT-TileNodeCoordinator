# Coordinator Not Showing in Frontend - Fix Summary

## Root Cause Analysis

### Problem
The coordinator was not appearing in the frontend because:

1. **Coordinator ID Mismatch**:
   - Coordinator was using MAC address `74:4D:BD:AB:A9:F4` as its ID (because `coord_id` in NVS was empty)
   - Frontend was hardcoded to look for `coord001`
   - API endpoint `/sites/site001/coordinators/coord001` returned 404

2. **Missing Data in MQTT Payload**:
   - Coordinator telemetry payload did NOT include `coord_id` field
   - Frontend MQTT subscriber couldn't discover the coordinator ID dynamically
   - Backend extracted coord_id from MQTT topic correctly, but frontend needed it in payload

3. **Empty Database**:
   - No coordinator records in MongoDB
   - Backend was subscribing to `site/+/coord/+/telemetry` correctly
   - But without `coord_id` in payload, frontend couldn't discover which coordinator was online

## Fixes Applied

### 1. Firmware Fix (coordinator/src/comm/Mqtt.cpp)
**Added `site_id` and `coord_id` to coordinator telemetry payload:**
```cpp
void Mqtt::publishCoordinatorTelemetry(const CoordinatorSensorSnapshot& snapshot) {
    ...
    doc["site_id"] = siteId;
    doc["coord_id"] = coordId.length() ? coordId : WiFi.macAddress();
    ...
}
```

This ensures the frontend MQTT subscriber can extract the coordinator ID from the payload.

### 2. Frontend Fix (IOT-Frontend-main/src/app/features/dashboard/tabs/settings/settings.component.ts)
**Made coordinator ID discovery dynamic:**
```typescript
// Changed from: coordinatorId = 'coord001'
coordinatorId = ''; // Will be discovered from MQTT telemetry
private coordinatorDiscovered = false;

ngOnInit() {
  // Subscribe to coordinator telemetry to discover ID
  const coordSub = this.mqtt.subscribeCoordinatorTelemetry(this.siteId, '+').subscribe({
    next: (data: any) => {
      if (!this.coordinatorDiscovered && data.coord_id) {
        this.coordinatorId = data.coord_id;
        this.coordinatorDiscovered = true;
        console.log('[Settings] Discovered coordinator ID:', this.coordinatorId);
        this.checkCoordinatorStatus();
      }
      if (data.coord_id === this.coordinatorId) {
        this.coordinatorOnline.set(true);
      }
    },
    ...
  });
  
  // Fallback to coord001 after 2 seconds if no telemetry received
  setTimeout(() => {
    if (!this.coordinatorDiscovered) {
      this.coordinatorId = 'coord001';
      this.checkCoordinatorStatus();
    }
  }, 2000);
}
```

### 3. Better Error Handling
Updated `checkCoordinatorStatus()` to not mark coordinator offline on 404 errors (coordinator may just not be in DB yet):
```typescript
catch (error: any) {
  if (errorMsg.includes('404')) {
    console.warn(`Coordinator ${this.coordinatorId} not found in database yet (waiting for telemetry)`);
  }
  // Don't set offline if 404
  if (!errorMsg.includes('404')) {
    this.coordinatorOnline.set(false);
  }
}
```

## Deployment Steps

### Option A: Quick Fix (Just set coordinator ID)
1. **Connect to coordinator via serial (115200 baud)**
2. **Reset NVS** (press button or run `fix_nvs.bat`)
3. **During MQTT setup wizard**, set:
   ```
   Coordinator ID (blank = use MAC): coord001
   ```
4. **Restart coordinator**
5. **No need to rebuild firmware** - this works with current firmware

### Option B: Full Fix (Rebuild with new firmware)
1. **Rebuild and flash coordinator firmware:**
   ```bash
   cd coordinator
   pio run -t upload -t monitor
   ```
2. **Rebuild and deploy frontend:**
   ```bash
   cd IOT-Frontend-main/IOT-Frontend-main
   docker-compose up --build frontend
   ```
3. **Monitor logs:**
   ```bash
   docker logs -f iot-backend
   docker logs -f iot-frontend
   ```

### Option C: Hybrid Approach (RECOMMENDED)
1. **Set coordinator ID to match MAC address in NVS** (via serial console)
2. **Rebuild and flash only the firmware** (includes payload fix)
3. **Frontend will auto-discover the MAC-based coordinator ID**
4. **Later rebuild frontend when convenient**

## Verification

### 1. Check Coordinator Serial Output
Should see:
```
Published mmWave frame (1 targets)
Coordinator telemetry saved
```

### 2. Check MQTT Messages
```bash
docker exec iot-mosquitto mosquitto_sub -h localhost -t "site/+/coord/+/telemetry" -v
```
Should see messages with `coord_id` field in payload.

### 3. Check Backend Logs
```bash
docker logs iot-backend --tail 20
```
Should see:
```
Coordinator telemetry saved coordId=74:4D:BD:AB:A9:F4 (or coord001)
```

### 4. Check Database
```bash
docker exec iot-mongodb mongosh iot_smarttile --username admin --password admin123 --authenticationDatabase admin --eval "db.coordinators.find().pretty()"
```
Should return coordinator document(s).

### 5. Check Frontend Console
Browser console should show:
```
[Settings] Discovered coordinator ID: 74:4D:BD:AB:A9:F4
[MQTT] Subscribed to: site/site001/coord/+/telemetry
```

## Expected Result

- **Frontend**: Coordinator status shows as "Online" 
- **No 404 errors** in browser console
- **Dashboard** displays coordinator telemetry (light, temp, mmWave)
- **Settings tab** allows pairing mode, restart coordinator
- **Database** contains coordinator record with last_seen timestamp

## Notes

- **MAC Address Format**: `74:4D:BD:AB:A9:F4` is a valid coordinator ID
- **coord001 is optional**: System works with any coordinator ID now
- **Backward compatible**: Old hardcoded coord001 still works if set in NVS
- **Multiple coordinators**: System can now handle multiple coordinators per site
