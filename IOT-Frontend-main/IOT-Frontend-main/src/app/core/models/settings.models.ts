/**
 * Settings Models
 * Type-safe interfaces for system-wide configuration
 */

export interface SystemSettings {
  // System Identity
  system_identity: SystemIdentitySettings;
  
  // Network & Connectivity
  network: NetworkSettings;
  
  // MQTT Behavior
  mqtt: MqttBehaviorSettings;
  
  // Coordinator Configuration
  coordinator: CoordinatorSettings;
  
  // Node Defaults
  node: NodeDefaultSettings;
  
  // Zones
  zones: ZoneSettings;
  
  // Energy & Automation
  automation: AutomationSettings;
  
  // Telemetry & Logging
  telemetry: TelemetrySettings;
  
  // Developer Tools
  developer: DeveloperSettings;
}

export interface SystemIdentitySettings {
  default_site_id: string;
  default_coord_id: string;
  max_coordinators_per_site: number;
  max_nodes_per_coordinator: number;
}

export interface NetworkSettings {
  esp32_wifi_ssid: string;
  esp32_wifi_password: string;
  esp32_wifi_timeout: number;
  esp32_reconnect_delay: number;
  esp32_ping_interval: number;
  api_url: string; // read-only
  ws_url: string;  // read-only
}

export interface MqttBehaviorSettings {
  mqtt_qos: 0 | 1 | 2;
  mqtt_keepalive: number;
  mqtt_client_id: string;
}

export interface CoordinatorSettings {
  esp32_firmware_version: string; // read-only
  esp32_ota_url: string;
  coord_led_count: number;
  coord_button_hold_time: number;
  coord_max_nodes: number;
}

export interface NodeDefaultSettings {
  node_led_count: number;
  node_default_brightness: number;
  node_temp_sensor_enabled: boolean;
  node_button_enabled: boolean;
}

export interface ZoneSettings {
  default_zones: string[];
  allow_custom_zones: boolean;
  max_zones: number;
}

export interface AutomationSettings {
  energy_saving_mode: boolean;
  auto_off_delay: number;
  motion_sensitivity: number;
  default_light_intensity: number;
}

export interface TelemetrySettings {
  telemetry_retention_days: number;
  debug_mode: boolean;
  metrics_interval: number;
  log_to_file: boolean;
}

export interface DeveloperSettings {
  mock_devices_enabled: boolean;
  simulator_enabled: boolean;
  simulator_interval: number;
}

/**
 * Response model when getting settings
 */
export interface SettingsResponse {
  success: boolean;
  data: SystemSettings;
  message?: string;
}

/**
 * Request model when updating settings
 */
export interface UpdateSettingsRequest {
  section: keyof SystemSettings;
  settings: Partial<SystemSettings[keyof SystemSettings]>;
}

/**
 * WebSocket/MQTT message for settings updates
 */
export interface SettingsUpdateMessage {
  type: 'settings_update';
  section: keyof SystemSettings;
  settings: any;
  timestamp: string;
}
