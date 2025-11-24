# Health Indicators Feature

## Overview
Added comprehensive health monitoring to the navbar showing real-time status of all system components.

## Health Indicators Displayed

### Individual Component Health
1. **API** - Backend REST API connectivity
2. **DB** - MongoDB database connection
3. **MQTT** - MQTT broker connectivity  
4. **Coordinator** - ESP32 coordinator online status (seen within last 5 minutes)

### System Status
- **System Active** - All critical components are healthy (API, DB, MQTT, WebSocket)
- **System Degraded** - One or more components are unhealthy

## Visual Design

### Health Indicators
- **Green dot** (✓) - Component is healthy
- **Red dot** (✗) - Component is unhealthy
- Hover shows full component name
- Compact design with labels: API, DB, MQTT, Coordinator

### System Status Indicator
- **White pulsing dot** + "System Active" - All systems operational
- **Red dot** + "System Degraded" - Issues detected
- Prominent placement after health indicators

## Implementation Details

### Backend Changes
**File**: `IOT-Backend-main/IOT-Backend-main/internal/http/handlers.go`

Enhanced `/health` endpoint to return:
```json
{
  "status": "healthy" | "degraded",
  "service": "iot-backend",
  "database": true,
  "mqtt": true,
  "coordinator": true,
  "timestamp": 1234567890
}
```

Health checks:
- **Database**: Queries sites collection with 2s timeout
- **MQTT**: Checks if client is connected
- **Coordinator**: Checks if any coordinator was seen in last 5 minutes

### Frontend Changes

#### Models
**File**: `src/app/core/models/api.models.ts`
- Updated `HealthStatus` interface with new fields

#### Data Service
**File**: `src/app/core/services/data.service.ts`
- Added signals: `dbHealthy`, `mqttBrokerHealthy`, `coordOnline`
- Enhanced `checkHealth()` to parse backend health response
- Updated `getSystemHealth()` to include all health metrics

#### Dashboard Component
**File**: `src/app/features/dashboard/dashboard.component.html`
- Replaced simple "System Active" indicator with detailed health grid
- Shows 4 individual component indicators + system status
- Responsive design with labels

**File**: `src/app/features/dashboard/dashboard.component.scss`
- Added `.health-indicators` container styles
- Individual `.health-item` with dot + label
- Color transitions: red (unhealthy) → green (healthy)
- Divider between component health and system status
- Mobile responsive (hides divider and time on small screens)

## Health Check Logic

### Overall System Health
System is considered **healthy** when:
- API is reachable
- WebSocket is connected
- MQTT WebSocket is connected
- Database is accessible
- MQTT broker is connected

Note: Coordinator status is informational only and doesn't affect overall system health, as the system can function without an active coordinator.

### Polling
- Backend health checked every 30 seconds (configurable via `healthCheckInterval`)
- Real-time updates via signals
- Automatic reconnection attempts for WebSocket/MQTT

## Usage

After rebuilding containers:
```bash
docker-compose build backend frontend
docker-compose up -d
```

Navigate to `http://localhost:4200` and check the navbar:
- Green indicators = healthy
- Red indicators = unhealthy
- "System Active" appears when all critical components are online

## Troubleshooting

### Database shows red
- Check MongoDB container: `docker logs iot-mongodb`
- Verify connection string in backend environment

### MQTT shows red  
- Check Mosquitto container: `docker logs iot-mosquitto`
- Verify MQTT credentials and broker address

### Coordinator shows red
- Normal if no ESP32 coordinator is connected
- Coordinator must publish telemetry within 5 minutes to show green
- Check coordinator logs via serial monitor

### API shows red
- Check backend container: `docker logs iot-backend`
- Verify backend is listening on port 8000
- Check nginx proxy configuration

## Benefits

1. **Immediate visibility** - Operators see system health at a glance
2. **Detailed diagnostics** - Individual component status helps isolate issues
3. **Real-time updates** - Health indicators update automatically
4. **Professional UI** - Polished design with smooth transitions
5. **Mobile friendly** - Responsive design for all screen sizes
