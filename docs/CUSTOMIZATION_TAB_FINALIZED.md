# Customization Tab - Final Implementation

## Overview

The Customization Tab has been finalized to work with **actual configurable parameters** from the coordinator and nodes, as defined in `ConfigManager.h` and persisted in NVS (Non-Volatile Storage).

## Architecture

### What's Configurable

The system uses `ConfigManager` (shared/src/ConfigManager.h) which defines specific configuration keys stored in ESP32 NVS:

#### **Coordinator Parameters** (ConfigKeys namespace)
- `presence_debounce_ms` - Debounce time for presence detection (default: 150ms)
- `occupancy_hold_ms` - How long to hold occupancy state (default: 5000ms)
- `fade_in_ms` - Fade-in duration for lights (default: 150ms)
- `fade_out_ms` - Fade-out duration for lights (default: 1000ms)
- `pairing_window_s` - Pairing window duration (default: 120s)

#### **Node Parameters** (ConfigKeys namespace)
- `pwm_freq_hz` - PWM frequency for LEDs (default: 1000Hz)
- `pwm_res_bits` - PWM resolution (default: 12 bits)
- `telemetry_s` - Telemetry interval (default: 5s)
- `rx_window_ms` - ESP-NOW receive window (default: 20ms)
- `rx_period_ms` - ESP-NOW receive period (default: 100ms)
- `derate_start_c` - Temperature to start derating (default: 70°C)
- `derate_min_duty_pct` - Minimum duty cycle when derating (default: 30%)
- `retry_count` - Command retry count (default: 3)
- `cmd_ttl_ms` - Command TTL (default: 1500ms)

#### **Node LED Configuration**
- `enabled` - LED enable/disable
- `brightness` - LED brightness (0-100%)
- `color` - LED color (hex format)

### What's NOT Configurable

The following are **managed by hardware classes** and not user-configurable:

- **MmWave Radar** (MmWave.h) - Detection zones, sensitivity, modes are hardcoded constants in firmware
- **Light Sensor** (AmbientLightSensor.h) - Uses TSL2561 library defaults, reads lux values only
- **LED Hardware** - Nodes use 4-pixel SK6812B strips as status indicators, not full addressable control

## Implementation

### Backend API Endpoints

File: `IOT-Backend-main/IOT-Backend-main/internal/http/customize_handlers.go`

```
GET  /api/v1/{type}/{id}/customize           - Get configuration
PUT  /api/v1/{type}/{id}/customize/config    - Update coordinator/node config
PUT  /api/v1/{type}/{id}/customize/led       - Update LED config (nodes only)
POST /api/v1/{type}/{id}/customize/reset     - Reset to defaults
POST /api/v1/{type}/{id}/led/preview         - LED preview
```

Where `{type}` is `coordinator` or `node`, and `{id}` is the device ID.

### MQTT Commands

The backend publishes MQTT commands to:
```
site/{siteId}/coord/{coordId}/cmd
site/{siteId}/node/{nodeId}/cmd
```

Command payload format:
```json
{
  "cmd": "update_config",
  "config": {
    "presence_debounce_ms": 150,
    "occupancy_hold_ms": 5000,
    ...
  }
}
```

### Frontend Component

File: `IOT-Frontend-main/IOT-Frontend-main/src/app/features/dashboard/tabs/customize.component.ts`

The component:
1. Fetches configuration from backend API
2. Displays configurable parameters based on device type
3. Saves changes via API which publishes MQTT commands
4. Shows read-only status for radar/light sensor

### Configuration Response Format

```json
{
  "coordinator": {
    "presence_debounce_ms": 150,
    "occupancy_hold_ms": 5000,
    "fade_in_ms": 150,
    "fade_out_ms": 1000,
    "pairing_window_s": 120
  },
  "radar": {
    "enabled": true,
    "online": false
  },
  "light": {
    "enabled": true
  }
}
```

For nodes:
```json
{
  "node": {
    "pwm_freq_hz": 1000,
    "pwm_res_bits": 12,
    "telemetry_s": 5,
    "rx_window_ms": 20,
    "rx_period_ms": 100,
    "derate_start_c": 70.0,
    "derate_min_duty_pct": 30,
    "retry_count": 3,
    "cmd_ttl_ms": 1500
  },
  "led": {
    "enabled": true,
    "brightness": 80,
    "color": "#00FFBF"
  }
}
```

## Coordinator Firmware Integration (Required)

The coordinator needs to handle the `update_config` command in its MQTT subscription handler:

### In `Coordinator::handleMqttCommand` (Coordinator.cpp)

Add handling for the `update_config` command:

```cpp
void Coordinator::handleMqttCommand(const String& topic, const String& payload) {
    // ... existing code ...
    
    if (cmd == "update_config") {
        // Parse config object
        JsonObject configObj = doc["config"];
        if (!configObj.isNull()) {
            // Get or create ConfigManager instance
            ConfigManager config("coordinator"); // or "node" depending on device
            if (!config.begin()) {
                Logger::error("Failed to open config namespace");
                return;
            }
            
            // Update each key from the config object
            for (JsonPair kv : configObj) {
                String key = kv.key().c_str();
                
                if (kv.value().is<int>()) {
                    config.setInt(key, kv.value().as<int>());
                } else if (kv.value().is<float>()) {
                    config.setFloat(key, kv.value().as<float>());
                } else if (kv.value().is<bool>()) {
                    config.setBool(key, kv.value().as<bool>());
                } else if (kv.value().is<const char*>()) {
                    config.setString(key, kv.value().as<String>());
                }
                
                Logger::info("Updated config key: %s", key.c_str());
            }
            
            config.end();
            publishLog("Configuration updated successfully", "INFO", "config");
        }
    }
    
    // ... existing code ...
}
```

## Testing

### 1. Backend Testing
```bash
cd IOT-Backend-main/IOT-Backend-main
go run cmd/iot/main.go
```

Test API endpoint:
```bash
curl http://localhost:8000/api/v1/coordinator/coord001/customize
```

### 2. Frontend Testing
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm install
npm start
```

Navigate to `http://localhost:4200` → Dashboard → Customize Tab

### 3. Integration Testing

1. Select a coordinator or node
2. Modify a configuration value (e.g., presence_debounce_ms: 200)
3. Click "Save"
4. Check MQTT broker for published command
5. Verify coordinator receives and applies configuration
6. Check NVS to confirm persistence

## Files Modified

### Backend
- `internal/http/customize_handlers.go` - New file with customization endpoints
- `internal/http/handlers.go` - Added route registration

### Frontend
- `tabs/customize.component.ts` - Updated with real configurable parameters
- Simplified from overly-detailed hardware specs to actual ConfigManager keys

## Configuration Flow

```
Frontend UI
    ↓
  API Call (PUT /api/v1/{type}/{id}/customize/config)
    ↓
Backend Handler
    ↓
  MQTT Publish (site/{siteId}/{type}/{id}/cmd)
    ↓
Coordinator/Node MQTT Handler
    ↓
ConfigManager.setInt/setFloat/setBool/setString
    ↓
  NVS Storage (persisted across reboots)
```

## Key Differences from Original Design

1. **No hardware-specific configuration UI**: MmWave radar zones, TSL2561 integration time, etc. are firmware constants, not user-configurable
2. **Focus on ConfigManager keys**: Only expose what's actually stored in NVS and used by firmware
3. **Status indicators**: Radar and light sensor status are read-only displays, not configuration
4. **Simple LED control**: Node LEDs are 4-pixel status indicators, not full addressable strips with effects

## References

- **ConfigManager**: `shared/src/ConfigManager.h` and `ConfigManager.cpp`
- **Coordinator**: `coordinator/src/core/Coordinator.cpp`
- **Product Requirements**: `docs/ProductRequirementDocument.md`
- **MQTT API**: `docs/mqtt_api.md`

## Status

✅ Backend API implemented
✅ Frontend component updated
✅ Configuration structure aligned with firmware
⏳ Coordinator MQTT handler needs update (see "Coordinator Firmware Integration" section above)

---

**Date**: 2025-01-26
**Status**: Ready for coordinator firmware integration
