import { Injectable, inject, signal, OnDestroy } from '@angular/core';
import { combineLatest, Subject, takeUntil } from 'rxjs';
import { ApiService } from './api.service';
import { WebSocketService } from './websocket.service';
import { MqttService } from './mqtt.service';
import { EnvironmentService } from './environment.service';
import { 
  Site, 
  Node, 
  Coordinator,
  NodeTelemetry, 
  CoordinatorTelemetry,
  PresenceEvent,
  SetLightCommand 
} from '../models/api.models';

/**
 * Data Service
 * High-level service that orchestrates API, WebSocket, and MQTT services
 * Provides simplified interface for components to interact with the system
 */
@Injectable({
  providedIn: 'root'
})
export class DataService implements OnDestroy {
  private readonly api = inject(ApiService);
  private readonly ws = inject(WebSocketService);
  private readonly mqtt = inject(MqttService);
  private readonly env = inject(EnvironmentService);
  private readonly destroy$ = new Subject<void>();

  // Application state
  public readonly sites = signal<Site[]>([]);
  public readonly nodes = signal<Map<string, Node>>(new Map());
  public readonly coordinators = signal<Map<string, Coordinator>>(new Map());
  public readonly activeSiteId = signal<string | null>(null);

  // Real-time telemetry
  public readonly latestTelemetry = signal<Map<string, NodeTelemetry>>(new Map());
  public readonly presenceEvents = signal<PresenceEvent[]>([]);

  // Connection states
  public readonly apiHealthy = signal<boolean>(false);
  public readonly wsConnected = signal<boolean>(false);
  public readonly mqttConnected = signal<boolean>(false);
  public readonly dbHealthy = signal<boolean>(false);
  public readonly mqttBrokerHealthy = signal<boolean>(false);
  public readonly coordOnline = signal<boolean>(false);

  constructor() {
    this.initialize();
  }

  ngOnDestroy(): void {
    this.destroy$.next();
    this.destroy$.complete();
    this.ws.disconnect();
    // this.mqtt.disconnect(); // Not used - WebSocket only
  }

  /**
   * Initialize the service
   */
  private initialize(): void {
    // Connect to WebSocket for telemetry broadcasts
    this.ws.connect();
    // Connect to MQTT for topic subscriptions (mmWave, etc.)
    this.mqtt.connect();

    // Subscribe to WebSocket telemetry
    this.ws.telemetry$
      .pipe(takeUntil(this.destroy$))
      .subscribe(telemetry => {
        this.handleTelemetry(telemetry);
      });

    // Subscribe to WebSocket presence events
    this.ws.presence$
      .pipe(takeUntil(this.destroy$))
      .subscribe(presence => {
        this.handlePresence(presence);
      });

    // Subscribe to WebSocket status updates
    this.ws.status$
      .pipe(takeUntil(this.destroy$))
      .subscribe(status => {
        this.handleStatusUpdate(status);
      });

    // Monitor connection states
    setInterval(() => {
      this.wsConnected.set(this.ws.connected());
      this.mqttConnected.set(this.ws.connected()); // Show WS status as MQTT too
    }, 1000);

    // Periodic health check
    this.checkHealth();
    setInterval(() => this.checkHealth(), this.env.healthCheckInterval);

    if (this.env.isDevelopment) {
      console.log('[DataService] Initialized');
    }
  }

  // ============================================================================
  // Site Management
  // ============================================================================

  /**
   * Load all sites
   */
  async loadSites(): Promise<void> {
    try {
      const sites = await new Promise<Site[]>((resolve, reject) => {
        this.api.getSites().subscribe({
          next: (data) => resolve(data),
          error: (err) => reject(err)
        });
      });
      this.sites.set(sites);
      
      if (this.env.isDevelopment) {
        console.log('[DataService] Loaded sites:', sites);
      }
    } catch (error) {
      console.error('[DataService] Failed to load sites:', error);
      throw error;
    }
  }

  /**
   * Load site by ID
   */
  async loadSite(siteId: string): Promise<Site> {
    try {
      let site: Site;
      try {
        site = await new Promise<Site>((resolve, reject) => {
          this.api.getSiteById(siteId).subscribe({
            next: (data) => resolve(data),
            error: (err) => reject(err)
          });
        });
      } catch (err: any) {
        // If site not found (404), create a placeholder
        const errorMsg = err.message || err.toString();
        if (errorMsg.includes('404') || errorMsg.includes('Not Found')) {
          console.warn(`[DataService] Site ${siteId} not found, creating placeholder`);
          site = {
            _id: siteId,
            name: siteId,
            location: 'Unknown',
            coordinators: [],
            zones: [],
            created_at: new Date(),
            updated_at: new Date()
          };
        } else {
          throw err;
        }
      }
      
      // Update sites array
      const currentSites = this.sites();
      const index = currentSites.findIndex(s => s._id === siteId);
      if (index >= 0) {
        currentSites[index] = site;
      } else {
        currentSites.push(site);
      }
      this.sites.set([...currentSites]);
      
      // Set as active site
      this.activeSiteId.set(siteId);
      
      // Subscribe to MQTT telemetry for this site
      this.subscribeSiteTelemetry(siteId);
      
      return site;
    } catch (error) {
      console.error('[DataService] Failed to load site:', error);
      throw error;
    }
  }

  private subscribedSites = new Set<string>();

  /**
   * Subscribe to telemetry for all nodes in a site
   */
  private subscribeSiteTelemetry(siteId: string): void {
    if (this.subscribedSites.has(siteId)) {
      return;
    }
    this.subscribedSites.add(siteId);

    // MQTT subscriptions disabled - WebSocket handles all telemetry
    // this.mqtt.subscribeAllNodesTelemetry(siteId)
    //   .pipe(takeUntil(this.destroy$))
    //   .subscribe(telemetry => {
    //     if (this.env.isDevelopment) {
    //       console.log('[DataService] Node telemetry:', telemetry);
    //     }
    //     this.handleTelemetry(telemetry);
    //   });

    // this.mqtt.messages$
    //   .pipe(takeUntil(this.destroy$))
    //   .subscribe(msg => {
    //     console.log('[DataService] MQTT message received:', msg.topic, msg.payload);
    //     const parts = msg.topic.split('/');
    //     if (parts.length === 5 && parts[0] === 'site' && parts[1] === siteId && parts[2] === 'coord' && parts[4] === 'telemetry') {
    //       const coordId = parts[3];
    //       const telemetry = { ...msg.payload, coord_id: coordId };
    //       console.log('[DataService] Coordinator telemetry received:', telemetry);
    //       this.handleTelemetry(telemetry);
    //     }
    //   });
    
    // this.mqtt.subscribeCoordinatorTelemetry(siteId, '+');

    // this.mqtt.subscribePairingRequests(siteId)
    //   .pipe(takeUntil(this.destroy$))
    //   .subscribe(pairing => {
    //     console.log('[DataService] Pairing request:', pairing);
    //   });

    if (this.env.isDevelopment) {
      console.log('[DataService] Site telemetry via WebSocket only:', siteId);
    }
  }

  // ============================================================================
  // Node Management
  // ============================================================================

  /**
   * Load node by ID
   */
  async loadNode(nodeId: string): Promise<Node> {
    try {
      const node = await new Promise<Node>((resolve, reject) => {
        this.api.getNodeById(nodeId).subscribe({
          next: (data) => resolve(data),
          error: (err) => reject(err)
        });
      });
      
      // Update nodes map
      const currentNodes = new Map(this.nodes());
      currentNodes.set(nodeId, node);
      this.nodes.set(currentNodes);
      
      return node;
    } catch (error) {
      console.error('[DataService] Failed to load node:', error);
      throw error;
    }
  }

  /**
   * Control node light
   */
  async setNodeLight(command: SetLightCommand): Promise<void> {
    try {
      await new Promise<void>((resolve, reject) => {
        this.api.setLight(command).subscribe({
          next: () => resolve(),
          error: (err) => reject(err)
        });
      });

      // Commands are sent via API, backend will publish to MQTT
      // if (this.mqtt.connected()) {
      //   this.mqtt.sendNodeCommand(command.site_id, command.node_id, {
      //     type: 'set_light',
      //     rgbw: command.rgbw,
      //     brightness: command.brightness,
      //     fade_duration: command.fade_duration || 500
      //   });
      // }

      if (this.env.isDevelopment) {
        console.log('[DataService] Set light:', command);
      }
    } catch (error) {
      console.error('[DataService] Failed to set light:', error);
      throw error;
    }
  }

  /**
   * Send direct command to node via MQTT (for per-pixel control)
   */
  sendNodeCommand(nodeId: string, command: any): void {
    console.log('[DataService] sendNodeCommand called:', nodeId, command);
    const siteId = this.activeSiteId() || 'site001';
    console.log('[DataService] siteId:', siteId, 'mqtt.connected():', this.mqtt.connected());
    
    if (this.mqtt.connected()) {
      console.log('[DataService] About to call mqtt.sendNodeCommand...');
      this.mqtt.sendNodeCommand(siteId, nodeId, command);
      console.log('[DataService] mqtt.sendNodeCommand called successfully');
      
      if (this.env.isDevelopment) {
        console.log('[DataService] Sent node command:', nodeId, command);
      }
    } else {
      console.warn('[DataService] MQTT not connected, cannot send command');
    }
  }

  /**
   * Control coordinator light
   */
  async setCoordinatorLight(siteId: string, coordId: string, rgb: { r: number, g: number, b: number }): Promise<void> {
    // Commands go via API/WebSocket, not direct MQTT
    // if (this.mqtt.connected()) {
    //   this.mqtt.sendCoordinatorCommand(siteId, coordId, {
    //     cmd: 'led.set',
    //     r: rgb.r,
    //     g: rgb.g,
    //     b: rgb.b
    //   });
    // }
    console.warn('[DataService] setCoordinatorLight not implemented via WebSocket');
  }

  /**
   * Reset coordinator light
   */
  async resetCoordinatorLight(siteId: string, coordId: string): Promise<void> {
    // Commands go via API/WebSocket, not direct MQTT
    // if (this.mqtt.connected()) {
    //   this.mqtt.sendCoordinatorCommand(siteId, coordId, {
    //     cmd: 'led.reset'
    //   });
    // }
    console.warn('[DataService] resetCoordinatorLight not implemented via WebSocket');
  }

  /**
   * Start pairing mode on coordinator
   */
  async startPairing(siteId: string, coordId: string, durationMs: number = 60000): Promise<void> {
    // Commands go via API/WebSocket, not direct MQTT
    // if (this.mqtt.connected()) {
    //   this.mqtt.sendCoordinatorCommand(siteId, coordId, {
    //     cmd: 'pair',
    //     duration_ms: durationMs
    //   });
    // }
    console.warn('[DataService] startPairing not implemented via WebSocket');
  }

  /**
   * Approve node pairing
   */
  async approvePairing(nodeId: string, siteId: string, zoneId?: string): Promise<void> {
    try {
      await new Promise<void>((resolve, reject) => {
        this.api.approveNodePairing({
          node_id: nodeId,
          site_id: siteId,
          zone_id: zoneId,
          approve: true
        }).subscribe({
          next: () => resolve(),
          error: (err) => reject(err)
        });
      });

      if (this.env.isDevelopment) {
        console.log('[DataService] Approved pairing:', nodeId);
      }
    } catch (error) {
      console.error('[DataService] Failed to approve pairing:', error);
      throw error;
    }
  }

  // ============================================================================
  // Coordinator Management
  // ============================================================================

  /**
   * Load coordinator by ID
   */
  async loadCoordinator(coordId: string): Promise<Coordinator> {
    try {
      const coordinator = await new Promise<Coordinator>((resolve, reject) => {
        this.api.getCoordinatorById(coordId).subscribe({
          next: (data) => resolve(data),
          error: (err) => reject(err)
        });
      });
      
      // Update coordinators map
      const currentCoords = new Map(this.coordinators());
      currentCoords.set(coordId, coordinator);
      this.coordinators.set(currentCoords);
      
      return coordinator;
    } catch (error) {
      console.error('[DataService] Failed to load coordinator:', error);
      throw error;
    }
  }

  // ============================================================================
  // Real-time Data Handlers
  // ============================================================================

  /**
   * Handle incoming telemetry
   */
  private handleTelemetry(telemetry: NodeTelemetry | CoordinatorTelemetry | any): void {
    if (this.env.isDevelopment) {
       // console.log('[DataService] Telemetry received:', telemetry);
    }

    // Handle both snake_case (nodeId) and camelCase (node_id) from WebSocket
    if ('nodeId' in telemetry) {
      telemetry.node_id = telemetry.nodeId;
    }
    if ('coordId' in telemetry) {
      telemetry.coord_id = telemetry.coordId;
    }
    if ('siteId' in telemetry) {
      telemetry.site_id = telemetry.siteId;
    }

    if ('node_id' in telemetry && !('coord_id' in telemetry)) {
      console.log('[DataService] Processing node telemetry for:', telemetry.node_id);
      // Node telemetry
      const currentTelemetry = new Map(this.latestTelemetry());
      currentTelemetry.set(telemetry.node_id, telemetry);
      this.latestTelemetry.set(currentTelemetry);

      // Update node in map
      const currentNodes = new Map(this.nodes());
      let node = currentNodes.get(telemetry.node_id);
      
      if (node) {
        // Update existing node - check if values actually changed to avoid unnecessary re-renders
        const newTemp = (telemetry as any).tempC || telemetry.temp_c;
        
        // Extract RGBW from nested light object if present (WebSocket format)
        let avgR = telemetry.avg_r || 0;
        let avgG = telemetry.avg_g || 0;
        let avgB = telemetry.avg_b || 0;
        let avgW = telemetry.avg_w || 0;
        if ((telemetry as any).light) {
          avgR = (telemetry as any).light.avgR || 0;
          avgG = (telemetry as any).light.avgG || 0;
          avgB = (telemetry as any).light.avgB || 0;
          avgW = (telemetry as any).light.avgW || 0;
        }
        
        const hasChanges = 
          !node.rgbw ||
          node.rgbw.r !== avgR ||
          node.rgbw.g !== avgG ||
          node.rgbw.b !== avgB ||
          node.rgbw.w !== avgW ||
          node.temperature !== newTemp ||
          node.status !== 'online';
        
        if (hasChanges) {
          console.log('[DataService] Updating existing node:', telemetry.node_id, 'tempC:', newTemp);
          node.rgbw = { r: avgR, g: avgG, b: avgB, w: avgW };
          node.temperature = newTemp;
          node.battery_voltage = (telemetry as any).vbatMv ? (telemetry as any).vbatMv / 1000 : undefined;
          node.battery_percent = (telemetry as any).vbatMv ? Math.min(100, Math.max(0, (((telemetry as any).vbatMv - 3000) / (4200 - 3000)) * 100)) : undefined;
          node.brightness = (telemetry as any).light?.brightness || telemetry.brightness;
          node.status = 'online';
          node.last_seen = new Date();
          currentNodes.set(telemetry.node_id, node);
        }else {
          // Just update timestamps without triggering signal update
          node.last_seen = new Date();
          return; // Skip signal update if nothing visual changed
        }
      }else {
        console.log('[DataService] Creating new node:', telemetry.node_id);
        
        // Extract RGBW from nested light object if present (WebSocket format)
        let avgR = telemetry.avg_r || 0;
        let avgG = telemetry.avg_g || 0;
        let avgB = telemetry.avg_b || 0;
        let avgW = telemetry.avg_w || 0;
        if ((telemetry as any).light) {
          avgR = (telemetry as any).light.avgR || 0;
          avgG = (telemetry as any).light.avgG || 0;
          avgB = (telemetry as any).light.avgB || 0;
          avgW = (telemetry as any).light.avgW || 0;
        }
        
        // Create new node if it doesn't exist
        node = {
          _id: telemetry.node_id,
          node_id: telemetry.node_id,
          name: (telemetry as any).lightId || telemetry.light_id || telemetry.node_id,
          site_id: this.activeSiteId() || 'site001',
          mac_address: telemetry.node_id,
          paired: true,
          status: 'online',
          rgbw: { r: avgR, g: avgG, b: avgB, w: avgW },
          brightness: (telemetry as any).light?.brightness || telemetry.brightness,
          temperature: (telemetry as any).tempC || telemetry.temp_c,
          battery_voltage: (telemetry as any).vbatMv ? (telemetry as any).vbatMv / 1000 : undefined,
          battery_percent: (telemetry as any).vbatMv ? Math.min(100, Math.max(0, (((telemetry as any).vbatMv - 3000) / (4200 - 3000)) * 100)) : undefined,
          last_seen: new Date(),
          created_at: new Date(),
          updated_at: new Date()
        };
        currentNodes.set(telemetry.node_id, node);
      }
      
      console.log('[DataService] Nodes map now has', currentNodes.size, 'nodes');
      this.nodes.set(currentNodes);
    } else if ('coord_id' in telemetry) {
      // Coordinator telemetry - only log every 10th update to reduce console spam
      if (Math.random() < 0.1) {
        console.log('[DataService] Processing coordinator telemetry for:', telemetry.coord_id);
      }
      
      // Coordinator telemetry
      const currentCoords = new Map(this.coordinators());
      const coord = currentCoords.get(telemetry.coord_id);
      
      if (coord) {
        // Update existing coordinator
        coord.last_seen = new Date();
        coord.status = 'online';
        
        // Update other fields if available in telemetry (support both camelCase and snake_case)
        if (telemetry.wifi_rssi !== undefined) coord.wifi_rssi = telemetry.wifi_rssi;
        if ((telemetry as any).lightLux !== undefined) coord.light_lux = (telemetry as any).lightLux;
        if (telemetry.light_lux !== undefined) coord.light_lux = telemetry.light_lux;
        if ((telemetry as any).tempC !== undefined) coord.temp_c = (telemetry as any).tempC;
        if (telemetry.temp_c !== undefined) coord.temp_c = telemetry.temp_c;
        if (telemetry.heap_free !== undefined) coord.heap_free = telemetry.heap_free;
        
        // Don't trigger signal update for every telemetry (reduces flickering)
        // The coordinator data is already updated in the map
        
        // Update health signal if needed
        if (!this.coordOnline()) {
          this.coordOnline.set(true);
        }
      } else {
        // Create a temporary coordinator entry if it doesn't exist
        const newCoord: Coordinator = {
          _id: telemetry.coord_id, // Use coord_id as _id for temp entry
          coord_id: telemetry.coord_id,
          site_id: telemetry.site_id || 'site001',
          mac_address: telemetry.coord_id, // Fallback
          status: 'online',
          last_seen: new Date(),
          created_at: new Date(),
          updated_at: new Date(),
          wifi_rssi: telemetry.wifi_rssi,
          light_lux: (telemetry as any).lightLux || telemetry.light_lux, // Support camelCase from backend
          temp_c: (telemetry as any).tempC || telemetry.temp_c, // Support camelCase from backend
          heap_free: telemetry.heap_free
        };
        
        currentCoords.set(telemetry.coord_id, newCoord);
        this.coordinators.set(currentCoords);
        this.coordOnline.set(true);
      }
    }
  }

  /**
   * Handle presence events
   */
  private handlePresence(presence: PresenceEvent): void {
    const currentEvents = this.presenceEvents();
    currentEvents.unshift(presence);
    
    // Keep only last 100 events
    if (currentEvents.length > 100) {
      currentEvents.pop();
    }
    
    this.presenceEvents.set([...currentEvents]);
  }

  /**
   * Handle status updates
   */
  private handleStatusUpdate(status: any): void {
    if (status.entity_type === 'node') {
      const currentNodes = new Map(this.nodes());
      const node = currentNodes.get(status.entity_id);
      if (node) {
        node.status = status.status;
        node.last_seen = new Date();
        currentNodes.set(status.entity_id, node);
        this.nodes.set(currentNodes);
      }
    } else if (status.entity_type === 'coordinator') {
      const currentCoords = new Map(this.coordinators());
      const coord = currentCoords.get(status.entity_id);
      if (coord) {
        coord.status = status.status;
        coord.last_seen = new Date();
        currentCoords.set(status.entity_id, coord);
        this.coordinators.set(currentCoords);
      }
    }
  }

  // ============================================================================
  // Health & Status
  // ============================================================================

  /**
   * Check backend health
   */
  private async checkHealth(): Promise<void> {
    try {
      await new Promise<void>((resolve, reject) => {
        this.api.getHealth().subscribe({
          next: (health) => {
            this.apiHealthy.set(health.status === 'healthy' || health.status === 'degraded');
            this.dbHealthy.set(health.database ?? false);
            this.mqttBrokerHealthy.set(health.mqtt ?? false);
            this.coordOnline.set(health.coordinator ?? false);
            resolve();
          },
          error: (err) => {
            this.apiHealthy.set(false);
            this.dbHealthy.set(false);
            this.mqttBrokerHealthy.set(false);
            this.coordOnline.set(false);
            reject(err);
          }
        });
      });
    } catch (error) {
      this.apiHealthy.set(false);
      this.dbHealthy.set(false);
      this.mqttBrokerHealthy.set(false);
      this.coordOnline.set(false);
      if (this.env.isDevelopment) {
        console.error('[DataService] Health check failed:', error);
      }
    }
  }

  /**
   * Get overall system health
   */
  getSystemHealth(): {
    api: boolean;
    websocket: boolean;
    mqtt: boolean;
    database: boolean;
    mqttBroker: boolean;
    coordinator: boolean;
    overall: boolean;
  } {
    const api = this.apiHealthy();
    const websocket = this.wsConnected();
    const database = this.dbHealthy();
    const mqttBroker = this.mqttBrokerHealthy();
    const coordinator = this.coordOnline();
    
    // MQTT indicator shows WebSocket status (not actual MQTT)
    const mqtt = websocket;
    
    return {
      api,
      websocket,
      mqtt,
      database,
      mqttBroker,
      coordinator,
      overall: api && websocket && database
    };
  }

  /**
   * Get node by ID from cache
   */
  getNode(nodeId: string): Node | undefined {
    return this.nodes().get(nodeId);
  }

  /**
   * Get coordinator by ID from cache
   */
  getCoordinator(coordId: string): Coordinator | undefined {
    return this.coordinators().get(coordId);
  }

  /**
   * Get latest telemetry for a node
   */
  getNodeTelemetry(nodeId: string): NodeTelemetry | undefined {
    return this.latestTelemetry().get(nodeId);
  }
}
