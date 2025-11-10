/**
 * Example Dashboard Component
 * Demonstrates best practices for using the core services
 * 
 * This example shows:
 * - How to inject and use DataService
 * - Reactive state with signals
 * - Real-time telemetry updates
 * - Connection state monitoring
 * - Error handling
 * - Proper subscription management
 */

import { Component, inject, effect, signal, OnDestroy } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Subject, takeUntil } from 'rxjs';
import { DataService } from '../../core/services/data.service';
import { Site, Node, NodeTelemetry, SetLightCommand } from '../../core/models/api.models';

@Component({
  selector: 'app-example-dashboard',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="dashboard">
      <!-- Connection Status -->
      <div class="status-bar" [class.healthy]="isHealthy()">
        <h3>System Status</h3>
        <div class="connections">
          <span [class.connected]="data.apiHealthy()">
            API: {{ data.apiHealthy() ? '✓' : '✗' }}
          </span>
          <span [class.connected]="data.wsConnected()">
            WebSocket: {{ data.wsConnected() ? '✓' : '✗' }}
          </span>
          <span [class.connected]="data.mqttConnected()">
            MQTT: {{ data.mqttConnected() ? '✓' : '✗' }}
          </span>
        </div>
      </div>

      <!-- Loading State -->
      <div *ngIf="loading()" class="loading">
        <p>Loading sites...</p>
      </div>

      <!-- Error State -->
      <div *ngIf="error()" class="error">
        <p>{{ error() }}</p>
        <button (click)="retry()">Retry</button>
      </div>

      <!-- Sites List -->
      <div *ngIf="!loading() && !error()" class="sites">
        <h2>Sites ({{ data.sites().length }})</h2>
        <div class="site-list">
          <div 
            *ngFor="let site of data.sites()" 
            class="site-card"
            [class.active]="site._id === data.activeSiteId()"
            (click)="selectSite(site._id)"
          >
            <h3>{{ site.name }}</h3>
            <p>{{ site.location || 'No location' }}</p>
            <p>{{ site.coordinators.length }} coordinator(s)</p>
            <p>{{ site.zones.length }} zone(s)</p>
          </div>
        </div>
      </div>

      <!-- Active Site Details -->
      <div *ngIf="data.activeSiteId()" class="active-site">
        <h2>Active Site: {{ getActiveSiteName() }}</h2>
        
        <!-- Nodes -->
        <div class="nodes-section">
          <h3>Nodes ({{ getNodesList().length }})</h3>
          <div class="node-list">
            <div 
              *ngFor="let node of getNodesList()" 
              class="node-card"
              [class.online]="node.status === 'online'"
              [class.offline]="node.status === 'offline'"
            >
              <h4>{{ node.node_id }}</h4>
              <p>Status: {{ node.status }}</p>
              
              <!-- Show telemetry if available -->
              <div *ngIf="getNodeTelemetry(node.node_id) as telemetry" class="telemetry">
                <p>Temperature: {{ telemetry.temperature?.toFixed(1) }}°C</p>
                <p>Battery: {{ telemetry.battery_percent }}%</p>
                <div class="color-preview" [style.background-color]="getRgbColor(telemetry.rgbw)"></div>
              </div>

              <!-- Controls -->
              <div class="controls">
                <button (click)="setNodeColor(node.node_id, 'red')">Red</button>
                <button (click)="setNodeColor(node.node_id, 'green')">Green</button>
                <button (click)="setNodeColor(node.node_id, 'blue')">Blue</button>
                <button (click)="setNodeColor(node.node_id, 'white')">White</button>
                <button (click)="turnOff(node.node_id)">Off</button>
              </div>
            </div>
          </div>
        </div>

        <!-- Real-time Updates -->
        <div class="realtime-section">
          <h3>Real-time Telemetry</h3>
          <p>Updates received: {{ telemetryCount() }}</p>
          <p>Last update: {{ lastUpdate() }}</p>
        </div>
      </div>
    </div>
  `,
  styles: [`
    .dashboard {
      padding: 20px;
      font-family: Arial, sans-serif;
    }

    .status-bar {
      padding: 15px;
      background: #f44336;
      color: white;
      border-radius: 8px;
      margin-bottom: 20px;
    }

    .status-bar.healthy {
      background: #4caf50;
    }

    .connections {
      display: flex;
      gap: 20px;
      margin-top: 10px;
    }

    .connections span {
      opacity: 0.6;
    }

    .connections span.connected {
      opacity: 1;
      font-weight: bold;
    }

    .loading, .error {
      text-align: center;
      padding: 40px;
    }

    .error {
      color: #f44336;
    }

    .site-list, .node-list {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
      gap: 15px;
      margin-top: 15px;
    }

    .site-card, .node-card {
      padding: 15px;
      border: 2px solid #e0e0e0;
      border-radius: 8px;
      cursor: pointer;
      transition: all 0.2s;
    }

    .site-card:hover, .node-card:hover {
      border-color: #2196f3;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    }

    .site-card.active {
      border-color: #4caf50;
      background: #f1f8f4;
    }

    .node-card.online {
      border-left: 4px solid #4caf50;
    }

    .node-card.offline {
      border-left: 4px solid #9e9e9e;
      opacity: 0.6;
    }

    .controls {
      display: flex;
      gap: 5px;
      margin-top: 10px;
      flex-wrap: wrap;
    }

    .controls button {
      padding: 5px 10px;
      border: none;
      border-radius: 4px;
      background: #2196f3;
      color: white;
      cursor: pointer;
      font-size: 12px;
    }

    .controls button:hover {
      background: #1976d2;
    }

    .color-preview {
      width: 100%;
      height: 30px;
      border-radius: 4px;
      margin-top: 10px;
    }

    .telemetry p {
      margin: 5px 0;
      font-size: 13px;
    }

    .realtime-section {
      margin-top: 30px;
      padding: 20px;
      background: #f5f5f5;
      border-radius: 8px;
    }
  `]
})
export class ExampleDashboardComponent implements OnDestroy {
  // Inject DataService using Angular 19 inject function
  data = inject(DataService);
  
  // Component state using signals
  loading = signal<boolean>(false);
  error = signal<string | null>(null);
  telemetryCount = signal<number>(0);
  lastUpdate = signal<string>('Never');
  
  // Cleanup subject
  private destroy$ = new Subject<void>();

  constructor() {
    // Load initial data
    this.loadInitialData();

    // Monitor telemetry updates using effect
    effect(() => {
      const telemetry = this.data.latestTelemetry();
      this.telemetryCount.set(telemetry.size);
      if (telemetry.size > 0) {
        this.lastUpdate.set(new Date().toLocaleTimeString());
      }
    });

    // Monitor connection changes
    effect(() => {
      const health = this.data.getSystemHealth();
      if (!health.overall) {
        console.warn('System not fully connected:', health);
      }
    });
  }

  ngOnDestroy(): void {
    this.destroy$.next();
    this.destroy$.complete();
  }

  /**
   * Load initial site data
   */
  private async loadInitialData(): Promise<void> {
    this.loading.set(true);
    this.error.set(null);

    try {
      await this.data.loadSites();
      
      // Auto-select first site if available
      const sites = this.data.sites();
      if (sites.length > 0 && !this.data.activeSiteId()) {
        await this.selectSite(sites[0]._id);
      }
    } catch (err: any) {
      this.error.set(err.message || 'Failed to load sites');
      console.error('Failed to load initial data:', err);
    } finally {
      this.loading.set(false);
    }
  }

  /**
   * Select and load a site
   */
  async selectSite(siteId: string): Promise<void> {
    try {
      await this.data.loadSite(siteId);
      console.log('Site loaded:', siteId);
    } catch (err: any) {
      this.error.set(err.message || 'Failed to load site');
      console.error('Failed to select site:', err);
    }
  }

  /**
   * Retry loading data
   */
  retry(): void {
    this.loadInitialData();
  }

  /**
   * Get active site name
   */
  getActiveSiteName(): string {
    const siteId = this.data.activeSiteId();
    if (!siteId) return '';
    
    const site = this.data.sites().find(s => s._id === siteId);
    return site?.name || siteId;
  }

  /**
   * Get list of nodes for active site
   */
  getNodesList(): Node[] {
    return Array.from(this.data.nodes().values());
  }

  /**
   * Get telemetry for a specific node
   */
  getNodeTelemetry(nodeId: string): NodeTelemetry | undefined {
    return this.data.getNodeTelemetry(nodeId);
  }

  /**
   * Convert RGBW to CSS color
   */
  getRgbColor(rgbw: { r: number; g: number; b: number; w: number }): string {
    if (!rgbw) return '#000000';
    return `rgb(${rgbw.r}, ${rgbw.g}, ${rgbw.b})`;
  }

  /**
   * Check if system is healthy
   */
  isHealthy(): boolean {
    return this.data.getSystemHealth().overall;
  }

  /**
   * Set node color
   */
  async setNodeColor(nodeId: string, color: 'red' | 'green' | 'blue' | 'white'): Promise<void> {
    const siteId = this.data.activeSiteId();
    if (!siteId) return;

    const colors = {
      red: { r: 255, g: 0, b: 0, w: 0 },
      green: { r: 0, g: 255, b: 0, w: 0 },
      blue: { r: 0, g: 0, b: 255, w: 0 },
      white: { r: 0, g: 0, b: 0, w: 255 }
    };

    const command: SetLightCommand = {
      node_id: nodeId,
      site_id: siteId,
      rgbw: colors[color],
      brightness: 80,
      fade_duration: 500
    };

    try {
      await this.data.setNodeLight(command);
      console.log(`Set ${nodeId} to ${color}`);
    } catch (err) {
      console.error('Failed to set light:', err);
      this.error.set('Failed to control light');
    }
  }

  /**
   * Turn off node
   */
  async turnOff(nodeId: string): Promise<void> {
    const siteId = this.data.activeSiteId();
    if (!siteId) return;

    const command: SetLightCommand = {
      node_id: nodeId,
      site_id: siteId,
      brightness: 0,
      fade_duration: 300
    };

    try {
      await this.data.setNodeLight(command);
      console.log(`Turned off ${nodeId}`);
    } catch (err) {
      console.error('Failed to turn off light:', err);
      this.error.set('Failed to control light');
    }
  }
}
