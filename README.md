# IOT Smart Tile Lighting System

A complete battery-powered smart indoor lighting system with ESP32-C3 light nodes, ESP32-S3 coordinator, and cloud backend infrastructure.

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
            │   MongoDB    │  │   Frontend   │  │   WebSocket  │
            │   Database   │  │   (Angular)  │  │   Clients    │
            └──────────────┘  └──────────────┘  └──────────────┘
```

## 📋 Features

- **Presence-based RGBW lighting** with mmWave sensor integration
- **Secure ESP-NOW communication** between nodes and coordinator
- **Real-time telemetry** via MQTT
- **Web-based dashboard** for monitoring and control
- **OTA firmware updates** for nodes and coordinator
- **Battery-aware operation** with thermal management
- **WebSocket support** for real-time UI updates
- **Google Home integration** for voice control and automation

## 🚀 Quick Start

### Prerequisites

- Docker and Docker Compose
- (Optional) ESP-IDF 5.x for firmware development
- (Optional) PlatformIO for ESP32 programming

### Running with Docker

1. **Clone the repository**
   ```bash
   git clone https://github.com/DICEsda/IOT-TileNodeCoordinator.git
   cd IOT-TileNodeCoordinator
   ```

2. **Configure environment** (Windows)
  - The quick start script will create a `.env` from `.env.example` if missing.
  - You can still do it manually if you prefer:
    ```batch
    copy .env.example .env
    REM Edit .env if needed
    ```

3. **Build and run**
  ```batch
  quick-start.bat
  ```

4. **Access the system**
   - Frontend: http://localhost:4200
   - Backend API: http://localhost:8000
   - MQTT Broker: mqtt://localhost:1883
   - MongoDB: mongodb://localhost:27017

### Stopping Services

```batch
docker-stop.bat
```

Or manually:
```batch
docker compose down
```

## 📡 MQTT Topics

Following the PRD specification:

### Uplink (Device → Cloud)
- `site/{siteId}/node/{nodeId}/telemetry` - Node telemetry data
- `site/{siteId}/coord/{coordId}/telemetry` - Coordinator telemetry
- `site/{siteId}/coord/{coordId}/mmwave` - Presence detection events

### Downlink (Cloud → Device)
- `site/{siteId}/coord/{coordId}/cmd` - Commands to coordinator
- `site/{siteId}/node/{nodeId}/cmd` - Commands to specific nodes

## 🔧 API Endpoints

### Health Check
- `GET /health` - Service health status

### Sites
- `GET /sites` - List all sites
- `GET /sites/{id}` - Get site by ID

### Coordinators
- `GET /coordinators/{id}` - Get coordinator details

### Nodes
- `GET /nodes/{id}` - Get node details

### Commands
- `POST /color-profile` - Update color profile
- `POST /set-light` - Control node lighting
- `POST /pairing/approve` - Approve node pairing

### Google Home
- `POST /api/v1/google/home/fulfillment` - Smart Home fulfillment
- `GET /oauth/google/authorize` - OAuth authorization
- `POST /oauth/google/token` - OAuth token exchange
- `POST /api/v1/google/home/report-state` - Report device state
- `POST /api/v1/google/home/request-sync` - Request device sync

### OTA
- `POST /ota/start` - Initiate OTA update
- `GET /ota/status` - Check OTA status

### WebSocket
- `WS /ws` - Real-time telemetry stream

## 🛠️ Development

### Backend (Go)

```bash
cd IOT-Backend-main/IOT-Backend-main
go mod download
go run cmd/iot/main.go
```

### Frontend (Angular)

```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm install
npm start
```

### Coordinator Firmware (ESP32-S3)

```bash
cd coordinator
pio run -t upload -t monitor
```

### Node Firmware (ESP32-C3)

```bash
cd node
pio run -t upload -t monitor
```

## 📦 Project Structure

```
IOT-TileNodeCoordinator/
├── coordinator/           # ESP32-S3 coordinator firmware
│   ├── src/
│   │   ├── main.cpp
│   │   ├── core/         # Core coordinator logic
│   │   ├── comm/         # ESP-NOW & MQTT
│   │   └── sensors/      # mmWave integration
│   └── platformio.ini
├── node/                  # ESP32-C3 node firmware
│   ├── src/
│   │   ├── main.cpp
│   │   ├── led/          # SK6812B LED control
│   │   ├── sensor/       # Temperature sensors
│   │   └── power/        # Power management
│   └── platformio.ini
├── IOT-Backend-main/      # Go backend service
│   ├── cmd/iot/          # Main entry point
│   ├── internal/
│   │   ├── config/       # Configuration
│   │   ├── db/           # MongoDB connection
│   │   ├── http/         # HTTP handlers
│   │   ├── mqtt/         # MQTT handlers
│   │   ├── repository/   # Data access layer
│   │   └── types/        # Data models
│   ├── Dockerfile
│   └── go.mod
├── IOT-Frontend-main/     # Angular frontend
│   ├── src/
│   │   └── app/
│   │       ├── core/           # Auth & services
│   │       │   ├── models/     # TypeScript data models
│   │       │   │   └── api.models.ts
│   │       │   └── services/   # Core services
│   │       │       ├── environment.service.ts  # Config management
│   │       │       ├── api.service.ts          # HTTP client
│   │       │       ├── websocket.service.ts    # Real-time WS
│   │       │       ├── mqtt.service.ts         # MQTT pub/sub
│   │       │       ├── data.service.ts         # High-level orchestration
│   │       │       └── README.md               # Service documentation
│   │       └── features/       # Dashboard, devices, logs
│   ├── Dockerfile
│   ├── nginx.conf
│   └── package.json
├── shared/                # Shared libraries (ESP-NOW messages)
├── docs/                  # Documentation
│   ├── ProductRequirementDocument.md
│   ├── mqtt_api.md
│   └── diagrams/
├── docker-compose.yml     # Docker orchestration
├── .env.example          # Environment template
└── README.md             # This file
```

## 🔐 Security

- **ESP-NOW Encryption**: Per-node LMK with PMK in secure partition
- **MQTT TLS**: Secure MQTT communication (configure in production)
- **Signed Firmware**: OTA updates with signature verification
- **Access Control**: Backend authentication (implement as needed)

## 📊 Telemetry Schema

### Node Telemetry
```json
{
  "ts": 1693560000,
  "node_id": "C3DDEE",
  "light_id": "L12",
  "avg_r": 240,
  "avg_g": 180,
  "avg_b": 120,
  "avg_w": 255,
  "status_mode": "operational",
  "temp_c": 24.2,
  "vbat_mv": 3700,
  "fw": "c3-1.0.2"
}
```

### Coordinator Telemetry
```json
{
  "ts": 1693560000,
  "fw": "s3-1.2.1",
  "nodes_online": 18,
  "wifi_rssi": -58,
  "mmwave_event_rate": 0.5
}
```

### mmWave Events
```json
{
  "ts": 1693560000,
  "events": [
    {
      "zone": 3,
      "presence": true,
      "confidence": 0.82
    }
  ]
}
```

## 🧪 Testing

### Backend Tests
```bash
cd IOT-Backend-main/IOT-Backend-main
go test ./...
```

### Frontend Tests
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm test
```

### Integration Tests
1. Start all services with `docker-run.bat`
2. Flash coordinator and nodes with firmware
3. Verify telemetry flow in backend logs
4. Check frontend dashboard for real-time updates

## 💻 Frontend Development

The Angular frontend provides real-time monitoring and control through five core services:

### Service Architecture

```
Components
    ↓
DataService (High-level orchestration)
    ↓
┌────────────────┬──────────────────┬─────────────────┐
│  ApiService    │ WebSocketService │   MqttService   │
│  (HTTP/REST)   │  (Real-time)     │   (Pub/Sub)     │
└────────────────┴──────────────────┴─────────────────┘
```

### Core Services

1. **EnvironmentService** - Configuration management
2. **ApiService** - Type-safe HTTP client for REST API
3. **WebSocketService** - Real-time bidirectional communication
4. **MqttService** - MQTT pub/sub via WebSocket bridge
5. **DataService** - High-level orchestration with caching

### Quick Example

```typescript
import { Component, inject } from '@angular/core';
import { DataService } from './core/services/data.service';

@Component({
  selector: 'app-dashboard',
  template: `
    <h1>Sites: {{ data.sites().length }}</h1>
    <p>System Health: {{ getHealth() }}</p>
  `
})
export class DashboardComponent {
  data = inject(DataService);

  ngOnInit() {
    this.data.loadSites();
  }

  getHealth() {
    const health = this.data.getSystemHealth();
    return health.overall ? '✓ Connected' : '✗ Disconnected';
  }

  async controlLight(nodeId: string) {
    await this.data.setNodeLight({
      node_id: nodeId,
      site_id: 'site-001',
      rgbw: { r: 255, g: 0, b: 0, w: 0 },
      brightness: 80
    });
  }
}
```

### Service Features

**ApiService:**
- Auto timeout handling
- Authorization token injection
- Type-safe responses
- Error handling

**WebSocketService:**
- Automatic reconnection
- Connection state signals
- Typed message streams
- Exponential backoff

**MqttService:**
- Topic wildcards (+ and #)
- Auto resubscription
- QoS support
- Helper methods for common topics

**DataService:**
- Unified state management
- Real-time cache updates
- Health monitoring
- Simplified API for components

### Available API Methods

```typescript
// Sites
getSites(): Observable<Site[]>
getSiteById(id: string): Observable<Site>

// Nodes
getNodeById(id: string): Observable<Node>
setLight(command: SetLightCommand): Observable<any>

// Coordinators
getCoordinatorById(id: string): Observable<Coordinator>

// Commands
approveNodePairing(approval: PairingApproval): Observable<any>
sendColorProfile(command: ColorProfileCommand): Observable<any>

// OTA
startOTAUpdate(request: StartOTARequest): Observable<OTAJob>
getOTAJobStatus(jobId: string): Observable<OTAJob>

// Google Home
reportGoogleHomeState(userId, deviceId, state): Observable<any>
requestGoogleHomeSync(userId): Observable<any>
```

### MQTT Topics

Following PRD specification:

**Telemetry (Subscribe):**
```typescript
mqtt.subscribeNodeTelemetry('site-001', 'node-001')
mqtt.subscribeAllNodesTelemetry('site-001')
mqtt.subscribeCoordinatorTelemetry('site-001', 'coord-001')
mqtt.subscribeZonePresence('site-001', 'zone-living-room')
mqtt.subscribePairingRequests('site-001')
```

**Commands (Publish):**
```typescript
mqtt.sendNodeCommand('site-001', 'node-001', {
  type: 'set_light',
  rgbw: { r: 255, g: 100, b: 0, w: 50 },
  brightness: 80
})

mqtt.sendZoneCommand('site-001', 'zone-living-room', {
  type: 'enable',
  fade_duration: 500
})
```

### Data Models

Comprehensive TypeScript interfaces in `core/models/api.models.ts`:

- Site, Zone
- Coordinator, CoordinatorTelemetry
- Node, NodeTelemetry, RGBWState
- SetLightCommand, ColorProfileCommand, PairingApproval
- PresenceEvent, OTAJob
- HealthStatus, WSMessage
- GoogleHomeDevice, GoogleHomeState

### Development Setup

```bash
cd IOT-Frontend-main/IOT-Frontend-main

# Install dependencies
npm install

# Start dev server
npm start

# Build for production
npm run build

# Run tests
npm test
```

For detailed service documentation, see [Frontend Services README](IOT-Frontend-main/IOT-Frontend-main/src/app/core/services/README.md).

## 🧪 Testing

### Backend Tests
```bash
cd IOT-Backend-main/IOT-Backend-main
go test ./...
```

### Frontend Tests
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm test
```

### Integration Tests
1. Start all services with `docker-run.bat`
2. Flash coordinator and nodes with firmware
3. Verify telemetry flow in backend logs
4. Check frontend dashboard for real-time updates

## 📝 Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `HTTP_ADDR` | `:8000` | Backend HTTP server address |
| `MQTT_BROKER` | `tcp://mosquitto:1883` | MQTT broker URL |
| `MQTT_USERNAME` | `user1` | MQTT username |
| `MQTT_PASSWORD` | `user1` | MQTT password |
| `MONGO_URI` | `mongodb://admin:admin123@mongodb:27017` | MongoDB connection string |
| `MONGO_DB` | `iot_smarttile` | MongoDB database name |
| `API_URL` | `http://localhost:8000` | Backend API URL (frontend) |
| `WS_URL` | `ws://localhost:8000/ws` | WebSocket URL (frontend) |
| `ESP32_WIFI_SSID` | - | WiFi SSID for ESP32 devices |
| `ESP32_WIFI_PASSWORD` | - | WiFi password for ESP32 devices |
| `GOOGLE_HOME_ENABLED` | `false` | Enable Google Home integration |
| `GOOGLE_HOME_PROJECT_ID` | - | Google Cloud project ID |
| `GOOGLE_HOME_CLIENT_ID` | - | OAuth client ID |
| `GOOGLE_HOME_CLIENT_SECRET` | - | OAuth client secret |

## 🐛 Troubleshooting

### Docker Issues
```batch
REM Clean rebuild
docker-compose down -v
docker-compose build --no-cache
docker-compose up -d
```

### Backend Not Connecting to MongoDB
- Check MongoDB is healthy: `docker-compose ps`
- Verify connection string in `.env`
- Check logs: `docker-compose logs backend`

### MQTT Connection Failed
- Ensure Mosquitto is running: `docker-compose ps mosquitto`
- Check MQTT credentials in `.env`
- Verify port 1883 is not blocked

### Frontend Not Loading
- Check backend is accessible: `curl http://localhost:8000/health`
- Verify nginx configuration in `IOT-Frontend-main/IOT-Frontend-main/nginx.conf`
- Check browser console for errors

## 📖 Documentation

### Core Documentation
- [Product Requirement Document](docs/ProductRequirementDocument.md) - Complete system requirements
- [MQTT API Reference](docs/mqtt_api.md) - MQTT topics and message formats
- [System Diagrams](docs/diagrams/) - Architecture and sequence diagrams

### Setup & Deployment
- [Deployment Guide](DEPLOYMENT.md) - Complete deployment instructions
- [Google Home Setup](GOOGLE_HOME_SETUP.md) - Voice control integration
- [Project Status](PROJECT_STATUS.md) - Current implementation status
- [Checklist](CHECKLIST.md) - Setup and verification checklist

### Testing & Development
- [API Collection](api-collection.json) - Postman/Thunder Client collection
- [Quick Start Script](quick-start.bat) - Automated setup for Windows

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## 📄 License

[Add your license here]

## 👥 Authors

- Daahir Hussein (Product Owner)

## 🙏 Acknowledgments

- ESP-IDF framework
- PlatformIO
- Angular team
- Go community
