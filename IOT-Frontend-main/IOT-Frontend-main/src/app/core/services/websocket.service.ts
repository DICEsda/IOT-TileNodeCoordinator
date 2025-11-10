import { Injectable, inject, signal } from '@angular/core';
import { Observable, Subject, BehaviorSubject } from 'rxjs';
import {
  WSMessage,
  WSTelemetryMessage,
  WSPresenceMessage,
  WSStatusMessage,
  WSPairingMessage,
  NodeTelemetry,
  CoordinatorTelemetry,
  PresenceEvent
} from '../models/api.models';
import { EnvironmentService } from './environment.service';

/**
 * WebSocket Service
 * Manages real-time bidirectional communication with the backend
 * Handles automatic reconnection and message routing
 */
@Injectable({
  providedIn: 'root'
})
export class WebSocketService {
  private readonly env = inject(EnvironmentService);
  private ws: WebSocket | null = null;
  private reconnectAttempts = 0;
  private reconnectTimer: any = null;
  private isIntentionalClose = false;

  // Connection state
  public readonly connected = signal<boolean>(false);
  public readonly connecting = signal<boolean>(false);
  public readonly connectionError = signal<string | null>(null);

  // Message streams
  private readonly messageSubject = new Subject<WSMessage>();
  private readonly telemetrySubject = new Subject<NodeTelemetry | CoordinatorTelemetry>();
  private readonly presenceSubject = new Subject<PresenceEvent>();
  private readonly statusSubject = new Subject<WSStatusMessage['payload']>();
  private readonly pairingSubject = new Subject<WSPairingMessage['payload']>();
  private readonly errorSubject = new Subject<{ message: string; error?: any }>();

  // Observable streams
  public readonly messages$ = this.messageSubject.asObservable();
  public readonly telemetry$ = this.telemetrySubject.asObservable();
  public readonly presence$ = this.presenceSubject.asObservable();
  public readonly status$ = this.statusSubject.asObservable();
  public readonly pairing$ = this.pairingSubject.asObservable();
  public readonly errors$ = this.errorSubject.asObservable();

  constructor() {
    if (this.env.isDevelopment) {
      console.log('[WebSocket] Service initialized');
    }
  }

  /**
   * Connect to WebSocket server
   */
  connect(): void {
    if (this.ws && (this.ws.readyState === WebSocket.OPEN || this.ws.readyState === WebSocket.CONNECTING)) {
      console.warn('[WebSocket] Already connected or connecting');
      return;
    }

    this.isIntentionalClose = false;
    this.connecting.set(true);
    this.connectionError.set(null);

    try {
      this.ws = new WebSocket(this.env.wsUrl);
      this.setupEventHandlers();
      
      if (this.env.isDevelopment) {
        console.log('[WebSocket] Connecting to:', this.env.wsUrl);
      }
    } catch (error) {
      this.handleConnectionError(error);
    }
  }

  /**
   * Disconnect from WebSocket server
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
    
    if (this.env.isDevelopment) {
      console.log('[WebSocket] Disconnected');
    }
  }

  /**
   * Send message to server
   */
  send(message: any): void {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      const error = 'WebSocket is not connected';
      this.errorSubject.next({ message: error });
      throw new Error(error);
    }

    try {
      const payload = typeof message === 'string' ? message : JSON.stringify(message);
      this.ws.send(payload);
      
      if (this.env.isDevelopment) {
        console.log('[WebSocket] Sent:', message);
      }
    } catch (error) {
      this.errorSubject.next({ 
        message: 'Failed to send message', 
        error 
      });
      throw error;
    }
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
      
      if (this.env.isDevelopment) {
        console.log('[WebSocket] Connected');
      }
    };

    this.ws.onmessage = (event: MessageEvent) => {
      try {
        const data = JSON.parse(event.data);
        this.handleMessage(data);
      } catch (error) {
        console.error('[WebSocket] Failed to parse message:', error);
        this.errorSubject.next({ 
          message: 'Failed to parse message', 
          error 
        });
      }
    };

    this.ws.onerror = (event: Event) => {
      console.error('[WebSocket] Error:', event);
      this.connectionError.set('WebSocket error occurred');
      this.errorSubject.next({ 
        message: 'WebSocket error occurred',
        error: event 
      });
    };

    this.ws.onclose = (event: CloseEvent) => {
      this.connected.set(false);
      this.connecting.set(false);
      
      if (this.env.isDevelopment) {
        console.log('[WebSocket] Closed:', event.code, event.reason);
      }

      if (!this.isIntentionalClose) {
        this.attemptReconnect();
      }
    };
  }

  /**
   * Handle incoming WebSocket message
   */
  private handleMessage(data: WSMessage): void {
    // Emit to general message stream
    this.messageSubject.next(data);

    // Route to specific streams based on type
    switch (data.type) {
      case 'telemetry':
        const telemetryMsg = data as WSTelemetryMessage;
        this.telemetrySubject.next(telemetryMsg.payload);
        break;

      case 'presence':
        const presenceMsg = data as WSPresenceMessage;
        this.presenceSubject.next(presenceMsg.payload);
        break;

      case 'status':
        const statusMsg = data as WSStatusMessage;
        this.statusSubject.next(statusMsg.payload);
        break;

      case 'pairing':
        const pairingMsg = data as WSPairingMessage;
        this.pairingSubject.next(pairingMsg.payload);
        break;

      case 'error':
        this.errorSubject.next({ 
          message: data.payload?.message || 'Unknown error',
          error: data.payload 
        });
        break;

      case 'command_ack':
        // Command acknowledgment - could be used for tracking command status
        if (this.env.isDevelopment) {
          console.log('[WebSocket] Command ACK:', data.payload);
        }
        break;

      default:
        console.warn('[WebSocket] Unknown message type:', data.type);
    }
  }

  /**
   * Attempt to reconnect to WebSocket server
   */
  private attemptReconnect(): void {
    if (this.reconnectAttempts >= this.env.wsMaxReconnectAttempts) {
      const error = 'Max reconnection attempts reached';
      console.error('[WebSocket]', error);
      this.connectionError.set(error);
      this.errorSubject.next({ message: error });
      return;
    }

    this.reconnectAttempts++;
    const delay = this.env.wsReconnectDelay * Math.min(this.reconnectAttempts, 5);
    
    if (this.env.isDevelopment) {
      console.log(`[WebSocket] Reconnecting in ${delay}ms (attempt ${this.reconnectAttempts}/${this.env.wsMaxReconnectAttempts})`);
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
    console.error('[WebSocket] Connection error:', error);
    this.connecting.set(false);
    this.connectionError.set(error.message || 'Connection failed');
    this.errorSubject.next({ 
      message: 'Failed to connect to WebSocket', 
      error 
    });
    
    if (!this.isIntentionalClose) {
      this.attemptReconnect();
    }
  }

  /**
   * Get current connection state
   */
  getConnectionState(): {
    connected: boolean;
    connecting: boolean;
    error: string | null;
    attempts: number;
  } {
    return {
      connected: this.connected(),
      connecting: this.connecting(),
      error: this.connectionError(),
      attempts: this.reconnectAttempts
    };
  }

  /**
   * Reset connection (disconnect and reconnect)
   */
  reset(): void {
    this.disconnect();
    setTimeout(() => this.connect(), 100);
  }
}
