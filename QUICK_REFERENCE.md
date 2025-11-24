# 🚀 Quick Reference Card - IOT Smart Tile System

## ⚡ One-Command Start

```batch
quick-start.bat
```
**That's it!** This script does everything:
- Creates .env from template
- Builds Docker images
- Starts all services
- Waits for health checks
- Opens frontend in browser

---

## 🎯 Essential Commands

### Docker Services

```batch
# Start all services
docker-compose up -d

# Stop all services
docker-compose down

# View logs (all services)
docker-compose logs -f

# View specific service logs
docker-compose logs -f backend
docker-compose logs -f frontend
docker-compose logs -f mosquitto

# Check service status
docker-compose ps

# Restart a service
docker-compose restart backend
```

### ESP32 Coordinator

```batch
# Build firmware
cd coordinator
pio run -e esp32-s3-devkitc-1

# Flash firmware
pio run -e esp32-s3-devkitc-1 -t upload

# Monitor serial output
pio run -e esp32-s3-devkitc-1 -t monitor

# Flash + Monitor (one command)
pio run -e esp32-s3-devkitc-1 -t upload -t monitor

# Clean build
pio run -e esp32-s3-devkitc-1 -t clean
```

### ESP32 Node

```batch
# Build firmware
cd node
pio run -e esp32-c3-mini-1

# Flash firmware
pio run -e esp32-c3-mini-1 -t upload

# Monitor serial output
pio run -e esp32-c3-mini-1 -t monitor
```

### Backend API Testing

```bash
# Health check
curl http://localhost:8000/health

# Get all sites
curl http://localhost:8000/api/v1/sites

# Get nodes
curl http://localhost:8000/api/v1/nodes

# Set light color
curl -X POST http://localhost:8000/api/v1/set-light \
  -H "Content-Type: application/json" \
  -d '{
    "node_id": "AA:BB:CC:DD:EE:FF",
    "site_id": "site001",
    "rgbw": {"r": 255, "g": 100, "b": 50, "w": 200},
    "brightness": 80
  }'
```

### MQTT Testing

```bash
# Subscribe to all topics
mosquitto_sub -h localhost -p 1883 -u user1 -P user1 -t '#' -v

# Subscribe to node telemetry
mosquitto_sub -h localhost -p 1883 -u user1 -P user1 -t 'site/+/node/+/telemetry'

# Subscribe to coordinator telemetry
mosquitto_sub -h localhost -p 1883 -u user1 -P user1 -t 'site/+/coord/+/telemetry'

# Publish test message
mosquitto_pub -h localhost -p 1883 -u user1 -P user1 \
  -t 'site/site001/node/TEST/cmd' \
  -m '{"cmd":"set_light","r":255,"g":0,"b":0,"w":0}'
```

### MongoDB Access

```bash
# Connect to MongoDB shell
docker exec -it iot-mongodb mongosh -u admin -p admin123

# Inside mongosh:
use iot_smarttile
show collections
db.nodes.find()
db.telemetry.find().limit(5)
db.sites.find()
```

---

## 🔌 Access Points

| Service | URL | Credentials |
|---------|-----|-------------|
| **Frontend Dashboard** | http://localhost:4200 | N/A |
| **Backend API** | http://localhost:8000 | N/A |
| **API Health** | http://localhost:8000/health | N/A |
| **MQTT Broker** | mqtt://localhost:1883 | user1 / user1 |
| **MongoDB** | mongodb://localhost:27017 | admin / admin123 |

---

## 🔧 Configuration Files

| File | Purpose |
|------|---------|
| `.env` | Environment variables (create from `.env.example`) |
| `docker-compose.yml` | Docker services configuration |
| `coordinator/platformio.ini` | Coordinator build config |
| `node/platformio.ini` | Node build config |
| `IOT-Backend-main/IOT-Backend-main/internal/config/mosquitto.conf` | MQTT broker config |

---

## 🎨 Pairing Workflow

### Step-by-Step Node Pairing

1. **Enter pairing mode on coordinator**:
   - Short press touch button on ESP32-S3
   - Status LED breathes blue
   - Pairing window open for 60 seconds

2. **Initiate pairing on node**:
   - Hold button for 2-3 seconds on ESP32-C3
   - Node LED blinks blue (sending join request)

3. **Confirmation**:
   - ✅ Node LED flashes green (paired)
   - ✅ Coordinator LED shows paired node count
   - ✅ Serial monitor: "PAIRING SUCCESS"

4. **Verify**:
   - Check backend logs: `docker-compose logs -f backend`
   - Check frontend: Navigate to Devices tab
   - Node should appear in list with "online" status

---

## 📊 Serial Monitor Output

### Coordinator Startup (Normal)
```
==========================================
ESP32-S3 SMART TILE COORDINATOR
==========================================

Initializing Logger...
[LOGGER] initialized and ready
*** BOOT START ***
Initializing NVS flash...
Smart Tile Coordinator starting...
Initializing ESP-NOW...
Initializing WiFi...
Connected to WiFi: YourSSID
IP Address: 192.168.1.xxx
MQTT connecting to broker...
MQTT connected successfully
Subscribed to: site/site001/coord/+/cmd
Coordinator ready
```

### Node Startup (Normal)
```
==========================================
ESP32-C3 SMART TILE NODE
==========================================

Node initializing...
MAC Address: AA:BB:CC:DD:EE:FF
ESP-NOW initialized
Waiting for coordinator...
[After pairing button press]
Sending JOIN_REQUEST...
Received JOIN_ACCEPT
PAIRING SUCCESS
Node operational
```

---

## 🐛 Quick Troubleshooting

| Problem | Quick Fix |
|---------|-----------|
| **Docker services won't start** | `docker-compose down -v && docker-compose up -d` |
| **Backend can't connect to MQTT** | Check MQTT broker IP in .env, verify with `docker-compose logs mosquitto` |
| **Coordinator won't connect WiFi** | Use serial prompt to reconfigure: Press 'y' when prompted |
| **Node won't pair** | Clear NVS: `fix_nvs.bat`, then retry pairing |
| **Frontend shows no data** | Check browser console, verify WebSocket connection |
| **MQTT connection refused** | Verify credentials: user1/user1 (default) |

---

## 📱 Serial Commands

### WiFi Configuration (Coordinator)
```
No Wi-Fi configured. Configure now? (y/n): y
Enter SSID: YourWiFiNetwork
Enter Password: YourPassword
```

### NVS Configuration (Debug)
```cpp
// In coordinator/src/main.cpp
ConfigManager config("mqtt");
config.begin();
config.setString("broker_host", "192.168.1.100");
config.setInt("broker_port", 1883);
config.end();
```

---

## 🔍 Verification Commands

### Check All Services Running
```batch
docker-compose ps
```
Expected: All 4 services "Up" and "healthy"

### Check Backend Health
```batch
curl http://localhost:8000/health
```
Expected: `{"status":"healthy"}`

### Check MQTT Broker
```batch
mosquitto_sub -h localhost -p 1883 -u user1 -P user1 -t '$SYS/#' -C 1
```
Expected: System info message

### Check MongoDB
```batch
docker exec -it iot-mongodb mongosh --eval "db.adminCommand('ping')" -u admin -p admin123
```
Expected: `{ ok: 1 }`

---

## 🎯 Common Tasks

### Add New Node
1. Flash node firmware: `cd node && pio run -t upload`
2. Enter pairing mode on coordinator (button press)
3. Hold node button for 2-3 seconds
4. Verify in frontend dashboard

### Update Firmware (OTA)
1. Build new firmware: `pio run -e esp32-c3-mini-1`
2. Upload via backend API (endpoint: `/api/v1/ota/start`)
3. Monitor progress in logs

### Change WiFi Network
1. Connect to coordinator via serial
2. Clear NVS: `fix_nvs.bat`
3. Re-flash and configure new credentials

### View Real-Time Telemetry
1. Backend logs: `docker-compose logs -f backend | grep telemetry`
2. MQTT: `mosquitto_sub -h localhost -t 'site/#' -v`
3. Frontend: Dashboard → Devices tab

### Export Data
```bash
# MongoDB export
docker exec iot-mongodb mongodump --db iot_smarttile --out /backup
docker cp iot-mongodb:/backup ./mongodb-backup
```

---

## 💡 Pro Tips

1. **Use tmux/screen** for multiple serial monitors
2. **Keep backend logs open** in separate terminal
3. **Use Postman/Insomnia** for API testing (import `api-collection.json`)
4. **Enable DEBUG logging** in coordinator for detailed output
5. **Monitor MQTT traffic** with MQTT Explorer GUI tool

---

## 📖 Documentation Links

- **Full Guide**: `COMPLETION_GUIDE.md`
- **Deployment**: `DEPLOYMENT.md`
- **API Reference**: `docs/mqtt_api.md`
- **Project Status**: `docs/development/PROJECT_STATUS.md`

---

**Quick Help**: If stuck, check logs first!
```batch
docker-compose logs -f backend
cd coordinator && pio device monitor
```

**Version**: 1.0
**Last Updated**: 2024-11-20
