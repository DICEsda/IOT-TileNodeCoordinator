import { Injectable, inject } from '@angular/core';
import { Observable, throwError, timeout, catchError } from 'rxjs';
import {
  Site,
  Coordinator,
  Node,
  SetLightCommand,
  ColorProfileCommand,
  PairingApproval,
  OTAJob,
  StartOTARequest,
  HealthStatus,
  GoogleHomeDevice,
  GoogleHomeState,
  MmwaveFrame
} from '../models/api.models';
import { EnvironmentService } from './environment.service';

/**
 * API Service
 * Handles all HTTP communication with the backend API
 * Provides type-safe methods for all endpoints
 */
@Injectable({
  providedIn: 'root'
})
export class ApiService {
  private readonly env = inject(EnvironmentService);
  private readonly baseUrl = this.env.apiUrl;

  constructor() {}

  // ============================================================================
  // Health & System
  // ============================================================================

  /**
   * Check backend health status
   */
  getHealth(): Observable<HealthStatus> {
    return this.get<HealthStatus>('/health');
  }

  // ============================================================================
  // Sites API
  // ============================================================================

  /**
   * Get all sites
   */
  getSites(): Observable<Site[]> {
    return this.get<Site[]>('/sites');
  }

  /**
   * Get site by ID
   */
  getSiteById(id: string): Observable<Site> {
    return this.get<Site>(`/sites/${id}`);
  }

  // ============================================================================
  // Coordinators API
  // ============================================================================

  /**
   * Get coordinator by ID
   */
  getCoordinatorById(id: string): Observable<Coordinator> {
    return this.get<Coordinator>(`/coordinators/${id}`);
  }

  // ============================================================================
  // Nodes API
  // ============================================================================

  /**
   * Get node by ID
   */
  getNodeById(id: string): Observable<Node> {
    return this.get<Node>(`/nodes/${id}`);
  }

  // ============================================================================
  // Commands API
  // ============================================================================

  /**
   * Set light state for a node
   */
  setLight(command: SetLightCommand): Observable<any> {
    return this.post<any>('/set-light', command);
  }

  /**
   * Send color profile to a zone
   */
  sendColorProfile(command: ColorProfileCommand): Observable<any> {
    return this.post<any>('/color-profile', command);
  }

  /**
   * Approve or reject node pairing
   */
  approveNodePairing(approval: PairingApproval): Observable<any> {
    return this.post<any>('/pairing/approve', approval);
  }

  // ============================================================================
  // OTA API
  // ============================================================================

  /**
   * Start OTA firmware update
   */
  startOTAUpdate(request: StartOTARequest): Observable<OTAJob> {
    return this.post<OTAJob>('/ota/start', request);
  }

  /**
   * Get OTA job status
   */
  getOTAJobStatus(jobId: string): Observable<OTAJob> {
    return this.get<OTAJob>(`/ota/status?job_id=${jobId}`);
  }

  // ============================================================================
  // Google Home API
  // ============================================================================

  /**
   * Report device state to Google Home
   */
  reportGoogleHomeState(userId: string, deviceId: string, state: GoogleHomeState): Observable<any> {
    return this.post<any>('/google/home/report-state', {
      user_id: userId,
      device_id: deviceId,
      state
    });
  }

  /**
   * Request Google to sync devices
   */
  requestGoogleHomeSync(userId: string): Observable<any> {
    return this.post<any>('/google/home/request-sync', {
      user_id: userId
    });
  }

  // ============================================================================
  // mmWave API
  // ============================================================================

  /**
   * Retrieve mmWave history with optional filtering
   */
  getMmwaveHistory(params?: { siteId?: string; coordinatorId?: string; limit?: number }): Observable<MmwaveFrame[]> {
    const search = new URLSearchParams();

    if (params?.siteId) {
      search.set('site_id', params.siteId);
    }

    if (params?.coordinatorId) {
      search.set('coord_id', params.coordinatorId);
    }

    if (params?.limit && params.limit > 0) {
      search.set('limit', params.limit.toString());
    }

    const query = search.toString();
    const suffix = query ? `?${query}` : '';

    return this.get<MmwaveFrame[]>(`/mmwave/history${suffix}`);
  }

  // ============================================================================
  // Settings API
  // ============================================================================

  /**
   * Get settings for a site
   */
  getSettings(siteId: string): Observable<any> {
    return this.get<any>(`/api/v1/settings?site_id=${siteId}`);
  }

  /**
   * Save settings for a site
   */
  saveSettings(siteId: string, settings: any): Observable<any> {
    return this.put<any>(`/api/v1/settings`, settings);
  }

  // ============================================================================
  // Coordinator Control API
  // ============================================================================

  /**
   * Get coordinator details
   */
  getCoordinator(siteId: string, coordinatorId: string): Observable<any> {
    return this.get<any>(`/sites/${siteId}/coordinators/${coordinatorId}`);
  }

  /**
   * Start pairing mode
   */
  startPairing(siteId: string, coordinatorId: string, durationSeconds: number): Observable<any> {
    return this.post<any>(`/api/v1/coordinator/pair`, {
      site_id: siteId,
      coordinator_id: coordinatorId,
      duration_ms: durationSeconds * 1000
    });
  }

  /**
   * Restart coordinator
   */
  restartCoordinator(siteId: string, coordinatorId: string): Observable<any> {
    return this.post<any>(`/api/v1/coordinator/restart`, {
      site_id: siteId,
      coordinator_id: coordinatorId
    });
  }

  /**
   * Update WiFi configuration
   */
  updateWiFiConfig(siteId: string, coordinatorId: string, ssid: string, password: string): Observable<any> {
    return this.post<any>(`/api/v1/coordinator/wifi`, {
      site_id: siteId,
      coordinator_id: coordinatorId,
      ssid,
      password
    });
  }

  // ============================================================================
  // Node Management API
  // ============================================================================

  /**
   * Get all nodes for a coordinator
   */
  getNodes(siteId: string, coordinatorId: string): Observable<any[]> {
    return this.get<any[]>(`/sites/${siteId}/coordinators/${coordinatorId}/nodes`);
  }

  /**
   * Delete a node
   */
  deleteNode(siteId: string, coordinatorId: string, nodeId: string): Observable<any> {
    return this.delete<any>(`/sites/${siteId}/coordinators/${coordinatorId}/nodes/${nodeId}`);
  }

  /**
   * Update node zone
   */
  updateNodeZone(siteId: string, coordinatorId: string, nodeId: string, zoneId: string): Observable<any> {
    return this.put<any>(`/api/v1/node/zone`, {
      site_id: siteId,
      coordinator_id: coordinatorId,
      node_id: nodeId,
      zone_id: zoneId
    });
  }

  /**
   * Update node name
   */
  updateNodeName(siteId: string, coordinatorId: string, nodeId: string, name: string): Observable<any> {
    return this.put<any>(`/api/v1/node/name`, {
      site_id: siteId,
      coordinator_id: coordinatorId,
      node_id: nodeId,
      name
    });
  }

  /**
   * Send test color to node
   */
  sendNodeColor(siteId: string, coordinatorId: string, nodeId: string, r: number, g: number, b: number, w: number): Observable<any> {
    return this.post<any>(`/api/v1/node/test-color`, {
      site_id: siteId,
      coordinator_id: coordinatorId,
      node_id: nodeId,
      r, g, b, w
    });
  }

  /**
   * Turn off node LEDs
   */
  turnOffNode(siteId: string, coordinatorId: string, nodeId: string): Observable<any> {
    return this.post<any>(`/api/v1/node/off`, {
      site_id: siteId,
      coordinator_id: coordinatorId,
      node_id: nodeId
    });
  }

  /**
   * Set node brightness
   */
  setNodeBrightness(siteId: string, coordinatorId: string, nodeId: string, brightness: number): Observable<any> {
    return this.post<any>(`/api/v1/node/brightness`, {
      site_id: siteId,
      coordinator_id: coordinatorId,
      node_id: nodeId,
      brightness
    });
  }

  // ============================================================================
  // Google Home Integration API
  // ============================================================================

  /**
   * Disconnect Google Home
   */
  disconnectGoogleHome(siteId: string): Observable<any> {
    return this.post<any>(`/api/v1/google/disconnect`, {
      site_id: siteId
    });
  }

  // ============================================================================
  // System Settings API
  // ============================================================================

  /**
   * Get all system settings
   */
  getSystemSettings(): Observable<any> {
    return this.get<any>('/api/v1/system/settings');
  }

  /**
   * Update a specific section of system settings
   */
  updateSystemSettings(section: string, settings: any): Observable<any> {
    return this.put<any>('/api/v1/system/settings', {
      section,
      settings
    });
  }

  /**
   * Validate settings password
   */
  validateSettingsPassword(password: string): Observable<{ valid: boolean }> {
    return this.post<{ valid: boolean }>('/api/v1/system/settings/validate-password', {
      password
    });
  }

  /**
   * Reset settings to defaults
   */
  resetSettingsToDefaults(): Observable<any> {
    return this.post<any>('/api/v1/system/settings/reset', {});
  }

  // ============================================================================
  // Generic HTTP Methods
  // ============================================================================

  /**
   * Generic GET request
   */
  get<T>(endpoint: string): Observable<T> {
    return new Observable<T>(observer => {
      fetch(`${this.baseUrl}${endpoint}`, {
        method: 'GET',
        headers: this.getHeaders(),
        signal: AbortSignal.timeout(this.env.apiTimeout)
      })
        .then(response => {
          if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
          }
          return response.json();
        })
        .then(data => {
          observer.next(data as T);
          observer.complete();
        })
        .catch(error => {
          observer.error(this.handleError(error));
        });
    }).pipe(
      timeout(this.env.apiTimeout),
      catchError(err => throwError(() => this.handleError(err)))
    );
  }

  /**
   * Generic POST request
   */
  post<T>(endpoint: string, body: any): Observable<T> {
    return new Observable<T>(observer => {
      fetch(`${this.baseUrl}${endpoint}`, {
        method: 'POST',
        headers: this.getHeaders(),
        body: JSON.stringify(body),
        signal: AbortSignal.timeout(this.env.apiTimeout)
      })
        .then(response => {
          if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
          }
          return response.json();
        })
        .then(data => {
          observer.next(data as T);
          observer.complete();
        })
        .catch(error => {
          observer.error(this.handleError(error));
        });
    }).pipe(
      timeout(this.env.apiTimeout),
      catchError(err => throwError(() => this.handleError(err)))
    );
  }

  /**
   * Generic PUT request
   */
  put<T>(endpoint: string, body: any): Observable<T> {
    return new Observable<T>(observer => {
      fetch(`${this.baseUrl}${endpoint}`, {
        method: 'PUT',
        headers: this.getHeaders(),
        body: JSON.stringify(body),
        signal: AbortSignal.timeout(this.env.apiTimeout)
      })
        .then(response => {
          if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
          }
          return response.json();
        })
        .then(data => {
          observer.next(data as T);
          observer.complete();
        })
        .catch(error => {
          observer.error(this.handleError(error));
        });
    }).pipe(
      timeout(this.env.apiTimeout),
      catchError(err => throwError(() => this.handleError(err)))
    );
  }

  /**
   * Generic DELETE request
   */
  private delete<T>(endpoint: string): Observable<T> {
    return new Observable<T>(observer => {
      fetch(`${this.baseUrl}${endpoint}`, {
        method: 'DELETE',
        headers: this.getHeaders(),
        signal: AbortSignal.timeout(this.env.apiTimeout)
      })
        .then(response => {
          if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
          }
          return response.json();
        })
        .then(data => {
          observer.next(data as T);
          observer.complete();
        })
        .catch(error => {
          observer.error(this.handleError(error));
        });
    }).pipe(
      timeout(this.env.apiTimeout),
      catchError(err => throwError(() => this.handleError(err)))
    );
  }

  /**
   * Get default headers for requests
   */
  private getHeaders(): HeadersInit {
    const headers: HeadersInit = {
      'Content-Type': 'application/json',
      'Accept': 'application/json'
    };

    // Add auth token if available
    const token = this.getAuthToken();
    if (token) {
      headers['Authorization'] = `Bearer ${token}`;
    }

    return headers;
  }

  /**
   * Get auth token from storage
   */
  private getAuthToken(): string | null {
    if (typeof localStorage !== 'undefined') {
      return localStorage.getItem('auth_token');
    }
    return null;
  }

  /**
   * Handle HTTP errors
   */
  private handleError(error: any): Error {
    if (this.env.isDevelopment) {
      console.error('API Error:', error);
    }

    if (error.name === 'AbortError' || error.name === 'TimeoutError') {
      return new Error('Request timeout - please check your connection');
    }

    if (error.message) {
      return new Error(error.message);
    }

    return new Error('An unexpected error occurred');
  }
}
