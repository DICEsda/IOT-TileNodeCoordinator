# IOT Smart Tile System - Node Coordinator

[![ESP32](https://img.shields.io/badge/ESP32-S3%20%7C%20C3-E7352C?style=flat&logo=espressif&logoColor=white)](https://www.espressif.com/)
[![Platform](https://img.shields.io/badge/Platform-Arduino-00979D?style=flat&logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Framework-FF7F00?style=flat&logo=platformio&logoColor=white)](https://platformio.org/)
[![Go](https://img.shields.io/badge/Go-1.21+-00ADD8?style=flat&logo=go&logoColor=white)](https://golang.org/)
[![Angular](https://img.shields.io/badge/Angular-15+-DD0031?style=flat&logo=angular&logoColor=white)](https://angular.io/)
[![TypeScript](https://img.shields.io/badge/TypeScript-5.0+-3178C6?style=flat&logo=typescript&logoColor=white)](https://www.typescriptlang.org/)
[![MQTT](https://img.shields.io/badge/MQTT-Mosquitto-3C5280?style=flat&logo=eclipsemosquitto&logoColor=white)](https://mosquitto.org/)
[![MongoDB](https://img.shields.io/badge/MongoDB-7.0-47A248?style=flat&logo=mongodb&logoColor=white)](https://www.mongodb.com/)
[![Docker](https://img.shields.io/badge/Docker-Compose-2496ED?style=flat&logo=docker&logoColor=white)](https://www.docker.com/)
[![License](https://img.shields.io/badge/License-MIT-yellow?style=flat)](LICENSE)

## Project
**Technologies & Features:**
- **Hardware:** `ESP32-S3` `ESP32-C3` `ESP-NOW Protocol` `RGBW LED` `TMP117` `TSL2561` `LD2450 mmWave`
- **Networking:** `MQTT` `WebSocket` `REST API` `Mesh Networking` `Wireless Sensor Network`
- **Backend:** `Go` `MongoDB` `Docker` `Time-Series Database`
- **Frontend:** `Angular` `TypeScript` `Real-Time Dashboard` `Data Visualization`
- **Features:** `Smart Lighting` `Zone Control` `Presence Detection` `Temperature Monitoring` `Battery Monitoring` `OTA Updates` `Deep Sleep` `Low Power`
- **Application:** `Home Automation` `IoT` `Embedded Systems`
- **Tools:** `PlatformIO` `Arduino` `PlantUML` `NVS Storage`

An intelligent IoT lighting system utilizing ESP-NOW for mesh networking between ESP32 nodes and a coordinator, with real-time monitoring, MQTT integration, and web-based control interface.

## Overview

The IOT Smart Tile System is a distributed embedded IoT platform designed for intelligent lighting control and environmental monitoring. The system consists of ESP32-C3 nodes that communicate via ESP-NOW with an ESP32-S3 coordinator, which acts as a gateway to MQTT, enabling cloud connectivity and web-based management.

### Key Highlights

- **ESP-NOW mesh networking** for low-latency node-to-coordinator communication
- **MQTT integration** with hierarchical topic structure for cloud connectivity
- **Real-time web dashboard** built with Angular and WebSockets
- **Dual-axis environmental monitoring** with ambient light and temperature sensors
- **mmWave radar integration** for presence detection (HLK-LD2450)
- **Interactive pairing system** with visual LED feedback
- **Docker-based deployment** for backend services (Go API, MongoDB, Mosquitto)
- **Serial diagnostics** with structured telemetry logging

## Features

### Hardware Features
- **Coordinator (ESP32-S3)**:
  - ESP-NOW orchestration for multiple nodes
  - MQTT broker connectivity with TLS support
  - Ambient light sensor (analog ADC)
  - mmWave radar sensor (LD2450) for presence detection
  - Addressable RGB LED strips (NeoPixel)
  - Touch button controls for pairing
  - Serial diagnostic output
  
- **Nodes (ESP32-C3)**:
  - RGBW LED control (PWM-based)
  - TMP117 high-precision temperature sensor
  - Button input with debouncing
  - Battery voltage monitoring
  - Deep sleep mode for power optimization
  - OTA update capability

### Software Features
- Real-time bidirectional communication (Node ↔ Coordinator ↔ MQTT ↔ Backend ↔ Frontend)
- RESTful API with WebSocket support for live updates
- Persistent storage using NVS (Non-Volatile Storage) on ESP32
- Interactive WiFi provisioning via serial console
- Zone-based lighting control
- Thermal management and monitoring
- MongoDB for data persistence
- Docker Compose orchestration for easy deployment

## System Architecture

![IOT Smart Tile System Architecture](Diagrams/IOT%20Smart%20Tile%20System%20Architecture.png)

The system consists of 2 zones (Zone 1 and Zone N), each with an ESP32-S3 coordinator managing N ESP32-C3 nodes via ESP-NOW. Both coordinators connect to a centralized MQTT broker, which interfaces with the Go backend API and Angular frontend.

### Additional Diagrams

See `Diagrams/` folder for detailed data flow diagrams:
- **Node Telemetry Uplink Flow.png** - How nodes send sensor data to the backend
- **Coordinator Telemetry Flow.png** - Coordinator sensor data publishing
- **Node Pairing Flow.png** - Adding new nodes to a zone
- **Historical Data Query Flow.png** - Retrieving and visualizing historical data

## Hardware

### Coordinator Module
- **Board**: ESP32-S3-DevKitC-1
- **Flash**: 8MB
- **RAM**: 512KB SRAM
- **WiFi**: 2.4 GHz 802.11 b/g/n
- **Connectivity**: ESP-NOW + MQTT over WiFi

### Node Module
- **Board**: ESP32-C3-MINI-1
- **Flash**: 4MB
- **RAM**: 400KB SRAM
- **WiFi**: 2.4 GHz 802.11 b/g/n (ESP-NOW only)
- **Power**: Battery-powered with deep sleep support

### Sensors & Actuators

**Coordinator**:
- Ambient Light Sensor (Analog ADC on GPIO pin)
- TSL2561 Digital Light Sensor (I2C)
- HLK-LD2450 mmWave Radar (UART)
- Adafruit NeoPixel LED strips

**Nodes**:
- TMP117 High-Precision Temperature Sensor (I2C, ±0.1°C accuracy)
- RGBW LED Channels (PWM control)
- Tactile buttons with hardware debouncing
- Battery voltage divider circuit

## Project Structure

```
IOT-TileNodeCoordinator/
├── coordinator/                     # ESP32-S3 Coordinator firmware
│   ├── src/
│   │   ├── core/                    # Core coordinator logic
│   │   │   ├── Coordinator.cpp/h    # Main coordinator orchestrator
│   │   │   └── ...
│   │   ├── managers/                # Subsystem managers
│   │   │   ├── EspNowManager.cpp/h  # ESP-NOW communication
│   │   │   ├── MqttManager.cpp/h    # MQTT broker integration
│   │   │   ├── WifiManager.cpp/h    # WiFi provisioning & reconnect
│   │   │   ├── NodeRegistry.cpp/h   # Node pairing & tracking
│   │   │   ├── ZoneControl.cpp/h    # Zone-based lighting
│   │   │   ├── ThermalControl.cpp/h # Temperature management
│   │   │   └── ...
│   │   ├── sensors/                 # Sensor implementations
│   │   └── main.cpp                 # Entry point
│   ├── platformio.ini               # PlatformIO configuration
│   └── partitions.csv               # Flash partition table
│
├── node/                            # ESP32-C3 Node firmware
│   ├── src/
│   │   └── main.cpp                 # Node application
│   └── platformio.ini               # PlatformIO configuration
│
├── shared/                          # Shared library (coordinator + nodes)
│   ├── src/
│   │   ├── Pins.h                   # Hardware pin definitions
│   │   ├── Messages.h               # ESP-NOW message structures
│   │   ├── Logger.h                 # Unified logging system
│   │   └── ConfigManager.h          # NVS configuration wrapper
│   └── library.json                 # PlatformIO library manifest
│
├── IOT-Backend-main/                # Go backend API
│   └── IOT-Backend-main/
│       ├── cmd/iot/main.go          # Backend entry point
│       ├── internal/
│       │   ├── api/                 # REST endpoints
│       │   ├── mqtt/                # MQTT client
│       │   ├── websocket/           # WebSocket server
│       │   ├── database/            # MongoDB integration
│       │   └── config/              # Configuration files
│       ├── Dockerfile
│       └── go.mod
│
├── IOT-Frontend-main/               # Angular web dashboard
│   └── IOT-Frontend-main/
│       ├── src/
│       │   ├── app/                 # Angular components
│       │   ├── environments/        # Environment configs
│       │   └── ...
│       ├── Dockerfile
│       └── package.json
│
├── docs/                            # Documentation
│   ├── diagrams/                    # System diagrams
│   ├── Pics/                        # Photos and screenshots
│   └── development/                 # Developer notes
│
├── scripts/                         # Utility scripts
├── Datasheet/                       # Hardware datasheets
├── Test code/                       # Testing utilities
├── docker-compose.yml               # Docker orchestration
├── api-collection.json              # Postman API collection
└── README.md                        # This file
```

## Setup

### Prerequisites

- **PlatformIO** (VSCode extension or CLI)
- **Docker & Docker Compose** (for backend services)
- **Node.js 18+** (for frontend development)
- **Go 1.21+** (for backend development)
- **Git**

### 1. Backend & Infrastructure Setup

#### Start Docker Services

```bash
# Create .env file with your configuration
cp .env.example .env

# Start MongoDB, Mosquitto MQTT, Backend API, and Frontend
docker-compose up -d

# View logs
docker-compose logs -f
```

The services will be available at:
- **Frontend**: http://localhost:4200
- **Backend API**: http://localhost:8000
- **MQTT Broker**: localhost:1883 (username: user1, password: user1)
- **MongoDB**: localhost:27017

#### Manual Backend Setup (Alternative)

```bash
cd IOT-Backend-main/IOT-Backend-main

# Install dependencies
go mod download

# Run backend
go run cmd/iot/main.go
```

### 2. Coordinator Setup (ESP32-S3)

#### Configure WiFi & MQTT

The coordinator uses interactive serial provisioning. On first boot:

1. Connect via serial monitor (115200 baud)
2. Follow WiFi configuration prompts
3. MQTT credentials are stored in `ConfigManager` under the `"mqtt"` namespace

Alternatively, pre-configure via code before flashing.

#### Build and Upload

```bash
cd coordinator

# Build and upload firmware
pio run -e esp32-s3-devkitc-1 -t upload

# Open serial monitor
pio device monitor
```

#### Expected Serial Output

```
COORDINATOR INIT: Starting...
LOGGER: Begin
NVS: Initialized
WIFI: Connecting to <SSID>...
WIFI: Connected (IP: 192.168.1.100)
MQTT: Connecting to broker...
MQTT: Connected
COORDINATOR DATA: light=450, temp=22.5, mmwave=0
STATUS: wifi=connected, mqtt=connected, pairing=closed
```

### 3. Node Setup (ESP32-C3)

#### Build and Upload

```bash
cd node

# For normal operation
pio run -e esp32-c3-mini-1 -t upload

# For debug mode with verbose logging
pio run -e esp32-c3-mini-1-debug -t upload -t monitor
```

#### Pairing Process

1. **Trigger pairing on coordinator**:
   - Short-press touch button on coordinator, OR
   - Send MQTT command: `{"cmd":"pair","duration_ms":60000}`

2. **Pair the node**:
   - Power on the node
   - Node will automatically pair with coordinator
   - LED feedback: Group of 4 pixels will flash on coordinator
   - Serial output: `PAIRING MODE STARTED`, `NODE PAIRED: [MAC]`

3. **Verify pairing**:
   - Check serial output for `FETCHED NODE [MAC] DATA: ...`
   - Check MQTT topic: `site/{siteId}/node/{nodeId}/telemetry`

### 4. Frontend Setup (Development)

```bash
cd IOT-Frontend-main/IOT-Frontend-main

# Install dependencies
npm install

# Start development server
npm start

# Navigate to http://localhost:4200
```

## Usage

### Web Dashboard

Access the Angular dashboard at `http://localhost:4200`:

- **Live Telemetry**: View real-time data from all nodes and coordinator
- **Zone Control**: Manage lighting zones and scenes
- **Node Management**: Monitor node status, battery levels, and pairing
- **mmWave Visualization**: See presence detection events
- **Historical Data**: View temperature and light trends

### MQTT API

The system uses a hierarchical MQTT topic structure:

#### Telemetry Topics (Published by Coordinator)

```
site/{siteId}/coord/{coordId}/telemetry    # Coordinator sensors (light, temp, mmWave)
site/{siteId}/coord/{coordId}/mmwave       # mmWave radar events
site/{siteId}/node/{nodeId}/telemetry      # Node telemetry (RGBW, temp, button, voltage)
site/{siteId}/coord/{coordId}/status       # Coordinator status updates
```

#### Command Topics (Subscribed by Coordinator)

```
site/{siteId}/coord/{coordId}/cmd          # Coordinator commands (pairing, settings)
site/{siteId}/node/{nodeId}/cmd            # Node commands (LED control, sleep)
```

#### Example Payloads

**Coordinator Telemetry**:
```json
{
  "coordId": "coord001",
  "siteId": "site001",
  "timestamp": 1701234567890,
  "light": 450,
  "temperature": 22.5,
  "mmWave": {
    "present": true,
    "targetCount": 2
  },
  "wifi": {
    "rssi": -65,
    "connected": true
  }
}
```

**Node Telemetry**:
```json
{
  "nodeId": "node001",
  "macAddress": "AA:BB:CC:DD:EE:FF",
  "timestamp": 1701234567890,
  "rgbw": {
    "r": 255,
    "g": 128,
    "b": 64,
    "w": 200
  },
  "temperature": 23.2,
  "buttonPressed": false,
  "batteryVoltage": 3.7,
  "rssi": -72
}
```

**Pairing Command**:
```json
{
  "cmd": "pair",
  "duration_ms": 60000
}
```

**LED Control Command**:
```json
{
  "cmd": "setLed",
  "nodeId": "node001",
  "rgbw": {
    "r": 255,
    "g": 0,
    "b": 0,
    "w": 100
  }
}
```

### REST API

See `api-collection.json` for the complete Postman collection.

**Base URL**: `http://localhost:8000`

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/health` | GET | Health check |
| `/api/sites` | GET | List all sites |
| `/api/sites/{siteId}/coordinators` | GET | List coordinators |
| `/api/sites/{siteId}/nodes` | GET | List nodes |
| `/api/sites/{siteId}/nodes/{nodeId}` | GET | Get node details |
| `/api/sites/{siteId}/telemetry` | GET | Get historical telemetry |
| `/api/sites/{siteId}/zones` | GET/POST | Manage lighting zones |
| `/ws` | WebSocket | Real-time data stream |

### Serial Diagnostics

The coordinator outputs structured telemetry over serial (115200 baud):

```
COORDINATOR DATA: light=450, temp=22.5, mmwave=present
STATUS: wifi=connected, mqtt=connected, pairing=closed, nodes=3
FETCHED NODE [AA:BB:CC:DD:EE:FF] DATA: temp=23.2, rgbw=[255,128,64,200], rssi=-72
```

## Pin Configuration

### Coordinator (ESP32-S3)

```cpp
// I2C Bus
#define SDA_PIN 21
#define SCL_PIN 22

// Sensors
#define AMBIENT_LIGHT_ADC 36        // Analog light sensor
#define MMWAVE_RX 16                // LD2450 RX
#define MMWAVE_TX 17                // LD2450 TX

// LEDs
#define NEOPIXEL_PIN 48             // Addressable LED strip
#define STATUS_LED_PIN 2            // Status indicator

// Controls
#define TOUCH_BUTTON_PIN 0          // Pairing button
```

### Node (ESP32-C3)

```cpp
// I2C Bus (TMP117)
#define SDA_PIN 8
#define SCL_PIN 9

// LED PWM Channels
#define LED_R_PIN 2
#define LED_G_PIN 3
#define LED_B_PIN 4
#define LED_W_PIN 5

// Controls
#define BUTTON_PIN 6

// Battery Monitor
#define BATTERY_ADC 0               // ADC1_CH0
```

## Dependencies

### Firmware Libraries
- ESP32 Arduino Core (Espressif Systems)
- PlatformIO build system
- PubSubClient (MQTT client by Nick O'Leary)
- ArduinoJson (Benoit Blanchon)
- Adafruit sensor libraries (NeoPixel, TSL2561, TMP117, Unified Sensor)
- ld2450 library (mmWave radar by goodgod)

### Backend Stack
- Go 1.21+
- MongoDB 7.0
- Eclipse Mosquitto 2.0

### Frontend Stack
- Angular 15+
- Node.js 18+
