# IOT SmartTile System - Quick Start Guide

## 🚀 Get Started in 5 Minutes

### Step 1: Configure Environment
```bash
# Copy the complete configuration file
cp .env.complete .env

# Edit .env and fill in these essential fields:
ESP32_WIFI_SSID=YourWiFiName
ESP32_WIFI_PASSWORD=YourWiFiPassword
MQTT_USERNAME=user1
MQTT_PASSWORD=user1
```

### Step 2: Start the System
```bash
# Start all services with Docker
docker-compose up --build

# Or use the quick start script
./start-docker-simple.bat
```

### Step 3: Access Dashboard
```
Open your browser and navigate to:
http://localhost:4200

The dashboard will load automatically.
```

### Step 4: Flash Firmware

**Coordinator (ESP32-S3):**
```bash
cd coordinator
pio run -e esp32-s3-devkitc-1 -t upload -t monitor
```

**Node (ESP32-C3):**
```bash
cd node
pio run -e esp32-c3-mini-1 -t upload
```

### Step 5: Pair Your First Node

1. **Open Settings Tab** in the dashboard
2. **Click "Enter Pairing Mode"** - you'll see a 60-second countdown
3. **Press the button on your node device** to initiate pairing
4. **Node appears** in the "Node Management" section automatically

### Step 6: Test LED Control

1. **Select a color** from the color picker
2. **Watch the LEDs change** color on your node
3. **Adjust brightness** with the slider
4. **Turn off** LEDs with the button

## ✅ You're Done!

Your IOT SmartTile system is now running. Here's what you can do next:

### In the Dashboard Tab:
- **View live mmWave radar** showing motion detection
- **Monitor node status** with real-time telemetry
- **See node cards** with LED status

### In the Settings Tab:
- **Manage zones** (Living Room, Bedroom, etc.)
- **Assign nodes to zones** for organization
- **Configure WiFi** credentials
- **Test LED colors** for connectivity
- **Restart coordinator** remotely
- **Enable Google Home** (optional)

## 🎯 Common Tasks

### Rename a Node
```
Settings → Node Management → Node Name input → Type new name
```

### Assign to Zone
```
Settings → Node Management → Zone Assignment dropdown → Select zone
```

### Change WiFi Network
```
Settings → WiFi Configuration → Enter SSID and password → Update WiFi Settings
```

### Restart Coordinator
```
Settings → Coordinator Control → Restart Coordinator button
```

### Remove a Node
```
Settings → Node Management → Delete button (trash icon)
```

## 🔧 System Architecture

```
┌─────────────┐     ESP-NOW      ┌─────────┐
│   Node      │ ◄──────────────► │  Coord  │
│  (ESP32-C3) │                  │(ESP32-S3)│
└─────────────┘                  └────┬────┘
                                      │
                                   WiFi/MQTT
                                      │
                  ┌───────────────────┴───────────────────┐
                  │                                       │
            ┌─────▼─────┐                         ┌──────▼──────┐
            │   MQTT    │                         │   Backend   │
            │  Broker   │◄────────────────────────│   (Go)      │
            └───────────┘                         └──────┬──────┘
                                                         │
                                                     HTTP/WS
                                                         │
                                                  ┌──────▼──────┐
                                                  │  Frontend   │
                                                  │  (Angular)  │
                                                  └─────────────┘
```

## 📊 Dashboard Overview

### Main Dashboard Tab
- **mmWave Radar Cone** - Shows 120° field of view with detected targets
- **Node Status Cards** - Real-time LED and sensor data
- **Connectivity Status** - WiFi, MQTT, online/offline indicators

### Settings Tab
- **Coordinator Control** - Pairing, restart, status
- **Node Management** - Single node control and configuration
- **Zone Management** - Create and manage zones
- **WiFi Configuration** - Network credentials
- **MQTT Configuration** - Broker settings
- **Google Home** - Voice control integration
- **System Settings** - Auto mode, sensitivity, brightness

## 🔍 Monitoring

### Serial Monitor (Coordinator)
```bash
cd coordinator
pio device monitor

# You'll see:
COORDINATOR DATA: light=512, temp=23.5, mmwave=presence
STATUS: wifi=connected, mqtt=connected, pairing=inactive
FETCHED NODE [AA:BB:CC:DD:EE:FF] DATA: avgColor=RGBW(255,0,0,0), temp=22.3
```

### MQTT Topics
```
# Coordinator publishes to:
site/site001/coord/coord001/telemetry
site/site001/coord/coord001/mmwave

# Node publishes to:
site/site001/node/node001/telemetry

# Commands sent to:
site/site001/coord/coord001/cmd
site/site001/node/node001/cmd
```

### API Endpoints
```
GET    /health                          # System health check
GET    /api/v1/settings?site_id=site001 # Get settings
POST   /api/v1/coordinator/pair         # Start pairing
POST   /api/v1/node/test-color          # Send test color
```

## 🐛 Troubleshooting

### Node Won't Pair
**Problem:** Pairing mode times out without finding node

**Solution:**
1. Ensure coordinator is online (check status in Settings)
2. Verify node is flashed with correct firmware
3. Press and hold node button during pairing window
4. Check Serial output on both devices
5. Ensure ESP-NOW channel matches WiFi channel

### LEDs Don't Respond
**Problem:** Color commands not changing LEDs

**Solution:**
1. Verify node is online (check last_seen timestamp)
2. Check MQTT broker is running: `docker ps`
3. View MQTT messages: `docker logs mosquitto`
4. Test with MQTT client: `mosquitto_pub -t "site/site001/node/node001/cmd" -m '{"cmd":"set_color","r":255,"g":0,"b":0,"w":0}'`

### Dashboard Not Loading
**Problem:** Frontend shows connection error

**Solution:**
1. Check backend is running: `docker ps`
2. View backend logs: `docker logs iot-backend`
3. Verify API_URL in .env: `API_URL=http://localhost:8000`
4. Check CORS settings in backend

### Coordinator Offline
**Problem:** Dashboard shows coordinator as offline

**Solution:**
1. Check Serial output from coordinator
2. Verify WiFi credentials in .env
3. Ensure MQTT broker is accessible
4. Restart coordinator
5. Check network connectivity

## 📝 Configuration Files

### Essential Environment Variables
```bash
# WiFi (REQUIRED)
ESP32_WIFI_SSID=YourNetwork
ESP32_WIFI_PASSWORD=YourPassword

# MQTT (REQUIRED)
MQTT_BROKER=tcp://mosquitto:1883
MQTT_USERNAME=user1
MQTT_PASSWORD=user1

# API Endpoints (REQUIRED)
API_URL=http://localhost:8000
WS_URL=ws://localhost:8000/ws

# Google Home (OPTIONAL)
GOOGLE_HOME_ENABLED=false
GOOGLE_HOME_PROJECT_ID=your-project-id
GOOGLE_HOME_CLIENT_ID=your-client-id
GOOGLE_HOME_CLIENT_SECRET=your-secret
GOOGLE_HOME_API_KEY=your-api-key
```

## 🎓 Next Steps

### 1. Add More Nodes
Currently supporting 1 node during development. To add more:
- Update `MAX_NODES_PER_COORDINATOR` in .env
- Flash additional node devices
- Pair each node individually

### 2. Set Up Zones
Create zones to organize your nodes:
```
Settings → Zone Management → Add Zone
```

Common zones:
- Living Room
- Bedroom
- Kitchen
- Bathroom
- Office
- Hallway

### 3. Enable Google Home
For voice control:
1. Create Google Cloud Project
2. Enable HomeGraph API
3. Set up OAuth credentials
4. Fill credentials in Settings
5. Link your account
6. Say "Hey Google, turn on the lights"

### 4. Customize Settings
Adjust system behavior:
- **Auto Mode** - Lights respond to motion automatically
- **Motion Sensitivity** - How sensitive the mmWave sensor is
- **Light Intensity** - Default brightness level
- **Auto-off Delay** - Time before lights turn off

## 📚 Documentation

- `README.md` - Project overview
- `IMPLEMENTATION_COMPLETE.md` - Full feature list
- `IMPLEMENTATION_TASKS.md` - Technical details
- `.env.complete` - All configuration options
- `docs/` - Additional documentation

## 💡 Tips

1. **Keep Serial Monitor Open** - Helpful for debugging
2. **Check MQTT Topics** - Use MQTT Explorer to see messages
3. **Test One Feature at a Time** - Easier to troubleshoot
4. **Use Test Colors** - Verify connectivity before automation
5. **Save Settings Frequently** - Don't lose your configuration

## 🎉 Success!

Your IOT SmartTile system is running and ready to use. Enjoy your smart lighting!

For questions or issues, check the troubleshooting section or review the full documentation.

**Happy Tiling! 🟦✨**
