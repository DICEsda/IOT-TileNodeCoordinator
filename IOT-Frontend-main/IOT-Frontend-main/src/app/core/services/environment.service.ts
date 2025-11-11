import { Injectable } from '@angular/core';

/**
 * Environment Service
 * Manages environment-specific configuration
 * Values can be overridden via environment variables or build configuration
 */
@Injectable({
  providedIn: 'root'
})
export class EnvironmentService {
  private readonly config = {
    // Backend API configuration
    // Default adjusted to backend exposed port 8000 and root path (health is at /health)
    // If your backend actually namespaces under /api/v1, override via window.env.API_URL
    apiUrl: this.getEnvVar('API_URL', 'http://localhost:8000'),
    wsUrl: this.getEnvVar('WS_URL', 'ws://localhost:8000/ws'),
    
    // MQTT WebSocket configuration (via backend proxy)
  mqttWsUrl: this.getEnvVar('MQTT_WS_URL', 'ws://localhost:8000/mqtt'),
    
    // Feature flags
    enableGoogleHome: this.getEnvVar('ENABLE_GOOGLE_HOME', 'false') === 'true',
    enableOTA: this.getEnvVar('ENABLE_OTA', 'true') === 'true',
    
    // Polling intervals (milliseconds)
    telemetryInterval: parseInt(this.getEnvVar('TELEMETRY_INTERVAL', '5000'), 10),
    healthCheckInterval: parseInt(this.getEnvVar('HEALTH_CHECK_INTERVAL', '30000'), 10),
    
    // WebSocket reconnection
    wsReconnectDelay: parseInt(this.getEnvVar('WS_RECONNECT_DELAY', '5000'), 10),
    wsMaxReconnectAttempts: parseInt(this.getEnvVar('WS_MAX_RECONNECT_ATTEMPTS', '10'), 10),
    
    // API timeouts
    apiTimeout: parseInt(this.getEnvVar('API_TIMEOUT', '30000'), 10),
    
    // Development mode
    isDevelopment: this.getEnvVar('NODE_ENV', 'development') === 'development',
    isProduction: this.getEnvVar('NODE_ENV', 'development') === 'production',
  };

  constructor() {
    if (this.config.isDevelopment) {
      console.log('Environment Config:', this.config);
    }
  }

  get apiUrl(): string {
    return this.config.apiUrl;
  }

  get wsUrl(): string {
    return this.config.wsUrl;
  }

  get mqttWsUrl(): string {
    return this.config.mqttWsUrl;
  }

  get enableGoogleHome(): boolean {
    return this.config.enableGoogleHome;
  }

  get enableOTA(): boolean {
    return this.config.enableOTA;
  }

  get telemetryInterval(): number {
    return this.config.telemetryInterval;
  }

  get healthCheckInterval(): number {
    return this.config.healthCheckInterval;
  }

  get wsReconnectDelay(): number {
    return this.config.wsReconnectDelay;
  }

  get wsMaxReconnectAttempts(): number {
    return this.config.wsMaxReconnectAttempts;
  }

  get apiTimeout(): number {
    return this.config.apiTimeout;
  }

  get isDevelopment(): boolean {
    return this.config.isDevelopment;
  }

  get isProduction(): boolean {
    return this.config.isProduction;
  }

  /**
   * Get environment variable with fallback
   */
  private getEnvVar(key: string, defaultValue: string): string {
    // In Angular, environment variables are typically set at build time
    // For runtime configuration, use window object or config file
    if (typeof window !== 'undefined' && (window as any).env && (window as any).env[key]) {
      return (window as any).env[key];
    }
    return defaultValue;
  }
}
