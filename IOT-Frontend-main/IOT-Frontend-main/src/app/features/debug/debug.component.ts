import { Component, OnInit, OnDestroy, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { interval, Subject } from 'rxjs';
import { takeUntil, firstValueFrom } from 'rxjs/operators';
import { EnvironmentService } from '../../core/services/environment.service';
import { ApiService } from '../../core/services/api.service';
import { WebSocketService } from '../../core/services/websocket.service';
import { MqttService } from '../../core/services/mqtt.service';

interface ConnectionStatus {
  service: string;
  connected: boolean;
  status: 'connected' | 'disconnected' | 'error';
  lastUpdate: Date;
  details?: string;
  latency?: number;
}

interface TelemetryMessage {
  timestamp: Date;
  topic: string;
  source: 'MQTT' | 'WebSocket' | 'HTTP';
  payload: any;
}

@Component({
  selector: 'app-debug',
  standalone: true,
  imports: [CommonModule, FormsModule],
  template: `
    <div class="debug-container">
      <div class="debug-header">
        <h1>🔧 System Debug Dashboard</h1>
        <div class="header-actions">
          <button (click)="refreshAll()" class="btn btn-primary">
            🔄 Refresh All
          </button>
          <button (click)="clearTelemetry()" class="btn btn-secondary">
            🗑️ Clear Logs
          </button>
        </div>
      </div>

      <!-- Environment Configuration -->
      <div class="debug-section">
        <h2>⚙️ Environment Configuration</h2>
        <div class="config-grid">
          <div class="config-item">
            <label>API URL:</label>
            <span class="value">{{ environment.apiUrl }}</span>
          </div>
          <div class="config-item">
            <label>WebSocket URL:</label>
            <span class="value">{{ environment.wsUrl }}</span>
          </div>
          <div class="config-item">
            <label>MQTT WebSocket:</label>
            <span class="value">{{ environment.mqttWsUrl || 'Not configured' }}</span>
          </div>
        </div>
      </div>

      <!-- Connection Status -->
      <div class="debug-section">
        <h2>🔌 Connection Status</h2>
        <div class="status-grid">
          @for (status of connectionStatuses(); track status.service) {
            <div class="status-card" [attr.data-status]="status.status">
              <div class="status-header">
                <span class="status-icon">
                  {{ status.status === 'connected' ? '✅' : 
                     status.status === 'disconnected' ? '❌' : '⚠️' }}
                </span>
                <h3>{{ status.service }}</h3>
              </div>
              <div class="status-body">
                <div class="status-row">
                  <span>Status:</span>
                  <span class="status-badge" [attr.data-status]="status.status">
                    {{ status.status | uppercase }}
                  </span>
                </div>
                @if (status.latency !== undefined) {
                  <div class="status-row">
                    <span>Latency:</span>
                    <span>{{ status.latency }}ms</span>
                  </div>
                }
                <div class="status-row">
                  <span>Last Update:</span>
                  <span>{{ formatTime(status.lastUpdate) }}</span>
                </div>
                @if (status.details) {
                  <div class="status-details">{{ status.details }}</div>
                }
              </div>
            </div>
          }
        </div>
      </div>

      <!-- API Health Check -->
      <div class="debug-section">
        <h2>🏥 Backend Health</h2>
        <div class="health-container">
          @if (healthStatus(); as health) {
            <div class="health-card" [attr.data-healthy]="health.status === 'ok'">
              <div class="health-row">
                <span>Status:</span>
                <span class="health-badge" [attr.data-status]="health.status">
                  {{ health.status === 'ok' ? '✅ Healthy' : '❌ Unhealthy' }}
                </span>
              </div>
              <div class="health-row">
                <span>Uptime:</span>
                <span>{{ health.uptime || 'N/A' }}</span>
              </div>
            </div>
          } @else {
            <div class="health-card" data-healthy="false">
              <p>❌ Unable to fetch health status</p>
              <button (click)="checkHealth()" class="btn btn-small">Retry</button>
            </div>
          }
        </div>
      </div>

      <!-- Live Telemetry Monitor -->
      <div class="debug-section">
        <h2>📡 Live Telemetry Monitor</h2>
        <div class="telemetry-controls">
          <button (click)="startMonitoring()" 
                  class="btn btn-primary"
                  [disabled]="monitoring()">
            {{ monitoring() ? '⏸️ Monitoring...' : '▶️ Start Monitor' }}
          </button>
          <button (click)="stopMonitoring()" 
                  class="btn btn-secondary"
                  [disabled]="!monitoring()">
            Stop
          </button>
          <button (click)="clearTelemetry()" class="btn btn-small">Clear</button>
        </div>
        <div class="telemetry-log">
          @if (telemetryMessages().length === 0) {
            <div class="empty-state">
              <p>📭 No telemetry messages yet</p>
              <p class="hint">Click "Start Monitor" to see live data</p>
            </div>
          } @else {
            @for (msg of telemetryMessages(); track msg.timestamp) {
              <div class="telemetry-message" [attr.data-source]="msg.source">
                <div class="msg-header">
                  <span class="msg-time">{{ formatTime(msg.timestamp) }}</span>
                  <span class="msg-source">{{ msg.source }}</span>
                  <span class="msg-topic">{{ msg.topic }}</span>
                </div>
                <pre class="msg-payload">{{ formatJson(msg.payload) }}</pre>
              </div>
            }
          }
        </div>
      </div>

      <!-- MQTT Topics Test -->
      <div class="debug-section">
        <h2>🧪 MQTT Topic Test</h2>
        <div class="mqtt-test">
          <div class="test-row">
            <label>Topic:</label>
            <input type="text" 
                   [(ngModel)]="testTopic" 
                   placeholder="site/site001/coord/+/telemetry">
          </div>
          <div class="test-actions">
            <button (click)="subscribeTestTopic()" class="btn btn-primary">
              Subscribe
            </button>
            <button (click)="unsubscribeTestTopic()" class="btn btn-secondary">
              Unsubscribe
            </button>
          </div>
          <div class="test-log">
            @for (log of mqttTestLog(); track $index) {
              <div class="log-entry">{{ log }}</div>
            }
          </div>
        </div>
      </div>
    </div>
  `,
  styles: [`
    .debug-container {
      padding: 2rem;
      background: #f5f5f5;
      min-height: 100vh;
    }

    .debug-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 2rem;
      padding: 1.5rem;
      background: white;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }

    .debug-header h1 {
      margin: 0;
      font-size: 1.8rem;
    }

    .header-actions {
      display: flex;
      gap: 0.5rem;
    }

    .debug-section {
      background: white;
      border-radius: 8px;
      padding: 1.5rem;
      margin-bottom: 1.5rem;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }

    .debug-section h2 {
      margin: 0 0 1rem 0;
      font-size: 1.3rem;
      color: #333;
    }

    .config-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 1rem;
    }

    .config-item {
      display: flex;
      justify-content: space-between;
      padding: 0.75rem;
      background: #f9f9f9;
      border-radius: 4px;
    }

    .config-item label {
      font-weight: 600;
      color: #666;
    }

    .config-item .value {
      color: #0066cc;
      font-family: monospace;
      word-break: break-all;
    }

    .status-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
      gap: 1rem;
    }

    .status-card {
      border: 2px solid #ddd;
      border-radius: 8px;
      padding: 1rem;
    }

    .status-card[data-status="connected"] {
      border-color: #4caf50;
      background: #f1f8f4;
    }

    .status-card[data-status="disconnected"] {
      border-color: #f44336;
      background: #fef1f0;
    }

    .status-card[data-status="error"] {
      border-color: #ff9800;
      background: #fff8f0;
    }

    .status-header {
      display: flex;
      align-items: center;
      gap: 0.5rem;
      margin-bottom: 0.75rem;
    }

    .status-icon {
      font-size: 1.5rem;
    }

    .status-header h3 {
      margin: 0;
      font-size: 1.1rem;
    }

    .status-row {
      display: flex;
      justify-content: space-between;
      margin-bottom: 0.5rem;
      font-size: 0.9rem;
    }

    .status-badge {
      padding: 0.2rem 0.5rem;
      border-radius: 4px;
      font-size: 0.8rem;
      font-weight: 600;
    }

    .status-badge[data-status="connected"] {
      background: #4caf50;
      color: white;
    }

    .status-badge[data-status="disconnected"] {
      background: #f44336;
      color: white;
    }

    .status-badge[data-status="error"] {
      background: #ff9800;
      color: white;
    }

    .status-details {
      margin-top: 0.5rem;
      padding: 0.5rem;
      background: rgba(0,0,0,0.05);
      border-radius: 4px;
      font-size: 0.85rem;
      color: #666;
      word-break: break-all;
    }

    .health-container {
      max-width: 600px;
    }

    .health-card {
      padding: 1rem;
      border-radius: 8px;
      border: 2px solid #ddd;
    }

    .health-card[data-healthy="true"] {
      border-color: #4caf50;
      background: #f1f8f4;
    }

    .health-card[data-healthy="false"] {
      border-color: #f44336;
      background: #fef1f0;
    }

    .health-row {
      display: flex;
      justify-content: space-between;
      margin-bottom: 0.5rem;
    }

    .health-badge[data-status="ok"] {
      color: #4caf50;
      font-weight: 600;
    }

    .telemetry-controls {
      display: flex;
      gap: 0.5rem;
      margin-bottom: 1rem;
    }

    .telemetry-log {
      max-height: 400px;
      overflow-y: auto;
      border: 1px solid #ddd;
      border-radius: 4px;
      padding: 0.5rem;
      background: #fafafa;
    }

    .telemetry-message {
      margin-bottom: 0.75rem;
      padding: 0.75rem;
      background: white;
      border-left: 4px solid #ddd;
      border-radius: 4px;
    }

    .telemetry-message[data-source="MQTT"] {
      border-left-color: #2196f3;
    }

    .telemetry-message[data-source="WebSocket"] {
      border-left-color: #4caf50;
    }

    .msg-header {
      display: flex;
      gap: 1rem;
      margin-bottom: 0.5rem;
      font-size: 0.85rem;
      flex-wrap: wrap;
    }

    .msg-time {
      color: #666;
    }

    .msg-source {
      font-weight: 600;
      color: #2196f3;
    }

    .msg-topic {
      color: #666;
      font-family: monospace;
      word-break: break-all;
    }

    .msg-payload {
      margin: 0;
      padding: 0.5rem;
      background: #f5f5f5;
      border-radius: 4px;
      font-size: 0.85rem;
      overflow-x: auto;
    }

    .mqtt-test {
      max-width: 800px;
    }

    .test-row {
      display: flex;
      gap: 1rem;
      align-items: center;
      margin-bottom: 1rem;
    }

    .test-row label {
      min-width: 60px;
      font-weight: 600;
    }

    .test-row input {
      flex: 1;
      padding: 0.5rem;
      border: 1px solid #ddd;
      border-radius: 4px;
    }

    .test-actions {
      display: flex;
      gap: 0.5rem;
      margin-bottom: 1rem;
    }

    .test-log {
      max-height: 200px;
      overflow-y: auto;
      border: 1px solid #ddd;
      border-radius: 4px;
      padding: 0.5rem;
      background: #fafafa;
      font-family: monospace;
      font-size: 0.85rem;
    }

    .log-entry {
      padding: 0.25rem;
      border-bottom: 1px solid #eee;
    }

    .btn {
      padding: 0.5rem 1rem;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-weight: 500;
      transition: all 0.3s;
    }

    .btn:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }

    .btn-primary {
      background: #2196f3;
      color: white;
    }

    .btn-primary:hover:not(:disabled) {
      background: #1976d2;
    }

    .btn-secondary {
      background: #757575;
      color: white;
    }

    .btn-secondary:hover:not(:disabled) {
      background: #616161;
    }

    .btn-small {
      padding: 0.3rem 0.75rem;
      font-size: 0.85rem;
    }

    .empty-state {
      text-align: center;
      padding: 2rem;
      color: #999;
    }

    .empty-state .hint {
      font-size: 0.9rem;
      margin-top: 0.5rem;
    }
  `]
})
export class DebugComponent implements OnInit, OnDestroy {
  private destroy$ = new Subject<void>();
  
  environment: any;
  
  connectionStatuses = signal<ConnectionStatus[]>([]);
  healthStatus = signal<any>(null);
  telemetryMessages = signal<TelemetryMessage[]>([]);
  mqttTestLog = signal<string[]>([]);
  monitoring = signal(false);
  
  testTopic = 'site/site001/coord/+/telemetry';

  constructor(
    private environmentService: EnvironmentService,
    private apiService: ApiService,
    private wsService: WebSocketService,
    private mqttService: MqttService
  ) {
    this.environment = {
      apiUrl: this.environmentService.apiUrl,
      wsUrl: this.environmentService.wsUrl,
      mqttWsUrl: this.environmentService.mqttWsUrl
    };
  }

  ngOnInit() {
    this.checkAllConnections();
    this.checkHealth();
    this.setupAutoRefresh();
  }

  ngOnDestroy() {
    this.destroy$.next();
    this.destroy$.complete();
  }

  private setupAutoRefresh() {
    interval(5000)
      .pipe(takeUntil(this.destroy$))
      .subscribe(() => {
        this.checkAllConnections();
      });
  }

  async checkAllConnections() {
    const statuses: ConnectionStatus[] = [];
    
    // Check HTTP API
    try {
      const start = Date.now();
      await firstValueFrom(this.apiService.getSites());
      const latency = Date.now() - start;
      statuses.push({
        service: 'HTTP API',
        connected: true,
        status: 'connected',
        lastUpdate: new Date(),
        latency,
        details: this.environment.apiUrl
      });
    } catch (error: any) {
      statuses.push({
        service: 'HTTP API',
        connected: false,
        status: 'error',
        lastUpdate: new Date(),
        details: error.message || 'Connection failed'
      });
    }

    // Check WebSocket
    const wsConnected = this.wsService.connected();
    statuses.push({
      service: 'WebSocket',
      connected: wsConnected,
      status: wsConnected ? 'connected' : 'disconnected',
      lastUpdate: new Date(),
      details: this.environment.wsUrl
    });

    // Check MQTT
    const mqttConnected = this.mqttService.connected();
    statuses.push({
      service: 'MQTT',
      connected: mqttConnected,
      status: mqttConnected ? 'connected' : 'disconnected',
      lastUpdate: new Date(),
      details: this.environment.mqttWsUrl
    });

    this.connectionStatuses.set(statuses);
  }

  async checkHealth() {
    try {
      const health = await firstValueFrom(this.apiService.getHealth());
      this.healthStatus.set(health);
    } catch (error) {
      this.healthStatus.set(null);
    }
  }

  startMonitoring() {
    this.monitoring.set(true);
    
    // Monitor WebSocket messages
    this.wsService.messages$
      .pipe(takeUntil(this.destroy$))
      .subscribe((msg: any) => {
        if (this.monitoring()) {
          this.addTelemetryMessage('WebSocket', msg.type || 'event', msg);
        }
      });
    
    // Subscribe to coordinator telemetry
    const siteId = 'site001';
    this.mqttService.subscribeCoordinatorTelemetry(siteId, '+')
      .pipe(takeUntil(this.destroy$))
      .subscribe((data: any) => {
        if (this.monitoring()) {
          this.addTelemetryMessage('MQTT', `site/${siteId}/coord/+/telemetry`, data);
        }
      });
    
    // Subscribe to node telemetry
    this.mqttService.subscribeNodeTelemetry(siteId, '+')
      .pipe(takeUntil(this.destroy$))
      .subscribe((data: any) => {
        if (this.monitoring()) {
          this.addTelemetryMessage('MQTT', `site/${siteId}/node/+/telemetry`, data);
        }
      });
  }

  stopMonitoring() {
    this.monitoring.set(false);
  }

  subscribeTestTopic() {
    if (!this.testTopic) return;
    
    this.mqttTestLog.update((log: string[]) => [...log, `Subscribing to: ${this.testTopic}`]);
    
    this.mqttService.subscribe(this.testTopic)
      .pipe(takeUntil(this.destroy$))
      .subscribe({
        next: (data: any) => {
          this.mqttTestLog.update((log: string[]) => [
            ...log,
            `✅ Received: ${JSON.stringify(data).substring(0, 100)}...`
          ]);
        },
        error: (error: any) => {
          this.mqttTestLog.update((log: string[]) => [
            ...log,
            `❌ Error: ${error.message}`
          ]);
        }
      });
  }

  unsubscribeTestTopic() {
    if (!this.testTopic) return;
    this.mqttService.unsubscribe(this.testTopic);
    this.mqttTestLog.update((log: string[]) => [...log, `Unsubscribed from: ${this.testTopic}`]);
  }

  refreshAll() {
    this.checkAllConnections();
    this.checkHealth();
  }

  clearTelemetry() {
    this.telemetryMessages.set([]);
  }

  private addTelemetryMessage(source: 'MQTT' | 'WebSocket' | 'HTTP', topic: string, payload: any) {
    this.telemetryMessages.update((messages: TelemetryMessage[]) => {
      const newMessages = [{
        timestamp: new Date(),
        topic,
        source,
        payload
      }, ...messages];
      
      // Keep only last 50 messages
      return newMessages.slice(0, 50);
    });
  }

  formatTime(date: Date): string {
    return date.toLocaleTimeString();
  }

  formatJson(obj: any): string {
    try {
      return JSON.stringify(obj, null, 2);
    } catch {
      return String(obj);
    }
  }
}
