# IoT System - Startup Instructions

## ⚠️ Current Issue: Frontend Connection Errors

Your frontend is showing connection errors because **the backend is not running**.

## ✅ Quick Fix (30 seconds)

### 1. Start Backend
```bash
# Double-click this file or run:
start-backend.bat
```

### 2. Refresh Browser
Your frontend should now connect successfully!

---

## 📋 Complete System Startup

### Prerequisites
- ✅ Go installed (backend)
- ✅ Node.js installed (frontend)
- ✅ Docker installed (optional, for MQTT & MongoDB)
- ✅ PlatformIO installed (for ESP32 firmware)

### Step-by-Step

#### 1. Start Infrastructure Services

**Option A: Docker (Recommended)**
```bash
docker-compose up -d
```

**Option B: Local Services**
```bash
# Start MQTT broker
net start mosquitto

# Start MongoDB
net start MongoDB
```

#### 2. Start Backend (Port 8000)
```bash
start-backend.bat
```

**Manual alternative**:
```bash
cd IOT-Backend-main\IOT-Backend-main
go run cmd\iot\main.go
```

**Verify backend is running**:
```bash
curl http://localhost:8000/health
```

#### 3. Start Frontend (Port 4200)
```bash
start-frontend.bat
```

**Manual alternative**:
```bash
cd IOT-Frontend-main\IOT-Frontend-main
npm install  # First time only
npm start
```

**Access frontend**: http://localhost:4200

#### 4. Flash Coordinator (ESP32-S3)
```bash
cd coordinator
pio run -e esp32-s3-devkitc-1 -t upload -t monitor
```

#### 5. Flash Nodes (ESP32-C3)
```bash
cd node
pio run -e esp32-c3-mini-1 -t upload -t monitor
```

---

## 🔍 Verify Everything Works

### Backend Health
```bash
curl http://localhost:8000/health
```
Expected: `{"status":"healthy",...}`

### Frontend Console
Open http://localhost:4200, press F12, check console:
```
[WebSocket] Connected ✓
[MQTT] Connected ✓
```

### MQTT Traffic
```bash
mosquitto_sub -h localhost -t "site/#" -v
```
Should show messages from coordinator/nodes.

---

## 📊 System Architecture

```
Frontend (Angular)     Backend (Go)          Devices
   :4200          ←→      :8000          ←→  Coordinator
                          ↕                   ↕
                       MQTT :1883         ESP-NOW
                          ↕                   ↕
                      MongoDB :27017         Nodes
```

---

## 🛠️ Troubleshooting

### "Backend not responding"
**Cause**: Backend not started  
**Fix**: Run `start-backend.bat`

### "MQTT connection failed"
**Cause**: MQTT broker not running  
**Fix**: `docker-compose up -d mosquitto` or `net start mosquitto`

### "Database error"
**Cause**: MongoDB not running  
**Fix**: `docker-compose up -d mongodb` or `net start MongoDB`

### "Port 8000 already in use"
**Cause**: Another process using the port  
**Fix**: 
```bash
netstat -ano | findstr :8000
taskkill /PID <PID> /F
```

### "npm command not found"
**Cause**: Node.js not installed  
**Fix**: Install from https://nodejs.org/

### "go command not found"
**Cause**: Go not installed  
**Fix**: Install from https://go.dev/dl/

---

## 📚 Documentation

| Document | Purpose |
|----------|---------|
| **QUICK_START.md** | This guide (start here!) |
| **docs/FRONTEND_BACKEND_STARTUP.md** | Detailed startup instructions |
| **docs/SESSION_SUMMARY.md** | All features implemented |
| **CHANGES.md** | Recent changes overview |

---

## 🎯 Quick Commands Reference

```bash
# Health checks
curl http://localhost:8000/health           # Backend
curl http://localhost:4200                  # Frontend
mosquitto_sub -h localhost -t "#" -C 1      # MQTT

# Start services
docker-compose up -d                        # Docker services
start-backend.bat                           # Backend
start-frontend.bat                          # Frontend

# Flash firmware
cd coordinator && pio run -t upload         # Coordinator
cd node && pio run -t upload                # Node

# View logs
docker-compose logs -f                      # All Docker logs
docker-compose logs -f backend              # Backend logs only
mosquitto_sub -h localhost -t "#" -v        # MQTT messages
```

---

## ✨ Features Implemented

1. **Live Monitor Backend** ✅
   - WebSocket broadcasting for real-time telemetry
   - Light control API (POST /api/v1/node/light/control)
   - Fixed green color for SK6812B LEDs

2. **Auto-Pairing Nodes** ✅
   - Nodes auto-enter pairing on boot if unconfigured
   - Automatic re-pairing after 30s connection loss
   - Zero-touch onboarding

3. **MQTT/ESP-NOW Logging** ✅
   - Detailed coordinator MQTT logging
   - Detailed node ESP-NOW logging
   - Statistics collection and heartbeat monitoring

---

## 🚀 Next Steps After Startup

1. ✅ Verify backend and frontend are running
2. ✅ Flash coordinator and nodes
3. ✅ Nodes should auto-pair with coordinator
4. ✅ Check Live Monitor page for temperature graph
5. ✅ Test LED control (on/off, brightness)
6. ✅ Monitor MQTT messages: `mosquitto_sub -t "site/#" -v`

---

## 💡 Tips

- Keep backend terminal open to see logs
- Keep frontend terminal open to see build logs
- Use `MqttLogger::printStats()` in coordinator for diagnostics
- Use `EspNowLogger::printStats()` in node for diagnostics
- Check browser console (F12) for frontend issues
- Subscribe to MQTT to see all messages: `mosquitto_sub -t "#" -v`

---

**Need help?** Check `docs/FRONTEND_BACKEND_STARTUP.md` for detailed troubleshooting!

