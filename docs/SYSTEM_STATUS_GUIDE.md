# System Status Guide

## Understanding Health Indicators

### What Each Indicator Means

#### API ✓/✗
- **Green (✓)**: Backend HTTP server is running and responding
- **Red (✗)**: Cannot reach backend API
- **Location**: `http://localhost:4200/api/health`

#### DB ✓/✗
- **Green (✓)**: MongoDB database is connected and queryable
- **Red (✗)**: Database connection failed or queries timing out
- **Dependency**: Requires MongoDB container running

#### MQTT ✓/✗  
- **Green (✓)**: Backend is connected to MQTT broker (Mosquitto)
- **Red (✗)**: MQTT broker unreachable or connection lost
- **Dependency**: Requires Mosquitto container running

#### Coordinator ✓/✗
- **Green (✓)**: ESP32 coordinator sent telemetry within last 5 minutes
- **Red (✗)**: No coordinator telemetry received (normal if ESP32 not powered on)
- **Optional**: System works without coordinator for UI testing

### System Status Indicator

#### "System Active" (white pulsing dot)
- All critical components healthy: API ✓, DB ✓, MQTT ✓
- WebSocket and MQTT WebSocket connected
- Dashboard fully functional

#### "System Degraded" (red dot)
- One or more critical components failed
- Some features may not work
- Check individual indicators to identify issue

---

## Common Scenarios

### Scenario 1: Everything Green Except Coordinator
```
✓ API    ✓ DB    ✓ MQTT    ✗ Coordinator
● System Active
```
**Status**: NORMAL - Coordinator is optional
**Can you use the dashboard?**: YES
**Can you see radar?**: NO (requires coordinator sending mmWave data)
**What's missing**: Real-time sensor data, light control

### Scenario 2: DB or MQTT Red
```
✓ API    ✗ DB    ✗ MQTT    ✗ Coordinator
● System Degraded
```
**Status**: PROBLEM - Core services down
**Can you use the dashboard?**: LIMITED
**Can you see radar?**: NO
**What to do**: Check Docker containers

### Scenario 3: All Green (Coordinator Connected)
```
✓ API    ✓ DB    ✓ MQTT    ✓ Coordinator
● System Active
```
**Status**: PERFECT - Full functionality
**Can you use the dashboard?**: YES
**Can you see radar?**: YES (if coordinator sends mmWave data)
**Features**: Everything works

---

## Does the Coordinator Need to Be On?

### Short Answer
**NO** for basic dashboard testing
**YES** for real-time features

### Features That Work WITHOUT Coordinator

✓ **Dashboard UI**
- Navigation, tabs, layouts all work
- Connection status indicators
- Settings panels

✓ **Backend API**
- Health checks
- Database queries
- MQTT messaging infrastructure

✓ **WebSocket/MQTT**
- Connections establish
- Infrastructure ready for data

### Features That NEED Coordinator

✗ **Live Radar Visualization**
- Requires mmWave sensor data from ESP32
- Shows empty until telemetry arrives

✗ **Light Control**
- Requires nodes paired with coordinator
- Commands won't reach lights

✗ **Presence Detection**
- Depends on mmWave sensor attached to coordinator
- No detection without hardware

✗ **Real-Time Telemetry**
- Temperature, voltage, status updates
- All come from coordinator/nodes

---

## Troubleshooting Current Issue

### You Report:
- Dashboard shows: API ✓, WebSocket ✓, MQTT ✓
- Navbar shows: DB ✗, MQTT ✗, Coordinator ✗
- Status: "System Degraded"
- Can't see radar

### Likely Cause
Backend was just rebuilt with new health endpoint. Let's verify:

### Quick Check
1. **Refresh the page** (Ctrl+F5 to force reload)
2. **Wait 30 seconds** for health check to run
3. **Check navbar** - should update to show:
   - API ✓ (already working)
   - DB ✓ (should turn green if MongoDB is running)
   - MQTT ✓ (should turn green if Mosquitto connected)
   - Coordinator ✗ (expected - ESP32 not on)

### If Still Shows Red

#### Check Backend Logs
```bash
docker logs --tail 50 iot-backend
```
Look for:
- "Connected to MQTT broker" ← Should see this
- Any database errors
- Health check responses

#### Check MongoDB
```bash
docker logs --tail 20 iot-mongodb
```
Should show MongoDB running normally

#### Check Mosquitto
```bash
docker logs --tail 20 iot-mosquitto
```
Should show MQTT broker accepting connections

#### Manual Health Check
Open browser to: `http://localhost:4200/api/health`
Should return JSON like:
```json
{
  "status": "healthy",
  "service": "iot-backend",
  "database": true,
  "mqtt": true,
  "coordinator": false,
  "timestamp": 1732118914
}
```

---

## About the Radar Display

### When Will You See the Radar?

The radar visualization appears in two conditions:

#### 1. Live Data Mode (Preferred)
- Coordinator must be powered on and connected
- mmWave sensor connected to coordinator
- Coordinator publishing to `site/{siteId}/coord/{coordId}/mmwave`
- **Benefit**: Real-time target tracking with movement trails

#### 2. Historical Data Mode (Testing)
- Coordinator was on previously
- Database has stored mmWave frames
- Frontend loads last 50 frames on mount
- **Limitation**: Shows past data, not live

### Why You Can't See It Now

Based on "System Degraded" status:
1. Database connection may be failing → can't load history
2. MQTT connection may be down → can't receive live data
3. Coordinator is off → no new data being generated

### How to Get Radar Working

**Option A: Fix Infrastructure (Recommended)**
1. Ensure all Docker containers healthy
2. Verify DB and MQTT show green
3. Dashboard will load historical data if any exists
4. Power on coordinator for live updates

**Option B: Just to See the Visualization**
1. Power on ESP32 coordinator
2. Connect mmWave sensor
3. Ensure coordinator publishes MQTT telemetry
4. Radar will animate immediately

---

## Next Steps

### Immediate Actions
1. **Hard refresh dashboard**: Ctrl+F5 or Ctrl+Shift+R
2. **Wait 30 seconds** for health check interval
3. **Observe navbar indicators** - should see DB and MQTT turn green
4. **Check console** (F12) for any JavaScript errors

### If Still Issues
1. Check browser console (F12 → Console tab)
2. Look for WebSocket connection errors
3. Verify all Docker containers running: `docker ps`
4. Check backend health directly: `http://localhost:4200/api/health`

### To See Radar with Coordinator
1. Power on ESP32 coordinator
2. Ensure Wi-Fi configured (see CONNECTIVITY_FIX.md)
3. Check serial output for MQTT connection
4. Wait for telemetry messages
5. Radar should populate with targets

---

## Summary

**Current State (Expected)**:
- ✓ API, WebSocket, MQTT connections work
- ✗ DB, MQTT broker status - checking with new health endpoint
- ✗ Coordinator - expected (not powered on)
- Radar empty - normal without coordinator or historical data

**To Get Full Green Dashboard**:
1. Refresh page (new backend health endpoint)
2. All should turn green except Coordinator
3. "System Active" replaces "System Degraded"

**To See Radar**:
1. Power on coordinator (ESP32)
2. Or have historical data in database
3. Without either, radar canvas is empty (by design)
