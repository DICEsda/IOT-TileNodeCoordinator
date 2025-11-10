# Frontend Services Documentation

This directory contains the core services for the IoT Tile System frontend application.

## Architecture Overview

The frontend uses a layered service architecture:

```
┌─────────────────────────────────────────────────────────┐
│                     Components                           │
│              (Dashboard, Visualizer, etc.)              │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   DataService                            │
│          (High-level orchestration layer)               │
└─────────────────────────────────────────────────────────┘
                          ↓
┌──────────────┬──────────────────┬──────────────────────┐
│  ApiService  │ WebSocketService │    MqttService       │
│   (HTTP)     │   (Real-time)    │  (Pub/Sub Events)    │
└──────────────┴──────────────────┴──────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                Backend API + MQTT Broker                 │
└─────────────────────────────────────────────────────────┘
```

## Services

### 1. EnvironmentService
**Location:** `environment.service.ts`

Manages environment-specific configuration and feature flags.

**Configuration:**
```typescript
apiUrl: 'http://localhost:8080/api/v1'
wsUrl: 'ws://localhost:8080/ws'
mqttWsUrl: 'ws://localhost:8080/mqtt'
enableGoogleHome: false
enableOTA: true
telemetryInterval: 5000ms
healthCheckInterval: 30000ms
```

**Usage:**
```typescript
constructor(private env: EnvironmentService) {
  const apiUrl = this.env.apiUrl;
  const isDev = this.env.isDevelopment;
}
```

---

### 2. ApiService
**Location:** `api.service.ts`

Type-safe HTTP client for all REST API endpoints.

**Features:**
- Automatic timeout handling
- Error handling with user-friendly messages
- Authorization token injection
- Type-safe responses using models

**Available Methods:**

#### Health & System
```typescript
getHealth(): Observable<HealthStatus>
```

#### Sites
```typescript
getSites(): Observable<Site[]>
getSiteById(id: string): Observable<Site>
```

#### Coordinators
```typescript
getCoordinatorById(id: string): Observable<Coordinator>
```

#### Nodes
```typescript
getNodeById(id: string): Observable<Node>
```

#### Commands
```typescript
setLight(command: SetLightCommand): Observable<any>
sendColorProfile(command: ColorProfileCommand): Observable<any>
approveNodePairing(approval: PairingApproval): Observable<any>
```

#### OTA Updates
```typescript
startOTAUpdate(request: StartOTARequest): Observable<OTAJob>
getOTAJobStatus(jobId: string): Observable<OTAJob>
```

#### Google Home
```typescript
reportGoogleHomeState(userId: string, deviceId: string, state: GoogleHomeState): Observable<any>
requestGoogleHomeSync(userId: string): Observable<any>
```

**Example Usage:**
```typescript
constructor(private api: ApiService) {}

async loadSites() {
  this.api.getSites().subscribe({
    next: (sites) => {
      console.log('Sites loaded:', sites);
    },
    error: (err) => {
      console.error('Failed to load sites:', err);
    }
  });
}

async controlLight() {
  this.api.setLight({
    node_id: 'node-001',
    site_id: 'site-001',
    rgbw: { r: 255, g: 0, b: 0, w: 0 },
    brightness: 80,
    fade_duration: 500
  }).subscribe({
    next: () => console.log('Light updated'),
    error: (err) => console.error('Failed:', err)
  });
}
```

---

### 3. WebSocketService
**Location:** `websocket.service.ts`

Manages bidirectional real-time communication with the backend.

**Features:**
- Automatic reconnection with exponential backoff
- Connection state management (signals)
- Type-safe message routing
- Error handling

**Connection State Signals:**
```typescript
connected: Signal<boolean>
connecting: Signal<boolean>
connectionError: Signal<string | null>
```

**Message Streams:**
```typescript
messages$: Observable<WSMessage>           // All messages
telemetry$: Observable<NodeTelemetry | CoordinatorTelemetry>
presence$: Observable<PresenceEvent>
status$: Observable<StatusUpdate>
pairing$: Observable<PairingRequest>
errors$: Observable<ErrorMessage>
```

**Methods:**
```typescript
connect(): void
disconnect(): void
send(message: any): void
getConnectionState(): ConnectionState
reset(): void
```

**Example Usage:**
```typescript
constructor(private ws: WebSocketService) {}

ngOnInit() {
  // Connect to WebSocket
  this.ws.connect();

  // Subscribe to telemetry
  this.ws.telemetry$.subscribe(telemetry => {
    console.log('Telemetry received:', telemetry);
    this.updateNodeDisplay(telemetry);
  });

  // Subscribe to presence events
  this.ws.presence$.subscribe(presence => {
    console.log('Presence:', presence);
    this.updateZoneStatus(presence);
  });

  // Monitor connection
  effect(() => {
    if (this.ws.connected()) {
      console.log('WebSocket connected');
    }
  });
}

ngOnDestroy() {
  this.ws.disconnect();
}
```

---

### 4. MqttService
**Location:** `mqtt.service.ts`

Handles MQTT pub/sub communication via WebSocket bridge.

**Features:**
- Topic-based subscriptions with wildcard support
- Automatic resubscription on reconnect
- QoS level support
- Topic pattern matching (+ and # wildcards)

**Connection State Signals:**
```typescript
connected: Signal<boolean>
connecting: Signal<boolean>
connectionError: Signal<string | null>
```

**Message Streams:**
```typescript
messages$: Observable<MqttMessage>  // All MQTT messages
```

**Methods:**
```typescript
connect(): void
disconnect(): void
subscribe(topic: string): Subject<any>
unsubscribe(topic: string): void
publish(topic: string, payload: any, qos?: 0 | 1 | 2): void
```

**Helper Methods:**
```typescript
subscribeNodeTelemetry(siteId: string, nodeId: string): Subject<any>
subscribeAllNodesTelemetry(siteId: string): Subject<any>
subscribeCoordinatorTelemetry(siteId: string, coordId: string): Subject<any>
subscribeZonePresence(siteId: string, zoneId: string): Subject<any>
subscribePairingRequests(siteId: string): Subject<any>
sendNodeCommand(siteId: string, nodeId: string, command: any): void
sendZoneCommand(siteId: string, zoneId: string, command: any): void
```

**MQTT Topic Structure:**
```
Telemetry:
  site/{siteId}/coord/{coordId}/telemetry
  site/{siteId}/node/{nodeId}/telemetry
  site/{siteId}/zone/{zoneId}/presence

Commands:
  site/{siteId}/node/{nodeId}/cmd
  site/{siteId}/zone/{zoneId}/cmd

Pairing:
  site/{siteId}/node/{nodeId}/pairing
```

**Example Usage:**
```typescript
constructor(private mqtt: MqttService) {}

ngOnInit() {
  // Connect to MQTT
  this.mqtt.connect();

  // Subscribe to all nodes in a site
  this.mqtt.subscribeAllNodesTelemetry('site-001')
    .subscribe(telemetry => {
      console.log('Node telemetry:', telemetry);
    });

  // Subscribe to specific node
  this.mqtt.subscribeNodeTelemetry('site-001', 'node-001')
    .subscribe(telemetry => {
      console.log('Specific node:', telemetry);
    });

  // Subscribe to zone presence
  this.mqtt.subscribeZonePresence('site-001', 'zone-living-room')
    .subscribe(presence => {
      console.log('Presence detected:', presence);
    });
}

controlLight() {
  // Send command via MQTT
  this.mqtt.sendNodeCommand('site-001', 'node-001', {
    type: 'set_light',
    rgbw: { r: 255, g: 100, b: 0, w: 50 },
    brightness: 80,
    fade_duration: 500
  });
}

ngOnDestroy() {
  this.mqtt.disconnect();
}
```

---

### 5. DataService
**Location:** `data.service.ts`

High-level orchestration service that combines API, WebSocket, and MQTT.

**Features:**
- Unified interface for components
- Automatic state synchronization
- Caching with real-time updates
- Health monitoring
- Simplified command execution

**State Signals:**
```typescript
sites: Signal<Site[]>
nodes: Signal<Map<string, Node>>
coordinators: Signal<Map<string, Coordinator>>
activeSiteId: Signal<string | null>
latestTelemetry: Signal<Map<string, NodeTelemetry>>
presenceEvents: Signal<PresenceEvent[]>
apiHealthy: Signal<boolean>
wsConnected: Signal<boolean>
mqttConnected: Signal<boolean>
```

**Methods:**

#### Site Management
```typescript
loadSites(): Promise<void>
loadSite(siteId: string): Promise<Site>
```

#### Node Management
```typescript
loadNode(nodeId: string): Promise<Node>
setNodeLight(command: SetLightCommand): Promise<void>
approvePairing(nodeId: string, siteId: string, zoneId?: string): Promise<void>
getNode(nodeId: string): Node | undefined
getNodeTelemetry(nodeId: string): NodeTelemetry | undefined
```

#### Coordinator Management
```typescript
loadCoordinator(coordId: string): Promise<Coordinator>
getCoordinator(coordId: string): Coordinator | undefined
```

#### Health & Status
```typescript
getSystemHealth(): { api: boolean, websocket: boolean, mqtt: boolean, overall: boolean }
```

**Example Usage (Recommended for Components):**
```typescript
import { Component, inject, effect } from '@angular/core';
import { DataService } from './core/services/data.service';

@Component({
  selector: 'app-dashboard',
  template: `
    <div>
      <h1>Sites: {{ data.sites().length }}</h1>
      <div *ngFor="let site of data.sites()">
        {{ site.name }}
      </div>
      
      <h2>Connection Status</h2>
      <p>API: {{ data.apiHealthy() ? '✓' : '✗' }}</p>
      <p>WebSocket: {{ data.wsConnected() ? '✓' : '✗' }}</p>
      <p>MQTT: {{ data.mqttConnected() ? '✓' : '✗' }}</p>
    </div>
  `
})
export class DashboardComponent {
  data = inject(DataService);

  constructor() {
    // Load sites on init
    this.data.loadSites();

    // React to site changes
    effect(() => {
      const sites = this.data.sites();
      console.log('Sites updated:', sites);
    });

    // React to telemetry updates
    effect(() => {
      const telemetry = this.data.latestTelemetry();
      console.log('Telemetry updated:', telemetry.size, 'nodes');
    });
  }

  async selectSite(siteId: string) {
    await this.data.loadSite(siteId);
    console.log('Site loaded and subscribed to telemetry');
  }

  async controlNode(nodeId: string) {
    await this.data.setNodeLight({
      node_id: nodeId,
      site_id: this.data.activeSiteId()!,
      rgbw: { r: 255, g: 0, b: 0, w: 0 },
      brightness: 100
    });
  }
}
```

## Data Models

All TypeScript interfaces are defined in `models/api.models.ts`:

- **Site**: Site configuration and zones
- **Coordinator**: ESP32 coordinator device
- **Node**: ESP32 node (tile) device
- **RGBWState**: RGBW color state
- **SetLightCommand**: Light control command
- **NodeTelemetry**: Real-time node data
- **CoordinatorTelemetry**: Real-time coordinator data
- **PresenceEvent**: mmWave presence detection
- **OTAJob**: Over-the-air update job
- **HealthStatus**: Backend health check
- **WSMessage**: WebSocket message wrapper
- **GoogleHomeDevice/State**: Google Home integration

## Best Practices

### 1. Use DataService for Components
Components should primarily interact with `DataService` rather than individual services:

```typescript
// ✓ Good
constructor(private data: DataService) {}

// ✗ Avoid (unless you have specific needs)
constructor(
  private api: ApiService,
  private ws: WebSocketService,
  private mqtt: MqttService
) {}
```

### 2. Handle Subscriptions Properly
Always unsubscribe to prevent memory leaks:

```typescript
private destroy$ = new Subject<void>();

ngOnInit() {
  this.ws.telemetry$
    .pipe(takeUntil(this.destroy$))
    .subscribe(data => { ... });
}

ngOnDestroy() {
  this.destroy$.next();
  this.destroy$.complete();
}
```

### 3. Use Signals for Reactive State
Leverage Angular signals for automatic change detection:

```typescript
// In service
public readonly connected = signal<boolean>(false);

// In component
<div>Status: {{ service.connected() ? 'Online' : 'Offline' }}</div>

// Or with effect
effect(() => {
  if (this.service.connected()) {
    console.log('Connected!');
  }
});
```

### 4. Error Handling
Always handle errors gracefully:

```typescript
try {
  await this.data.loadSites();
} catch (error) {
  console.error('Failed to load sites:', error);
  // Show user-friendly error message
  this.showError('Unable to load sites. Please try again.');
}
```

### 5. Connection Management
Monitor connection states and provide feedback:

```typescript
const health = this.data.getSystemHealth();
if (!health.overall) {
  console.warn('System not fully connected:', health);
  // Show reconnection UI
}
```

## Testing

### Running Tests
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm test
```

### Example Unit Test
```typescript
describe('DataService', () => {
  let service: DataService;
  let apiMock: jasmine.SpyObj<ApiService>;

  beforeEach(() => {
    apiMock = jasmine.createSpyObj('ApiService', ['getSites']);
    TestBed.configureTestingModule({
      providers: [
        DataService,
        { provide: ApiService, useValue: apiMock }
      ]
    });
    service = TestBed.inject(DataService);
  });

  it('should load sites', async () => {
    const mockSites = [{ _id: '1', name: 'Test Site' }];
    apiMock.getSites.and.returnValue(of(mockSites));
    
    await service.loadSites();
    
    expect(service.sites().length).toBe(1);
  });
});
```

## Environment Configuration

### Development (.env.development)
```env
API_URL=http://localhost:8080/api/v1
WS_URL=ws://localhost:8080/ws
MQTT_WS_URL=ws://localhost:8080/mqtt
ENABLE_GOOGLE_HOME=false
NODE_ENV=development
```

### Production (.env.production)
```env
API_URL=https://api.yourdomain.com/api/v1
WS_URL=wss://api.yourdomain.com/ws
MQTT_WS_URL=wss://api.yourdomain.com/mqtt
ENABLE_GOOGLE_HOME=true
NODE_ENV=production
```

## Troubleshooting

### WebSocket Connection Issues
1. Check that backend is running: `docker-compose ps`
2. Verify WebSocket URL in environment config
3. Check browser console for connection errors
4. Ensure CORS is enabled on backend

### MQTT Messages Not Receiving
1. Verify MQTT service is connected: `mqtt.connected()`
2. Check topic subscription patterns
3. Verify backend MQTT broker connection
4. Check MQTT topic naming matches PRD

### API Timeout Errors
1. Increase timeout in `EnvironmentService`
2. Check backend logs: `docker-compose logs backend`
3. Verify network connectivity
4. Check API endpoint availability

## Related Documentation
- [Backend API Documentation](../../../docs/mqtt_api.md)
- [Product Requirements](../../../docs/ProductRequirementDocument.md)
- [Deployment Guide](../../../DEPLOYMENT.md)
- [Google Home Setup](../../../GOOGLE_HOME_SETUP.md)
