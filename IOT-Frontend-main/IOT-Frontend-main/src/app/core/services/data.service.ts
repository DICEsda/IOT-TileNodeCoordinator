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

  constructor() {
    this.initialize();
  }

  ngOnDestroy(): void {
    this.destroy$.next();
    this.destroy$.complete();
    this.ws.disconnect();
    this.mqtt.disconnect();
  }

  /**
   * Initialize the service
   */
  private initialize(): void {
    // Connect to real-time services
    this.ws.connect();
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
      this.mqttConnected.set(this.mqtt.connected());
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
      const site = await new Promise<Site>((resolve, reject) => {
        this.api.getSiteById(siteId).subscribe({
          next: (data) => resolve(data),
          error: (err) => reject(err)
        });
      });
      
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

  /**
   * Subscribe to telemetry for all nodes in a site
   */
  private subscribeSiteTelemetry(siteId: string): void {
    // Subscribe to all node telemetry
    this.mqtt.subscribeAllNodesTelemetry(siteId)
      .pipe(takeUntil(this.destroy$))
      .subscribe(telemetry => {
        this.handleTelemetry(telemetry);
      });

    // Subscribe to pairing requests
    this.mqtt.subscribePairingRequests(siteId)
      .pipe(takeUntil(this.destroy$))
      .subscribe(pairing => {
        console.log('[DataService] Pairing request:', pairing);
      });

    if (this.env.isDevelopment) {
      console.log('[DataService] Subscribed to site telemetry:', siteId);
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

      // Also send via MQTT for immediate response
      if (this.mqtt.connected()) {
        this.mqtt.sendNodeCommand(command.site_id, command.node_id, {
          type: 'set_light',
          rgbw: command.rgbw,
          brightness: command.brightness,
          fade_duration: command.fade_duration || 500
        });
      }

      if (this.env.isDevelopment) {
        console.log('[DataService] Set light:', command);
      }
    } catch (error) {
      console.error('[DataService] Failed to set light:', error);
      throw error;
    }
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
  private handleTelemetry(telemetry: NodeTelemetry | CoordinatorTelemetry): void {
    if ('node_id' in telemetry) {
      // Node telemetry
      const currentTelemetry = new Map(this.latestTelemetry());
      currentTelemetry.set(telemetry.node_id, telemetry);
      this.latestTelemetry.set(currentTelemetry);

      // Update node in map
      const currentNodes = new Map(this.nodes());
      const node = currentNodes.get(telemetry.node_id);
      if (node) {
        node.rgbw = telemetry.rgbw;
        node.temperature = telemetry.temperature;
        node.battery_voltage = telemetry.battery_voltage;
        node.battery_percent = telemetry.battery_percent;
        node.last_seen = new Date();
        currentNodes.set(telemetry.node_id, node);
        this.nodes.set(currentNodes);
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
            this.apiHealthy.set(health.status === 'healthy');
            resolve();
          },
          error: (err) => {
            this.apiHealthy.set(false);
            reject(err);
          }
        });
      });
    } catch (error) {
      this.apiHealthy.set(false);
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
    overall: boolean;
  } {
    const api = this.apiHealthy();
    const websocket = this.wsConnected();
    const mqtt = this.mqttConnected();
    
    return {
      api,
      websocket,
      mqtt,
      overall: api && websocket && mqtt
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
