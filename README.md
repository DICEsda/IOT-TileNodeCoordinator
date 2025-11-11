# IOT Smart Tile Lighting System

> A complete IoT lighting system featuring ESP32-powered smart tiles with presence detection, real-time telemetry, and cloud-based control.

[![CI/CD](https://github.com/DICEsda/IOT-TileNodeCoordinator/actions/workflows/ci-cd.yml/badge.svg)](https://github.com/DICEsda/IOT-TileNodeCoordinator/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## 📖 Overview

This project implements a battery-powered smart indoor lighting system that combines edge computing with cloud infrastructure. The system uses ESP32 microcontrollers to manage RGBW LED tiles, detect room occupancy with mmWave sensors, and communicate with a cloud backend for monitoring and control.

**Key Features:**
- 🎨 **Presence-based RGBW lighting** - Automatic color-adaptive lighting triggered by room occupancy
- 📡 **ESP-NOW mesh network** - Low-latency, encrypted peer-to-peer communication between devices
- 🔋 **Battery-powered nodes** - Energy-efficient operation with thermal management
- ☁️ **Cloud-connected** - Real-time telemetry and remote control via MQTT and REST API
- 🏠 **Google Home integration** - Voice control and smart home automation
- 🔄 **OTA updates** - Wireless firmware updates with rollback support
- 📊 **Real-time dashboard** - Angular-based web interface with live monitoring

## 🏗️ Architecture

```
┌─────────────────┐
│   ESP32 Nodes   │ ──── ESP-NOW ──── ┐
│   (SK6812B LED) │                   │
└─────────────────┘                   ▼
                              ┌──────────────────┐
┌─────────────────┐           │  ESP32-S3        │
│  mmWave Sensor  │ ────────► │  Coordinator     │
└─────────────────┘           └──────────────────┘
                                      │ WiFi/MQTT
                                      ▼
                              ┌──────────────────┐
                              │  MQTT Broker     │
                              │  (Mosquitto)     │
                              └──────────────────┘
                                      │
                                      ▼
                              ┌──────────────────┐
                              │  Backend API     │
                              │  (Go)            │
                              └──────────────────┘
                                      │
                    ┌─────────────────┼─────────────────┐
                    ▼                 ▼                 ▼
            ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
            │   MongoDB    │  │   Frontend   │  │  Google Home │
            │   Database   │  │   (Angular)  │  │  Integration │
            └──────────────┘  └──────────────┘  └──────────────┘
```

### System Components

#### **Hardware Layer**
- **Light Nodes (ESP32-C3)**: Battery-powered tiles with 4-LED SK6812B RGBW strips, temperature sensors, and ESP-NOW communication
- **Coordinator (ESP32-S3)**: Central hub with mmWave presence detection, WiFi connectivity, and MQTT bridge

#### **Communication Layer**
- **ESP-NOW**: Low-latency encrypted mesh network for node-to-coordinator communication (< 180ms latency)
- **MQTT**: Pub/sub messaging for cloud telemetry and command distribution
- **WebSocket**: Real-time bidirectional communication for web dashboard

#### **Application Layer**
- **Backend (Go)**: REST API, MQTT handlers, OAuth2 authentication, OTA management
- **Frontend (Angular 19)**: Responsive web dashboard with real-time monitoring
- **Database (MongoDB)**: Persistent storage for telemetry, configuration, and device state

## 🚀 Quick Start

### Prerequisites

- **Docker** and **Docker Compose** (for running services)
- **Git** (for cloning the repository)
- Optional: **PlatformIO** or **ESP-IDF 5.x** (for firmware development)

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/DICEsda/IOT-TileNodeCoordinator.git
   cd IOT-TileNodeCoordinator
   ```

2. **Run the quick start script** (Windows)
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

3. **Access the services**
   - **Frontend Dashboard**: http://localhost:4200
   - **Backend API**: http://localhost:8000
   - **API Health Check**: http://localhost:8000/health
   - **MQTT Broker**: mqtt://localhost:1883

### Testing the System

```bash
# View backend logs
docker compose logs -f backend

# Check service health
curl http://localhost:8000/health

# Access MongoDB
docker exec -it iot-mongodb mongosh
```

### Stopping Services

```batch
docker compose down
```

## 💡 Use Cases

1. **Smart Office Lighting**: Automatic desk lighting that adapts to occupancy and time of day
2. **Home Automation**: Integration with Google Home for voice-controlled ambient lighting
3. **Energy Efficiency**: Battery-powered operation with intelligent power management
4. **Real-time Monitoring**: Live telemetry dashboard for system health and usage analytics

## 🛠️ Technology Stack

### Embedded Systems
- **ESP-IDF 5.x**: Framework for ESP32 development
- **FreeRTOS**: Real-time operating system for task management
- **ESP-NOW**: Proprietary protocol for peer-to-peer communication
- **PlatformIO**: Development platform for embedded systems

### Backend
- **Go 1.21+**: High-performance backend service
- **MQTT (Mosquitto)**: Message broker for IoT communication
- **MongoDB**: NoSQL database for flexible data storage
- **Docker**: Containerization for easy deployment

### Frontend
- **Angular 19**: Modern web framework with signals and TypeScript
- **RxJS**: Reactive programming for real-time data streams
- **Three.js**: 3D visualization for room layout
- **WebSocket**: Real-time bidirectional communication

### Cloud & Integration
- **Google Home API**: Smart home device integration
- **OAuth2**: Secure authentication and authorization
- **GitHub Actions**: CI/CD pipeline for automated testing and deployment

## 📊 Features in Detail

### Presence Detection
- mmWave sensor for accurate occupancy detection (no false triggers)
- Zone-based presence mapping for multi-room support
- Configurable debounce and hold times (150ms/5000ms)

### RGBW Lighting Control
- 4-channel color control (Red, Green, Blue, White)
- Smooth fade transitions (configurable 150-1000ms)
- Multiple patterns: uniform, gradient, indexed
- Visual status indicators (pairing, OTA, errors)

### Power Management
- Battery-aware operation with voltage monitoring
- Temperature-based derating (70-85°C range)
- Light sleep mode with periodic wake for commands
- Typical battery life: 6-12 months (depending on usage)

### Security
- Per-node encryption with Long-term Keys (LMK)
- TLS for MQTT communication
- Signed firmware for OTA updates
- Secure pairing with physical button confirmation

### Real-time Telemetry
- Node telemetry: LED state, temperature, battery voltage, firmware version
- Coordinator telemetry: online nodes, WiFi RSSI, event rates
- mmWave events: zone presence with confidence scores
- 30-second reporting interval (configurable)

## 📡 API Overview

### REST Endpoints

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

### MQTT Topics

**Uplink (Device → Cloud)**
```
site/{siteId}/node/{nodeId}/telemetry       # Node telemetry
site/{siteId}/coord/{coordId}/telemetry     # Coordinator telemetry
site/{siteId}/coord/{coordId}/mmwave        # Presence events
```

**Downlink (Cloud → Device)**
```
site/{siteId}/coord/{coordId}/cmd           # Coordinator commands
site/{siteId}/node/{nodeId}/cmd             # Node commands
```

For complete API documentation, see [docs/mqtt_api.md](docs/mqtt_api.md).

## 🔧 Development

### Firmware Development

**Coordinator (ESP32-S3)**
```bash
cd coordinator
pio run -t upload -t monitor
```

**Node (ESP32-C3)**
```bash
cd node
pio run -t upload -t monitor
```

### Backend Development

```bash
cd IOT-Backend-main/IOT-Backend-main
go mod download
go run cmd/iot/main.go
```

### Frontend Development

```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm install
npm start
# Access at http://localhost:4200
```

### Running Tests

```bash
# Backend tests
cd IOT-Backend-main/IOT-Backend-main
go test ./...

# Frontend tests
cd IOT-Frontend-main/IOT-Frontend-main
npm test
```

## 📚 Documentation

- **[Product Requirements](docs/ProductRequirementDocument.md)** - Complete system specifications
- **[MQTT API Reference](docs/mqtt_api.md)** - Detailed message schemas and topics
- **[Deployment Guide](DEPLOYMENT.md)** - Production deployment instructions
- **[Google Home Setup](GOOGLE_HOME_SETUP.md)** - Voice control integration guide
- **[System Diagrams](docs/diagrams/)** - Architecture and sequence diagrams
- **[Development Notes](docs/development/)** - Technical implementation details

## 🎯 Project Status

✅ **Production Ready** - The core system is fully functional and deployed

| Component | Status | Notes |
|-----------|--------|-------|
| Hardware Integration | ✅ Complete | ESP32-C3/S3 firmware operational |
| ESP-NOW Communication | ✅ Complete | Encrypted, reliable mesh network |
| MQTT Bridge | ✅ Complete | Real-time telemetry streaming |
| Backend API | ✅ Complete | All endpoints implemented |
| Frontend Dashboard | ✅ Complete | Real-time monitoring and control |
| Google Home | ✅ Complete | Voice control integration |
| OTA Updates | ✅ Complete | Wireless firmware deployment |
| Docker Deployment | ✅ Complete | One-command setup |

## 🤝 Contributing

This is a showcase project demonstrating full-stack IoT development capabilities. Contact information is available through GitHub.

### Development Setup

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes with descriptive messages
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👤 Author

**Daahir Hussein**
- Role: Software Engineer & Product Owner
- GitHub: [@DICEsda](https://github.com/DICEsda)
- Project: Full-stack IoT System Development

### Skills Demonstrated

- **Embedded Systems**: ESP32 firmware development with ESP-IDF and FreeRTOS
- **Backend Development**: Go microservices with MQTT and REST APIs
- **Frontend Development**: Angular 19 with reactive programming (RxJS)
- **IoT Architecture**: Edge computing, mesh networks, cloud integration
- **DevOps**: Docker containerization, CI/CD with GitHub Actions
- **Cloud Integration**: OAuth2, Google Home API, WebSocket communication
- **System Design**: Scalable architecture with real-time telemetry

## 🙏 Acknowledgments

- ESP32 development community for ESP-IDF framework
- PlatformIO for the excellent embedded development platform
- Angular team for the modern web framework
- Go community for the robust backend ecosystem

---

**Note for Recruiters**: This project demonstrates end-to-end system design and implementation, from embedded firmware to cloud infrastructure. It showcases proficiency in embedded systems, backend development, frontend engineering, IoT protocols, and DevOps practices. The system is fully functional and deployed in production environments.
