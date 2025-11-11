# IOT Smart Tile Lighting System

> A complete IoT lighting system featuring ESP32-powered smart tiles with presence detection, real-time telemetry, and cloud-based control.

[![CI/CD](https://github.com/DICEsda/IOT-TileNodeCoordinator/actions/workflows/ci-cd.yml/badge.svg)](https://github.com/DICEsda/IOT-TileNodeCoordinator/actions)

## Overview

This project implements a battery-powered smart indoor lighting system that combines edge computing with cloud infrastructure. The system uses ESP32 microcontrollers to manage RGBW LED tiles, detect room occupancy with mmWave sensors, and communicate with a cloud backend for monitoring and control.

Key features:
- Presence-based RGBW lighting triggered by room occupancy
- ESP-NOW mesh network with low-latency encrypted communication
- Battery-powered nodes with thermal and power management
- Cloud integration via MQTT and REST API
- Google Home integration
- OTA updates with rollback support
- Real-time Angular dashboard

## Architecture and diagrams

The ASCII architecture diagram has been removed. Refer to the PlantUML diagrams in `docs/diagrams`:

- `docs/diagrams/class_diagram.puml` — class relationships and main components
- `docs/diagrams/presence_detection.puml` — presence detection flow
- `docs/diagrams/pairing_sequence.puml` — secure pairing sequence
- `docs/diagrams/thermal_management.puml` — thermal derating logic
- `docs/diagrams/connectivity_sequence.puml` — coordinator, MQTT, and node connectivity sequence

### System components

Hardware layer
- Light Nodes (ESP32-C3): Battery-powered tiles with 4-LED SK6812B RGBW strips, temperature sensors, and ESP-NOW communication
- Coordinator (ESP32-S3): Central hub with mmWave presence detection, WiFi connectivity, and MQTT bridge

Communication layer
- ESP-NOW: Low-latency encrypted mesh network for node-to-coordinator communication (< 180 ms latency)
- MQTT: Pub/sub messaging for cloud telemetry and command distribution
- WebSocket: Real-time bidirectional communication for web dashboard

Application layer
- Backend (Go): REST API, MQTT handlers, OAuth2 authentication, OTA management
- Frontend (Angular 19): Responsive web dashboard with real-time monitoring
- Database (MongoDB): Persistent storage for telemetry, configuration, and device state

## Quick Start

### Prerequisites

- Docker and Docker Compose (for running services)
- Git (for cloning the repository)
- Optional: PlatformIO or ESP-IDF 5.x (for firmware development)

### Installation

1. Clone the repository
   ```bash
   git clone https://github.com/DICEsda/IOT-TileNodeCoordinator.git
   cd IOT-TileNodeCoordinator
   ```

2. Run the quick start script (Windows)
   ```batch
   quick-start.bat
   ```

   Or manually:
   ```bash
   # Copy environment template
   cp .env.example .env
   
   # Start all services
   docker compose up -d
   ```

3. Access the services
   - Frontend Dashboard: http://localhost:4200
   - Backend API: http://localhost:8000
   - API Health Check: http://localhost:8000/health
   - MQTT Broker: mqtt://localhost:1883

### Testing the system

```bash
# View backend logs
docker compose logs -f backend

# Check service health
curl http://localhost:8000/health

# Access MongoDB
docker exec -it iot-mongodb mongosh
```

### Stopping services

```batch
docker compose down
```

## Use cases

1. Smart office lighting: Automatic desk lighting that adapts to occupancy and time of day
2. Home automation: Integration with Google Home for voice-controlled ambient lighting
3. Energy efficiency: Battery-powered operation with intelligent power management
4. Real-time monitoring: Live telemetry dashboard for system health and usage analytics

## Technology stack

### Embedded systems
- ESP-IDF 5.x: Framework for ESP32 development
- FreeRTOS: Real-time operating system for task management
- ESP-NOW: Proprietary protocol for peer-to-peer communication
- PlatformIO: Development platform for embedded systems

### Backend
- Go 1.21+: High-performance backend service
- MQTT (Mosquitto): Message broker for IoT communication
- MongoDB: NoSQL database for flexible data storage
- Docker: Containerization for easy deployment

### Frontend
- Angular 19: Modern web framework with signals and TypeScript
- RxJS: Reactive programming for real-time data streams
- Three.js: 3D visualization for room layout
- WebSocket: Real-time bidirectional communication

### Cloud and integration
- Google Home API: Smart home device integration
- OAuth2: Secure authentication and authorization
- GitHub Actions: CI/CD pipeline for automated testing and deployment

## Features in detail

### Presence detection
- mmWave sensor for accurate occupancy detection
- Zone-based presence mapping for multi-room support
- Configurable debounce and hold times (150 ms / 5000 ms)

### RGBW lighting control
- 4-channel color control (Red, Green, Blue, White)
- Smooth fade transitions (configurable 150–1000 ms)
- Multiple patterns: uniform, gradient, indexed
- Visual status indicators (pairing, OTA, errors)

### Power management
- Battery-aware operation with voltage monitoring
- Temperature-based derating (70–85 °C range)
- Light sleep mode with periodic wake for commands

### Security
- Per-node encryption with long-term keys
- TLS for MQTT communication
- Signed firmware for OTA updates
- Secure pairing with physical button confirmation

### Real-time telemetry
- Node telemetry: LED state, temperature, battery voltage, firmware version
- Coordinator telemetry: online nodes, WiFi RSSI, event rates
- mmWave events: zone presence with confidence scores
- 30-second reporting interval (configurable)

## API overview

### REST endpoints

```http
GET  /health                      # Service health status
GET  /sites                       # List all sites
GET  /sites/{id}                  # Get site details
GET  /nodes/{id}                  # Get node details
GET  /coordinators/{id}           # Get coordinator details
POST /color-profile               # Update color profile
POST /set-light                   # Control node lighting
POST /pairing/approve             # Approve node pairing
POST /ota/start                   # Initiate firmware update
GET  /ota/status                  # Check OTA status
WS   /ws                          # WebSocket connection
```

### MQTT topics

Uplink (Device → Cloud)
```
site/{siteId}/node/{nodeId}/telemetry       # Node telemetry
site/{siteId}/coord/{coordId}/telemetry     # Coordinator telemetry
site/{siteId}/coord/{coordId}/mmwave        # Presence events
```

Downlink (Cloud → Device)
```
site/{siteId}/coord/{coordId}/cmd           # Coordinator commands
site/{siteId}/node/{nodeId}/cmd             # Node commands
```

For complete API documentation, see `docs/mqtt_api.md`.

## Development

### Firmware development

Coordinator (ESP32-S3)
```bash
cd coordinator
pio run -t upload -t monitor
```

Node (ESP32-C3)
```bash
cd node
pio run -t upload -t monitor
```

### Backend development

```bash
cd IOT-Backend-main/IOT-Backend-main
go mod download
go run cmd/iot/main.go
```

### Frontend development

```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm install
npm start
# Access at http://localhost:4200
```

### Running tests

```bash
# Backend tests
cd IOT-Backend-main/IOT-Backend-main
go test ./...

# Frontend tests
cd IOT-Frontend-main/IOT-Frontend-main
npm test
```

## Documentation

- Product Requirements: `docs/ProductRequirementDocument.md`
- MQTT API Reference: `docs/mqtt_api.md`
- Deployment Guide: `DEPLOYMENT.md`
- Google Home Setup: `GOOGLE_HOME_SETUP.md`
- System Diagrams: `docs/diagrams/`

## Project structure

```
IOT-TileNodeCoordinator/
├── coordinator/
├── node/
├── IOT-Backend-main/
├── IOT-Frontend-main/
├── shared/
├── docs/
│   ├── ProductRequirementDocument.md
│   ├── mqtt_api.md
│   └── diagrams/
├── docker-compose.yml
├── quick-start.bat
├── DEPLOYMENT.md
├── GOOGLE_HOME_SETUP.md
└── README.md
```

## Project status

| Component | Status | Notes |
|-----------|--------|-------|
| Hardware Integration | In Progress | ESP32-C3/S3 firmware operational |
| ESP-NOW Communication | Complete | Encrypted, reliable mesh network |
| MQTT Bridge | In Progress | Real-time telemetry streaming |
| Backend API | Complete | All endpoints implemented |
| Frontend Dashboard | Complete | Real-time monitoring and control |
| Google Home | In Progress | Voice control integration |
| OTA Updates | Complete | Wireless firmware deployment |
| Docker Deployment | Complete | One-command setup |



### Development setup

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-change`)
3. Commit changes with descriptive messages
4. Push to the branch (`git push origin feature/your-change`)
5. Open a Pull Request