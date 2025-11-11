# Frontend Services Quick Reference

## 📦 Installation

```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm install
```

## 🚀 Quick Start

### 1. Basic Usage (Recommended)

```typescript
import { Component, inject } from '@angular/core';
import { DataService } from './core/services';

@Component({
  selector: 'app-my-component',
  template: `
    <div>
      <p>Sites: {{ data.sites().length }}</p>
      <p>Connected: {{ data.wsConnected() ? '✓' : '✗' }}</p>
    </div>
  `
})
export class MyComponent {
  data = inject(DataService);

  ngOnInit() {
    this.data.loadSites();
  }
}
```

### 2. Control Lights

```typescript
async controlLight(nodeId: string) {
  await this.data.setNodeLight({
    node_id: nodeId,
    site_id: 'site-001',
    rgbw: { r: 255, g: 0, b: 0, w: 0 },
    brightness: 80,
    fade_duration: 500
  });
}
```

### 3. Subscribe to Telemetry

```typescript
import { effect } from '@angular/core';

constructor() {
  // React to telemetry updates
  effect(() => {
    const telemetry = this.data.latestTelemetry();
    console.log('Telemetry updated:', telemetry.size);
  });
}
```

## 📚 Services Overview

| Service | Purpose | When to Use |
|---------|---------|-------------|
| **DataService** | High-level orchestration | Most components (recommended) |
| **ApiService** | HTTP REST calls | Custom API interactions |
| **WebSocketService** | Real-time events | Custom WS message handling |
| **MqttService** | MQTT pub/sub | Custom MQTT subscriptions |
| **EnvironmentService** | Configuration | Access config values |

## 🔌 API Methods

### Sites
```typescript
await data.loadSites()
await data.loadSite('site-001')
```

### Nodes
```typescript
await data.loadNode('node-001')
await data.setNodeLight(command)
await data.approvePairing('node-001', 'site-001')
```

### Coordinators
```typescript
await data.loadCoordinator('coord-001')
```

### Health
```typescript
const health = data.getSystemHealth()
// { api: true, websocket: true, mqtt: true, overall: true }
```

## 📡 MQTT Topics

### Subscribe
```typescript
// Single node
mqtt.subscribeNodeTelemetry('site-001', 'node-001')

// All nodes
mqtt.subscribeAllNodesTelemetry('site-001')

// Coordinator
mqtt.subscribeCoordinatorTelemetry('site-001', 'coord-001')

// Zone presence
mqtt.subscribeZonePresence('site-001', 'zone-living-room')

// Pairing requests
mqtt.subscribePairingRequests('site-001')
```

### Publish
```typescript
// Node command
mqtt.sendNodeCommand('site-001', 'node-001', {
  type: 'set_light',
  rgbw: { r: 255, g: 0, b: 0, w: 0 }
})

// Zone command
mqtt.sendZoneCommand('site-001', 'zone-living-room', {
  type: 'enable'
})
```

## 🎨 Color Examples

```typescript
// Red
{ r: 255, g: 0, b: 0, w: 0 }

// Green
{ r: 0, g: 255, b: 0, w: 0 }

// Blue
{ r: 0, g: 0, b: 255, w: 0 }

// White (warm)
{ r: 0, g: 0, b: 0, w: 255 }

// Orange
{ r: 255, g: 165, b: 0, w: 0 }

// Purple
{ r: 128, g: 0, b: 128, w: 0 }
```

## 🔄 Real-time Updates

```typescript
import { effect } from '@angular/core';
import { DataService } from './core/services';

export class Component {
  data = inject(DataService);

  constructor() {
    // Auto-update when sites change
    effect(() => {
      const sites = this.data.sites();
      console.log('Sites:', sites);
    });

    // Auto-update when nodes change
    effect(() => {
      const nodes = this.data.nodes();
      console.log('Nodes:', nodes.size);
    });

    // Auto-update when telemetry arrives
    effect(() => {
      const telemetry = this.data.latestTelemetry();
      this.updateUI(telemetry);
    });
  }
}
```

## 🛡️ Error Handling

```typescript
try {
  await this.data.loadSites();
} catch (error) {
  console.error('Failed:', error);
  this.showError('Unable to load sites');
}
```

## 🔧 Configuration

### Environment Variables (.env)
```env
API_URL=http://localhost:8080/api/v1
WS_URL=ws://localhost:8080/ws
MQTT_WS_URL=ws://localhost:8080/mqtt
ENABLE_GOOGLE_HOME=false
TELEMETRY_INTERVAL=5000
```

### Access Config
```typescript
constructor(private env: EnvironmentService) {
  console.log('API URL:', this.env.apiUrl);
  console.log('Is Dev:', this.env.isDevelopment);
}
```

## 📊 Connection Monitoring

```typescript
// Check individual connections
if (data.apiHealthy()) {
  console.log('API connected');
}

if (data.wsConnected()) {
  console.log('WebSocket connected');
}

if (data.mqttConnected()) {
  console.log('MQTT connected');
}

// Check overall health
const health = data.getSystemHealth();
if (health.overall) {
  console.log('All systems operational');
}
```

## 🧹 Cleanup

```typescript
import { OnDestroy } from '@angular/core';
import { Subject, takeUntil } from 'rxjs';

export class Component implements OnDestroy {
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
}
```

## 📖 Documentation

- **Full docs**: `src/app/core/services/README.md`
- **Example**: `src/app/features/example-dashboard/`
- **Models**: `src/app/core/models/api.models.ts`

## 🐛 Troubleshooting

### Services not working
```bash
# Install dependencies
npm install
```

### WebSocket not connecting
Check backend is running:
```bash
docker-compose ps
curl http://localhost:8080/health
```

### MQTT not receiving
Verify MQTT broker:
```bash
docker-compose logs mosquitto
```

## ✅ Checklist

- [ ] Run `npm install`
- [ ] Configure `.env` file
- [ ] Start backend: `docker-compose up`
- [ ] Inject `DataService` in components
- [ ] Call `loadSites()` on init
- [ ] Subscribe to real-time updates with `effect()`
- [ ] Handle errors with try/catch
- [ ] Clean up with `ngOnDestroy()`

## 🎯 Common Patterns

### Load and Display Sites
```typescript
data = inject(DataService);
sites = this.data.sites;

ngOnInit() {
  this.data.loadSites();
}
```

### Control Multiple Nodes
```typescript
async setAllNodesColor(color: RGBWState) {
  const nodes = Array.from(this.data.nodes().values());
  const siteId = this.data.activeSiteId();
  
  for (const node of nodes) {
    await this.data.setNodeLight({
      node_id: node.node_id,
      site_id: siteId!,
      rgbw: color,
      brightness: 80
    });
  }
}
```

### Monitor Node Status
```typescript
getNodeStatus(nodeId: string): string {
  const node = this.data.getNode(nodeId);
  return node?.status || 'unknown';
}

isNodeOnline(nodeId: string): boolean {
  return this.getNodeStatus(nodeId) === 'online';
}
```

### Display Real-time Temperature
```typescript
getTemperature(nodeId: string): number | null {
  const telemetry = this.data.getNodeTelemetry(nodeId);
  return telemetry?.temperature ?? null;
}
```

## 🚀 Ready to Code!

Import services and start building:

```typescript
import { DataService } from './core/services';
import { Node, SetLightCommand } from './core/models';

// That's it! Start coding...
```

---

**Need help?** See `src/app/core/services/README.md` for comprehensive documentation.
