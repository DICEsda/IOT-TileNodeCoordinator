import { Injectable, inject, signal } from '@angular/core';
import { Subject } from 'rxjs';
import { EnvironmentService } from './environment.service';

/**
 * MQTT Topic Structure (as per PRD)
 * site/{siteId}/coord/{coordId}/telemetry
 * site/{siteId}/coord/{coordId}/mmwave
 * site/{siteId}/node/{nodeId}/telemetry
 * site/{siteId}/node/{nodeId}/pairing
 * site/{siteId}/zone/{zoneId}/presence
 * 
 * Commands:
 * site/{siteId}/node/{nodeId}/cmd
 * site/{siteId}/zone/{zoneId}/cmd
 */

export interface MqttMessage {
  topic: string;
  payload: any;
  timestamp: Date;
}

export interface MqttConnectionState {
  connected: boolean;
  connecting: boolean;
  error: string | null;
}

/**
 * MQTT Service
 * Handles MQTT communication via WebSocket bridge through backend
 * The backend proxies MQTT messages over WebSocket
 */
@Injectable({
  providedIn: 'root'
})
export class MqttService {
  private readonly env = inject(EnvironmentService);
  private ws: WebSocket | null = null;
  private reconnectAttempts = 0;
  private reconnectTimer: any = null;
  private isIntentionalClose = false;
  private subscriptions = new Set<string>();

  // Connection state
  public readonly connected = signal<boolean>(false);
  public readonly connecting = signal<boolean>(false);
  public readonly connectionError = signal<string | null>(null);

  // Message streams
  private readonly messageSubject = new Subject<MqttMessage>();
  public readonly messages$ = this.messageSubject.asObservable();

  // Topic-specific subjects
  private readonly topicSubjects = new Map<string, Subject<any>>();

  constructor() {
    if (this.env.isDevelopment) {
      console.log('[MQTT] Service initialized');
    }
  }

  /**
   * Connect to MQTT via WebSocket bridge
   */
  connect(): void {
    if (this.ws && (this.ws.readyState === WebSocket.OPEN || this.ws.readyState === WebSocket.CONNECTING)) {
      console.warn('[MQTT] Already connected or connecting');
      return;
    }

    this.isIntentionalClose = false;
    this.connecting.set(true);
    this.connectionError.set(null);

    try {
      this.ws = new WebSocket(this.env.mqttWsUrl);
      this.setupEventHandlers();
      
      if (this.env.isDevelopment) {
        console.log('[MQTT] Connecting to:', this.env.mqttWsUrl);
      }
    } catch (error) {
      this.handleConnectionError(error);
    }
  }

  /**
   * Disconnect from MQTT
   */
  disconnect(): void {
    this.isIntentionalClose = true;
    this.clearReconnectTimer();
    
    if (this.ws) {
      this.ws.close(1000, 'Client disconnect');
      this.ws = null;
    }
    
    this.connected.set(false);
    this.connecting.set(false);
    this.subscriptions.clear();
    
    if (this.env.isDevelopment) {
      console.log('[MQTT] Disconnected');
    }
  }

  /**
   * Subscribe to MQTT topic
   */
  subscribe(topic: string): Subject<any> {
    if (!this.topicSubjects.has(topic)) {
      this.topicSubjects.set(topic, new Subject<any>());
    }

    this.subscriptions.add(topic);

    // Send subscription request to backend
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.send({
        type: 'subscribe',
        topic: topic
      });
    }

    if (this.env.isDevelopment) {
      console.log('[MQTT] Subscribed to:', topic);
    }

    return this.topicSubjects.get(topic)!;
  }

  /**
   * Unsubscribe from MQTT topic
   */
  unsubscribe(topic: string): void {
    this.subscriptions.delete(topic);

    // Send unsubscription request to backend
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.send({
        type: 'unsubscribe',
        topic: topic
      });
    }

    // Clean up subject
    const subject = this.topicSubjects.get(topic);
    if (subject) {
      subject.complete();
      this.topicSubjects.delete(topic);
    }

    if (this.env.isDevelopment) {
      console.log('[MQTT] Unsubscribed from:', topic);
    }
  }

  /**
   * Publish message to MQTT topic
   */
  publish(topic: string, payload: any, qos: 0 | 1 | 2 = 0): void {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new Error('MQTT WebSocket is not connected');
    }

    this.send({
      type: 'publish',
      topic: topic,
      payload: payload,
      qos: qos
    });

    if (this.env.isDevelopment) {
      console.log('[MQTT] Published to', topic, ':', payload);
    }
  }

  /**
   * Subscribe to telemetry for a specific node
   */
  subscribeNodeTelemetry(siteId: string, nodeId: string): Subject<any> {
    return this.subscribe(`site/${siteId}/node/${nodeId}/telemetry`);
  }

  /**
   * Subscribe to telemetry for all nodes in a site
   */
  subscribeAllNodesTelemetry(siteId: string): Subject<any> {
    return this.subscribe(`site/${siteId}/node/+/telemetry`);
  }

  /**
   * Subscribe to coordinator telemetry
   */
  subscribeCoordinatorTelemetry(siteId: string, coordId: string): Subject<any> {
    return this.subscribe(`site/${siteId}/coord/${coordId}/telemetry`);
  }

  /**
   * Subscribe to presence events for a zone
   */
  subscribeZonePresence(siteId: string, zoneId: string): Subject<any> {
    return this.subscribe(`site/${siteId}/zone/${zoneId}/presence`);
  }

  /**
   * Subscribe to pairing requests
   */
  subscribePairingRequests(siteId: string): Subject<any> {
    return this.subscribe(`site/${siteId}/node/+/pairing`);
  }

  /**
   * Subscribe to mmWave frames for a coordinator or all coordinators in a site
   */
  subscribeCoordinatorMmwave(siteId: string, coordinatorId: string = '+'): Subject<any> {
    return this.subscribe(`site/${siteId}/coord/${coordinatorId}/mmwave`);
  }

  /**
   * Send command to a node
   */
  sendNodeCommand(siteId: string, nodeId: string, command: any): void {
    const topic = `site/${siteId}/node/${nodeId}/cmd`;
    this.publish(topic, command, 1);
  }

  /**
   * Send command to a zone
   */
  sendZoneCommand(siteId: string, zoneId: string, command: any): void {
    const topic = `site/${siteId}/zone/${zoneId}/cmd`;
    this.publish(topic, command, 1);
  }

  /**
   * Setup WebSocket event handlers
   */
  private setupEventHandlers(): void {
    if (!this.ws) return;

    this.ws.onopen = () => {
      this.connected.set(true);
      this.connecting.set(false);
      this.reconnectAttempts = 0;
      this.connectionError.set(null);
      
      // Resubscribe to all topics
      this.resubscribeAll();
      
      if (this.env.isDevelopment) {
        console.log('[MQTT] Connected');
      }
    };

    this.ws.onmessage = (event: MessageEvent) => {
      try {
        const data = JSON.parse(event.data);
        this.handleMessage(data);
      } catch (error) {
        console.error('[MQTT] Failed to parse message:', error);
      }
    };

    this.ws.onerror = (event: Event) => {
      console.error('[MQTT] Error:', event);
      this.connectionError.set('MQTT WebSocket error occurred');
    };

    this.ws.onclose = (event: CloseEvent) => {
      this.connected.set(false);
      this.connecting.set(false);
      
      if (this.env.isDevelopment) {
        console.log('[MQTT] Closed:', event.code, event.reason);
      }

      if (!this.isIntentionalClose) {
        this.attemptReconnect();
      }
    };
  }

  /**
   * Handle incoming MQTT message from WebSocket
   */
  private handleMessage(data: any): void {
    if (data.type === 'message' && data.topic && data.payload) {
      const message: MqttMessage = {
        topic: data.topic,
        payload: data.payload,
        timestamp: new Date(data.timestamp || Date.now())
      };

      // Emit to general message stream
      this.messageSubject.next(message);

      // Emit to topic-specific subjects
      this.topicSubjects.forEach((subject, subscribedTopic) => {
        if (this.topicMatches(subscribedTopic, data.topic)) {
          subject.next(data.payload);
        }
      });

      if (this.env.isDevelopment) {
        console.log('[MQTT] Received:', data.topic, data.payload);
      }
    }
  }

  /**
   * Check if topic matches subscription pattern
   * Supports MQTT wildcards: + (single level), # (multi level)
   */
  private topicMatches(pattern: string, topic: string): boolean {
    if (pattern === topic) return true;

    const patternParts = pattern.split('/');
    const topicParts = topic.split('/');

    for (let i = 0; i < patternParts.length; i++) {
      if (patternParts[i] === '#') {
        return true;
      }
      if (patternParts[i] === '+') {
        continue;
      }
      if (patternParts[i] !== topicParts[i]) {
        return false;
      }
    }

    return patternParts.length === topicParts.length;
  }

  /**
   * Resubscribe to all topics after reconnection
   */
  private resubscribeAll(): void {
    if (this.subscriptions.size === 0) return;

    this.subscriptions.forEach(topic => {
      this.send({
        type: 'subscribe',
        topic: topic
      });
    });

    if (this.env.isDevelopment) {
      console.log('[MQTT] Resubscribed to', this.subscriptions.size, 'topics');
    }
  }

  /**
   * Send message via WebSocket
   */
  private send(message: any): void {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new Error('MQTT WebSocket is not connected');
    }

    try {
      this.ws.send(JSON.stringify(message));
    } catch (error) {
      console.error('[MQTT] Failed to send message:', error);
      throw error;
    }
  }

  /**
   * Attempt to reconnect
   */
  private attemptReconnect(): void {
    if (this.reconnectAttempts >= this.env.wsMaxReconnectAttempts) {
      const error = 'Max MQTT reconnection attempts reached';
      console.error('[MQTT]', error);
      this.connectionError.set(error);
      return;
    }

    this.reconnectAttempts++;
    const delay = this.env.wsReconnectDelay * Math.min(this.reconnectAttempts, 5);
    
    if (this.env.isDevelopment) {
      console.log(`[MQTT] Reconnecting in ${delay}ms (attempt ${this.reconnectAttempts}/${this.env.wsMaxReconnectAttempts})`);
    }

    this.clearReconnectTimer();
    this.reconnectTimer = setTimeout(() => {
      this.connect();
    }, delay);
  }

  /**
   * Clear reconnection timer
   */
  private clearReconnectTimer(): void {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
  }

  /**
   * Handle connection error
   */
  private handleConnectionError(error: any): void {
    console.error('[MQTT] Connection error:', error);
    this.connecting.set(false);
    this.connectionError.set(error.message || 'Connection failed');
    
    if (!this.isIntentionalClose) {
      this.attemptReconnect();
    }
  }

  /**
   * Get current connection state
   */
  getConnectionState(): MqttConnectionState {
    return {
      connected: this.connected(),
      connecting: this.connecting(),
      error: this.connectionError()
    };
  }

  /**
   * Get list of active subscriptions
   */
  getSubscriptions(): string[] {
    return Array.from(this.subscriptions);
  }
}
