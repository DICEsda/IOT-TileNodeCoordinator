# Settings Tab Backend Requirements

This document outlines the backend REST API endpoints, MQTT topics, and MongoDB schemas required to support the new Settings tab implementation.

## Overview

The Settings tab manages system-wide environment configuration with:
- Password protection (password: "1234")
- Real-time updates via WebSocket/MQTT
- Persistent storage in MongoDB
- Dynamic support for multiple coordinators and nodes

## REST API Endpoints

### 1. Get System Settings
```
GET /api/v1/system/settings
Response: {
  success: boolean,
  data: SystemSettings,
  message?: string
}
```

### 2. Update System Settings
```
PUT /api/v1/system/settings
Request: {
  section: string, // "system_identity" | "network" | "mqtt" | etc.
  settings: object // Section-specific settings
}
Response: {
  success: boolean,
  message: string
}
```

### 3. Validate Settings Password
```
POST /api/v1/system/settings/validate-password
Request: {
  password: string
}
Response: {
  valid: boolean
}
```

### 4. Reset Settings to Defaults
```
POST /api/v1/system/settings/reset
Response: {
  success: boolean,
  message: string
}
```

## MongoDB Schema

### Collection: `system_settings`

```javascript
{
  _id: ObjectId,
  updated_at: Date,
  
  // System Identity
  system_identity: {
    default_site_id: String,        // "site001"
    default_coord_id: String,       // "coord001"
    max_coordinators_per_site: Number,  // 10
    max_nodes_per_coordinator: Number   // 250
  },
  
  // Network & Connectivity
  network: {
    esp32_wifi_ssid: String,
    esp32_wifi_password: String,    // Encrypted
    esp32_wifi_timeout: Number,     // milliseconds
    esp32_reconnect_delay: Number,  // milliseconds
    esp32_ping_interval: Number,    // milliseconds
    api_url: String,                // Read-only, from env
    ws_url: String                  // Read-only, from env
  },
  
  // MQTT Behavior
  mqtt: {
    mqtt_qos: Number,               // 0, 1, or 2
    mqtt_keepalive: Number,         // seconds
    mqtt_client_id: String
  },
  
  // Coordinator Configuration
  coordinator: {
    esp32_firmware_version: String, // Read-only
    esp32_ota_url: String,
    coord_led_count: Number,
    coord_button_hold_time: Number, // milliseconds
    coord_max_nodes: Number
  },
  
  // Node Defaults
  node: {
    node_led_count: Number,
    node_default_brightness: Number, // 0-255
    node_temp_sensor_enabled: Boolean,
    node_button_enabled: Boolean
  },
  
  // Zones
  zones: {
    default_zones: [String],
    allow_custom_zones: Boolean,
    max_zones: Number
  },
  
  // Energy & Automation
  automation: {
    energy_saving_mode: Boolean,
    auto_off_delay: Number,         // seconds
    motion_sensitivity: Number,     // 0-100
    default_light_intensity: Number // 0-100
  },
  
  // Telemetry & Logging
  telemetry: {
    telemetry_retention_days: Number,
    debug_mode: Boolean,
    metrics_interval: Number,       // seconds
    log_to_file: Boolean
  },
  
  // Developer Tools
  developer: {
    mock_devices_enabled: Boolean,
    simulator_enabled: Boolean,
    simulator_interval: Number      // milliseconds
  }
}
```

## MQTT Topics

### Settings Updates (Backend → Frontend)
```
Topic: system/settings/update
Payload: {
  type: "settings_update",
  section: "network" | "mqtt" | etc.,
  settings: {...},
  timestamp: "2024-01-01T00:00:00Z"
}
```

### Settings Applied to Devices (Backend → Coordinators/Nodes)
When settings change, backend should publish relevant updates to devices:

```
Topic: site/{siteId}/coord/{coordId}/config
Payload: {
  wifi_ssid: string,
  wifi_timeout: number,
  reconnect_delay: number,
  ping_interval: number,
  led_count: number,
  button_hold_time: number,
  max_nodes: number,
  mqtt_qos: number,
  mqtt_keepalive: number
}
```

```
Topic: site/{siteId}/node/{nodeId}/config
Payload: {
  led_count: number,
  default_brightness: number,
  temp_sensor_enabled: boolean,
  button_enabled: boolean
}
```

## WebSocket Messages

The backend WebSocket server should forward MQTT settings updates to connected frontend clients:

```javascript
{
  type: "settings_update",
  section: "network",
  settings: {...},
  timestamp: "2024-01-01T00:00:00Z"
}
```

## Default Values

```javascript
const DEFAULT_SETTINGS = {
  system_identity: {
    default_site_id: "site001",
    default_coord_id: "coord001",
    max_coordinators_per_site: 10,
    max_nodes_per_coordinator: 250
  },
  network: {
    esp32_wifi_ssid: "",
    esp32_wifi_password: "",
    esp32_wifi_timeout: 10000,
    esp32_reconnect_delay: 5000,
    esp32_ping_interval: 30000,
    api_url: process.env.API_URL || "http://localhost:3000",
    ws_url: process.env.WS_URL || "ws://localhost:3000/ws"
  },
  mqtt: {
    mqtt_qos: 1,
    mqtt_keepalive: 60,
    mqtt_client_id: "smartlight-backend"
  },
  coordinator: {
    esp32_firmware_version: "1.0.0",
    esp32_ota_url: "",
    coord_led_count: 12,
    coord_button_hold_time: 3000,
    coord_max_nodes: 250
  },
  node: {
    node_led_count: 60,
    node_default_brightness: 128,
    node_temp_sensor_enabled: true,
    node_button_enabled: true
  },
  zones: {
    default_zones: ["Living Room", "Bedroom", "Kitchen", "Bathroom", "Office", "Hallway"],
    allow_custom_zones: true,
    max_zones: 50
  },
  automation: {
    energy_saving_mode: false,
    auto_off_delay: 30,
    motion_sensitivity: 50,
    default_light_intensity: 80
  },
  telemetry: {
    telemetry_retention_days: 30,
    debug_mode: false,
    metrics_interval: 60,
    log_to_file: true
  },
  developer: {
    mock_devices_enabled: false,
    simulator_enabled: false,
    simulator_interval: 1000
  }
};
```

## Security Considerations

1. **Password Protection**: The password "1234" should be stored as an environment variable `SETTINGS_PASSWORD` and hashed
2. **WiFi Password**: ESP32 WiFi password should be encrypted in the database
3. **API Keys**: No sensitive API keys or secrets should be exposed through this interface
4. **Rate Limiting**: Implement rate limiting on settings update endpoints

## Implementation Steps

1. Create MongoDB model for `system_settings`
2. Implement REST endpoints in backend
3. Add password validation middleware
4. Implement MQTT publisher for settings updates
5. Update WebSocket server to forward settings updates
6. Add settings change handlers to update ESP32 devices
7. Implement settings initialization on backend startup

## Frontend Integration

The frontend components are already created:
- `settings.models.ts`: TypeScript interfaces
- `settings-new.component.ts`: Main Settings component with password protection
- `settings-new.component.html`: Comprehensive UI for all 9 sections
- `api.service.ts`: Updated with settings endpoints

## Testing Checklist

- [ ] Password validation works correctly
- [ ] Settings persist to MongoDB
- [ ] Real-time updates via WebSocket work
- [ ] MQTT messages are published to devices
- [ ] Device configuration updates apply correctly
- [ ] Read-only fields cannot be modified
- [ ] Developer mode toggle shows/hides developer section
- [ ] Zone management (add/remove/edit) works
- [ ] Reset to defaults restores all values
- [ ] Individual section saving works
- [ ] Bulk "Save All" works correctly
