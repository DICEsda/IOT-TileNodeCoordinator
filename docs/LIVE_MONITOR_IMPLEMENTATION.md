# Live Monitor Implementation Guide

## Overview
This document describes the end-to-end implementation for live temperature monitoring and SK6812B light control from the frontend Live Monitor page.

## Implementation Status

### ✅ Backend (Completed)

#### 1. WebSocket Broadcasting (`internal/http/ws_broadcast.go`)
- Created `WSBroadcaster` service to manage WebSocket connections
- Broadcasts node telemetry (temperature, light state) to all connected clients
- Broadcasts coordinator telemetry (light lux, mmWave, WiFi RSSI)
- Message format:
  ```json
  {
    "type": "node_telemetry",
    "payload": {
      "nodeId": "node-abc123",
      "lightId": "light-xyz",
      "ts": 1700000000,
      "tempC": 23.5,
      "light": {
        "on": true,
        "brightness": 128,
        "avgR": 0,
        "avgG": 255,
        "avgB": 0,
        "avgW": 0
      },
      "vbatMv": 3300,
      "statusMode": "operational"
    }
  }
  ```

#### 2. MQTT Handler Integration (`internal/mqtt/handlers.go`)
- Updated `Handler` to include `WSBroadcaster`
- `handleNodeTelemetry`: Now broadcasts telemetry to WebSocket clients after saving to DB
- `handleCoordTelemetry`: Broadcasts coordinator data to WebSocket clients
- Existing MQTT subscriptions remain intact: `site/+/node/+/telemetry`, `site/+/coord/+/telemetry`

#### 3. HTTP Module (`internal/http/http.go`)
- Integrated `WSBroadcaster` into fx dependency injection
- Added `StartBroadcaster` lifecycle hook

#### 4. Light Control API (`internal/http/handlers.go`)
- New endpoint: `POST /api/v1/node/light/control`
- Request body:
  ```json
  {
    "site_id": "site001",
    "node_id": "node-abc123",
    "on": true,
    "brightness": 128
  }
  ```
- Publishes MQTT command to `site/{siteId}/node/{nodeId}/cmd`
- Command payload:
  ```json
  {
    "cmd": "set_light",
    "on": true,
    "brightness": 128,
    "r": 0,
    "g": 255,
    "b": 0,
    "w": 0
  }
  ```
- Always uses green color (0, 255, 0) when ON per requirements

### 🔧 Coordinator Firmware (To Be Implemented)

#### Required Changes in `coordinator/src/comm/Mqtt.cpp`:

1. **Subscribe to node command topic** in `connectMqtt()`:
```cpp
// After subscribing to coordinator commands:
String nodeCmdTopic = "site/" + siteId + "/node/+/cmd";
mqttClient.subscribe(nodeCmdTopic.c_str());
Logger::info("Subscribed to: %s", nodeCmdTopic.c_str());
```

2. **Handle node commands** in `processMessage()`:
```cpp
void Mqtt::processMessage(const String& topic, const String& payload) {
    // ... existing coordinator command handling ...
    
    // Handle node commands: site/{siteId}/node/{nodeId}/cmd
    if (topic.indexOf("/node/") >= 0 && topic.endsWith("/cmd")) {
        // Extract nodeId from topic
        int nodeStart = topic.indexOf("/node/") + 6;
        int nodeEnd = topic.indexOf("/cmd");
        String nodeId = topic.substring(nodeStart, nodeEnd);
        
        // Parse command
        StaticJsonDocument<512> doc;
        if (deserializeJson(doc, payload) != DeserializationError::Ok) {
            Logger::error("Failed to parse node command");
            return;
        }
        
        String cmd = doc["cmd"].as<String>();
        if (cmd == "set_light") {
            // Forward to node via ESP-NOW
            SetLightMessage lightMsg;
            lightMsg.light_id = nodeId;
            lightMsg.r = doc["r"] | 0;
            lightMsg.g = doc["g"] | 0;
            lightMsg.b = doc["b"] | 0;
            lightMsg.w = doc["w"] | 0;
            lightMsg.value = doc["brightness"] | 0;
            
            // Send via ESP-NOW to the node
            if (espNowManager) {
                espNowManager->sendToNode(nodeId, lightMsg.toJson());
            }
            
            Logger::info("Forwarded light command to node %s", nodeId.c_str());
        }
    }
}
```

3. **Add ESP-NOW manager reference** to Mqtt class:
```cpp
// In Mqtt.h:
class EspNowManager; // forward declaration

class Mqtt {
public:
    // ... existing methods ...
    void setEspNowManager(EspNowManager* manager) { espNowManager = manager; }
    
private:
    EspNowManager* espNowManager = nullptr;
    // ... existing members ...
};
```

4. **Wire up in `Coordinator::begin()`**:
```cpp
mqtt->setEspNowManager(espNow);
```

### 🔧 Node Firmware (To Be Implemented)

#### 1. Ensure TMP117 Temperature Reading

The node should already have TMP177Sensor, verify in `node/src/main.cpp`:

```cpp
#include "sensor/TMP177Sensor.h"

TMP177Sensor tempSensor;

void setup() {
    // ... existing setup ...
    
    // Initialize temperature sensor (SDA=pin3, SCL=pin2)
    if (!tempSensor.begin(3, 2)) {
        Serial.println("WARNING: TMP177 sensor not found!");
    }
}

void loop() {
    // Read temperature every second
    static uint32_t lastTempRead = 0;
    if (millis() - lastTempRead >= 1000) {
        lastTempRead = millis();
        float temp = tempSensor.readTemperature();
        
        // Include in node status message
        NodeStatusMessage status;
        status.temperature = temp;
        // ... fill other fields ...
        
        // Send to coordinator via ESP-NOW
        sendToCoordinator(status.toJson());
    }
}
```

#### 2. Handle set_light Commands

In node's ESP-NOW receive callback:

```cpp
void onDataReceived(const uint8_t* mac, const uint8_t* data, int len) {
    String json = String((char*)data);
    
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, json) != DeserializationError::Ok) {
        return;
    }
    
    String msgType = doc["msg"].as<String>();
    
    if (msgType == "set_light") {
        SetLightMessage lightMsg;
        if (lightMsg.fromJson(json)) {
            // Apply to SK6812B strip
            uint8_t r = lightMsg.r;
            uint8_t g = lightMsg.g;
            uint8_t b = lightMsg.b;
            uint8_t w = lightMsg.w;
            uint8_t brightness = lightMsg.value;
            
            // Set LED controller
            if (ledController) {
                ledController->setBrightness(brightness);
                ledController->setColor(r, g, b, w);
            }
        }
    }
}
```

### 🎨 Frontend (To Be Implemented)

#### 1. Extend WebSocket Service

The WebSocket service already exists at `src/app/core/services/websocket.service.ts`. Add type for node telemetry:

```typescript
// In api.models.ts or websocket.service.ts
export interface NodeTelemetryMessage {
  type: 'node_telemetry';
  payload: {
    nodeId: string;
    lightId: string;
    ts: number;
    tempC: number;
    light: {
      on: boolean;
      brightness: number;
      avgR: number;
      avgG: number;
      avgB: number;
      avgW: number;
    };
    vbatMv: number;
    statusMode: string;
  };
}
```

#### 2. Create Temperature Graph Component

Create `src/app/features/dashboard/components/temperature-graph/temperature-graph.component.ts`:

```typescript
import { Component, Input, OnDestroy, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Chart, ChartConfiguration } from 'chart.js/auto';

@Component({
  selector: 'app-temperature-graph',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="temperature-graph">
      <h3>Node Temperature (TMP117)</h3>
      <div class="current-temp">
        <span class="temp-value">{{ currentTemp.toFixed(1) }}°C</span>
      </div>
      <canvas #chartCanvas></canvas>
    </div>
  `,
  styleUrls: ['./temperature-graph.component.scss']
})
export class TemperatureGraphComponent implements OnInit, OnDestroy {
  @Input() nodeId!: string;
  
  currentTemp: number = 0;
  private chart?: Chart;
  private dataPoints: { x: Date; y: number }[] = [];
  private maxDataPoints = 300; // 5 minutes at 1Hz
  
  ngOnInit() {
    this.initChart();
  }
  
  ngOnDestroy() {
    this.chart?.destroy();
  }
  
  addDataPoint(timestamp: number, tempC: number) {
    this.currentTemp = tempC;
    this.dataPoints.push({ x: new Date(timestamp * 1000), y: tempC });
    
    // Keep only last N points (rolling window)
    if (this.dataPoints.length > this.maxDataPoints) {
      this.dataPoints.shift();
    }
    
    this.updateChart();
  }
  
  private initChart() {
    const config: ChartConfiguration = {
      type: 'line',
      data: {
        datasets: [{
          label: 'Temperature (°C)',
          data: this.dataPoints,
          borderColor: 'rgb(75, 192, 192)',
          tension: 0.4,
          fill: false
        }]
      },
      options: {
        responsive: true,
        scales: {
          x: {
            type: 'time',
            time: {
              unit: 'minute'
            }
          },
          y: {
            beginAtZero: false,
            title: {
              display: true,
              text: 'Temperature (°C)'
            }
          }
        },
        animation: false
      }
    };
    
    const canvas = document.querySelector('canvas');
    if (canvas) {
      this.chart = new Chart(canvas, config);
    }
  }
  
  private updateChart() {
    if (this.chart) {
      this.chart.data.datasets[0].data = this.dataPoints;
      this.chart.update('none'); // No animation for performance
    }
  }
}
```

#### 3. Create Light Control Component

Create `src/app/features/dashboard/components/light-control/light-control.component.ts`:

```typescript
import { Component, Input, inject } from '@angular/common';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { HttpClient } from '@angular/common/http';
import { EnvironmentService } from '../../../../core/services/environment.service';

@Component({
  selector: 'app-light-control',
  standalone: true,
  imports: [CommonModule, FormsModule],
  template: `
    <div class="light-control">
      <h3>Node Light (SK6812B)</h3>
      <div class="control-row">
        <label>Power:</label>
        <button 
          [class.on]="isOn" 
          (click)="toggleLight()">
          {{ isOn ? 'ON' : 'OFF' }}
        </button>
      </div>
      <div class="control-row" *ngIf="isOn">
        <label>Brightness:</label>
        <input 
          type="range" 
          min="0" 
          max="255" 
          [(ngModel)]="brightness"
          (change)="onBrightnessChange()">
        <span>{{ brightness }}</span>
      </div>
      <div class="status">
        Color: Green (fixed)
      </div>
    </div>
  `,
  styleUrls: ['./light-control.component.scss']
})
export class LightControlComponent {
  private http = inject(HttpClient);
  private env = inject(EnvironmentService);
  
  @Input() siteId!: string;
  @Input() nodeId!: string;
  @Input() isOn: boolean = false;
  @Input() brightness: number = 128;
  
  toggleLight() {
    this.isOn = !this.isOn;
    this.sendCommand();
  }
  
  onBrightnessChange() {
    if (this.isOn) {
      this.sendCommand();
    }
  }
  
  private sendCommand() {
    const url = `${this.env.apiUrl}/api/v1/node/light/control`;
    const payload = {
      site_id: this.siteId,
      node_id: this.nodeId,
      on: this.isOn,
      brightness: this.isOn ? this.brightness : 0
    };
    
    this.http.post(url, payload).subscribe({
      next: () => console.log('Light command sent'),
      error: (err) => console.error('Failed to send light command:', err)
    });
  }
  
  // Called from parent when telemetry arrives
  updateFromTelemetry(on: boolean, brightness: number) {
    this.isOn = on;
    this.brightness = brightness;
  }
}
```

#### 4. Integrate into Live Monitor Page

Update `src/app/features/dashboard/components/light-monitor/light-monitor.component.ts`:

```typescript
import { Component, OnInit, OnDestroy, ViewChild } from '@angular/core';
import { CommonModule } from '@angular/common';
import { WebSocketService } from '../../../../core/services/websocket.service';
import { TemperatureGraphComponent } from '../temperature-graph/temperature-graph.component';
import { LightControlComponent } from '../light-control/light-control.component';
import { Subscription } from 'rxjs';

@Component({
  selector: 'app-live-monitor',
  standalone: true,
  imports: [CommonModule, TemperatureGraphComponent, LightControlComponent],
  template: `
    <div class="live-monitor">
      <h2>Live Monitor</h2>
      
      <div class="monitor-grid">
        <app-temperature-graph 
          [nodeId]="selectedNodeId"
          #tempGraph>
        </app-temperature-graph>
        
        <app-light-control
          [siteId]="siteId"
          [nodeId]="selectedNodeId"
          #lightControl>
        </app-light-control>
      </div>
    </div>
  `
})
export class LiveMonitorComponent implements OnInit, OnDestroy {
  @ViewChild('tempGraph') tempGraph?: TemperatureGraphComponent;
  @ViewChild('lightControl') lightControl?: LightControlComponent;
  
  siteId = 'site001'; // TODO: Get from app state
  selectedNodeId = 'node-default'; // TODO: Get from app state
  
  private subscription?: Subscription;
  
  constructor(private wsService: WebSocketService) {}
  
  ngOnInit() {
    // Connect to WebSocket
    this.wsService.connect();
    
    // Subscribe to node telemetry messages
    this.subscription = this.wsService.messages$.subscribe(msg => {
      if (msg.type === 'node_telemetry') {
        const payload = (msg as any).payload;
        
        // Update temperature graph
        if (this.tempGraph && payload.nodeId === this.selectedNodeId) {
          this.tempGraph.addDataPoint(payload.ts, payload.tempC);
        }
        
        // Update light control state
        if (this.lightControl && payload.nodeId === this.selectedNodeId) {
          this.lightControl.updateFromTelemetry(
            payload.light.on,
            payload.light.brightness
          );
        }
      }
    });
  }
  
  ngOnDestroy() {
    this.subscription?.unsubscribe();
    this.wsService.disconnect();
  }
}
```

### Testing Instructions

#### 1. Backend Testing
```bash
cd IOT-Backend-main/IOT-Backend-main
go test ./internal/http
go test ./internal/mqtt
go build ./cmd/iot
```

#### 2. Firmware Testing
```bash
# Coordinator
cd coordinator
pio run -e esp32-s3-devkitc-1 -t upload -t monitor

# Node
cd node
pio run -e esp32-c3-mini-1 -t upload -t monitor
```

#### 3. Frontend Testing
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm install chart.js
npm start
```

Navigate to the Live Monitor page and verify:
- Temperature updates approximately every second
- Graph shows rolling window of last 5 minutes
- Toggle switch turns LED green when ON
- Brightness slider controls LED brightness
- Color remains green (RGB: 0, 255, 0)

### Verification Checklist

- [ ] Backend builds without errors
- [ ] WebSocket connections established
- [ ] Node telemetry reaches backend via MQTT
- [ ] Temperature displayed on frontend updates live
- [ ] Graph shows temperature history
- [ ] ON/OFF toggle sends command to backend
- [ ] Backend publishes to `site/{siteId}/node/{nodeId}/cmd`
- [ ] Coordinator receives and forwards command via ESP-NOW
- [ ] Node receives command and controls SK6812B
- [ ] LED turns green when ON
- [ ] Brightness slider adjusts LED brightness
- [ ] Telemetry confirms coordinator does NOT have temp sensor (only node temp shown)

### Known Issues & Next Steps

1. **Chart.js Dependency**: Add to package.json:
   ```json
   "dependencies": {
     "chart.js": "^4.4.0"
   }
   ```

2. **Node Selection**: Currently hardcoded, should be selectable from registered nodes

3. **Multi-node Support**: Extend to show multiple nodes simultaneously

4. **Historical Data**: Add API endpoint to fetch temperature history from DB

5. **Error Handling**: Add retry logic for failed commands

6. **Firmware OTA**: Ensure new firmware can be deployed via existing OTA mechanism

### Architecture Summary

```
┌─────────────┐     WebSocket      ┌──────────┐
│   Frontend  │◄──────────────────►│ Backend  │
│ (Angular)   │                    │  (Go)    │
└─────────────┘                    └────┬─────┘
                                        │ MQTT
                                        ▼
                                   ┌──────────┐
                                   │  MQTT    │
                                   │  Broker  │
                                   └────┬─────┘
                                        │
                          ┌─────────────┴─────────────┐
                          │                           │
                    ┌─────▼──────┐            ┌──────▼─────┐
                    │Coordinator │ ESP-NOW    │    Node    │
                    │  (ESP32-S3)│◄──────────►│ (ESP32-C3) │
                    └────────────┘            └──────┬─────┘
                                                     │
                                              ┌──────▼──────┐
                                              │  TMP117     │
                                              │  SK6812B    │
                                              └─────────────┘
```

### Message Flow

1. **Telemetry**: Node → Coordinator → MQTT → Backend → WebSocket → Frontend
2. **Commands**: Frontend → Backend HTTP → MQTT → Coordinator → ESP-NOW → Node → LED

