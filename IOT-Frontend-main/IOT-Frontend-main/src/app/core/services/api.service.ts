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
  GoogleHomeState
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
  // Generic HTTP Methods
  // ============================================================================

  /**
   * Generic GET request
   */
  private get<T>(endpoint: string): Observable<T> {
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
  private post<T>(endpoint: string, body: any): Observable<T> {
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
  private put<T>(endpoint: string, body: any): Observable<T> {
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
