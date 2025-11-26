/**
 * API Models for IoT Tile System
 * Defines TypeScript interfaces matching backend data structures
 */

// ============================================================================
// Site Models
// ============================================================================

export interface Site {
  _id: string;
  name: string;
  location?: string;
  coordinators: string[];
  zones: Zone[];
  created_at: Date;
  updated_at: Date;
}

export interface Zone {
  _id?: string;
  name: string;
  site_id: string;
  coordinator_id: string;
  created_at?: Date;
  updated_at?: Date;
}

// ============================================================================
// Coordinator Models
// ============================================================================

export interface Coordinator {
  _id: string;
  coord_id: string;
  site_id: string;
  mac_address: string;
  wifi_ssid?: string;
  wifi_rssi?: number;
  mqtt_broker?: string;
  firmware_version?: string;
  status: 'online' | 'offline' | 'error';
  uptime?: number;
  heap_free?: number;
  light_lux?: number;
  temp_c?: number;
  last_seen: Date;
  created_at: Date;
  updated_at: Date;
}

export interface CoordinatorTelemetry {
  coord_id: string;
  site_id: string;
  uptime: number;
  heap_free: number;
  wifi_rssi: number;
  mqtt_connected: boolean;
  light_lux?: number;
  temp_c?: number;
  timestamp: Date;
}

// ============================================================================
// Node Models
// ============================================================================

export interface Node {
  _id: string;
  node_id: string;
  name?: string;
  site_id: string;
  zone_id?: string;
  mac_address: string;
  paired: boolean;
  status: 'online' | 'offline' | 'error' | 'pairing';
  
  // Light state
  rgbw?: RGBWState;
  brightness?: number;
  
  // Sensor data
  temperature?: number;
  battery_voltage?: number;
  battery_percent?: number;
  
  // Metadata
  firmware_version?: string;
  last_seen: Date;
  created_at: Date;
  updated_at: Date;
}

export interface RGBWState {
  r: number;
  g: number;
  b: number;
  w: number;
}

export interface NodeTelemetry {
  node_id: string;
  site_id: string;
  rgbw: RGBWState;
  temperature: number;
  battery_voltage: number;
  battery_percent: number;
  timestamp: Date;
}

// ============================================================================
// Command Models
// ============================================================================

export interface SetLightCommand {
  node_id: string;
  site_id: string;
  rgbw?: RGBWState;
  brightness?: number;
  fade_duration?: number;
}

export interface ColorProfileCommand {
  zone_id: string;
  site_id: string;
  profile: 'warm' | 'cool' | 'daylight' | 'custom';
  rgbw?: RGBWState;
}

export interface PairingApproval {
  node_id: string;
  site_id: string;
  zone_id?: string;
  approve: boolean;
}

// ============================================================================
// OTA Models
// ============================================================================

export interface OTAJob {
  _id: string;
  job_id: string;
  target_type: 'coordinator' | 'node';
  target_id: string;
  firmware_url: string;
  version: string;
  status: 'pending' | 'in_progress' | 'completed' | 'failed';
  progress?: number;
  error?: string;
  created_at: Date;
  updated_at: Date;
}

export interface StartOTARequest {
  target_type: 'coordinator' | 'node';
  target_id: string;
  firmware_url: string;
  version: string;
}

// ============================================================================
// Presence Models
// ============================================================================

export interface PresenceEvent {
  zone_id: string;
  site_id: string;
  presence: boolean;
  distance?: number;
  timestamp: Date;
}

export interface MmwaveTarget {
  id: number;
  distance_mm: number;
  speed_cm_s: number;
  resolution_mm: number;
  position_x_mm: number;
  position_y_mm: number;
  velocity_x_m_s: number;
  velocity_y_m_s: number;
}

export interface MmwaveFrame {
  id?: string;
  site_id: string;
  coordinator_id: string;
  sensor_id: string;
  presence: boolean;
  confidence: number;
  targets: MmwaveTarget[];
  timestamp: Date;
}

// ============================================================================
// Health Models
// ============================================================================

export interface HealthStatus {
  status: 'healthy' | 'degraded' | 'unhealthy';
  service: string;
  database?: boolean;
  mqtt?: boolean;
  coordinator?: boolean;
  timestamp?: number;
  checks?: {
    mongodb?: boolean;
    mqtt?: boolean;
    google_home?: boolean;
  };
}

// ============================================================================
// WebSocket Message Models
// ============================================================================

export interface WSMessage {
  type: 'telemetry' | 'presence' | 'status' | 'pairing' | 'command_ack' | 'error';
  payload: any;
  timestamp: Date;
}

export interface WSTelemetryMessage extends WSMessage {
  type: 'telemetry';
  payload: NodeTelemetry | CoordinatorTelemetry;
}

export interface WSPresenceMessage extends WSMessage {
  type: 'presence';
  payload: PresenceEvent;
}

export interface WSStatusMessage extends WSMessage {
  type: 'status';
  payload: {
    entity_id: string;
    entity_type: 'node' | 'coordinator';
    status: 'online' | 'offline' | 'error';
  };
}

export interface WSPairingMessage extends WSMessage {
  type: 'pairing';
  payload: {
    node_id: string;
    mac_address: string;
    status: 'requesting' | 'approved' | 'rejected';
  };
}

// ============================================================================
// API Response Models
// ============================================================================

export interface ApiResponse<T> {
  success: boolean;
  data?: T;
  error?: string;
  timestamp: Date;
}

export interface PaginatedResponse<T> {
  items: T[];
  total: number;
  page: number;
  page_size: number;
  has_more: boolean;
}

// ============================================================================
// Google Home Models
// ============================================================================

export interface GoogleHomeDevice {
  id: string;
  type: string;
  traits: string[];
  name: {
    name: string;
    nicknames?: string[];
  };
  willReportState: boolean;
  roomHint?: string;
  deviceInfo?: {
    manufacturer: string;
    model: string;
    hwVersion?: string;
    swVersion?: string;
  };
}

export interface GoogleHomeState {
  online: boolean;
  on?: boolean;
  brightness?: number;
  color?: {
    spectrumRgb: number;
  };
}
