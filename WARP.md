# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

**IOT-TileNodeCoordinator** is a smart indoor lighting system with:
- **Coordinator** (ESP32-S3): Central hub managing mmWave presence detection, ESP-NOW control plane, and MQTT bridge to backend
- **Light Nodes** (ESP32-C3): Battery-powered devices driving SK6812B RGBW LEDs, temperature sensing, and status indication
- **Backend**: Configuration, telemetry, and OTA management via MQTT

See `docs/ProductRequirementDocument.md` for full spec.

## Build Commands

### Coordinator (ESP32-S3)
```bash
# Build
cd coordinator && pio run -e esp32-s3-devkitc-1

# Upload to device
pio run -e esp32-s3-devkitc-1 -t upload

# Monitor serial output
pio run -e esp32-s3-devkitc-1 -t monitor

# Clean build
pio run -e esp32-s3-devkitc-1 -t clean
```

### Light Node (ESP32-C3)
```bash
# Build
cd node && pio run -e esp32-c3-mini-1-debug

# Upload to device
pio run -e esp32-c3-mini-1-debug -t upload

# Monitor serial output
pio run -e esp32-c3-mini-1-debug -t monitor
```

### Shared Library
Located in `shared/src/`. Automatically included in both coordinator and node builds via `lib_extra_dirs`.

## Architecture

### Coordinator (`coordinator/src/`)

**Core**: `core/Coordinator.*` - Main coordinator orchestrating all subsystems via event handlers
- **Initialization Chain**: ESP-NOW → MQTT → mmWave → NodeRegistry → ZoneControl → ThermalControl → Buttons
- **Event Flow**: mmWave detection → zone mapping → light commands → MQTT telemetry

**Communication Layer** (`comm/`):
- **EspNow.cpp**: Manages ESP-NOW encrypted unicast with peer persistence; handles pairing mode
- **Mqtt.cpp**: WiFi/MQTT bridge; subscribes to backend commands, publishes telemetry

**Subsystems**:
- **NodeRegistry** (`nodes/`): Tracks paired nodes with MAC-to-ID mapping, timeout detection, and NVS persistence
- **ZoneControl** (`zones/`): Maps zone IDs to light IDs; applies presence logic
- **MmWave** (`sensors/`): Parses mmWave sensor events (zone/presence/confidence)
- **ThermalControl** (`sensors/`): Processes node temperature telemetry; applies deration algorithm (100% → 30% brightness over 70–85°C)
- **ButtonControl** (`input/`): Local button presses for manual override/pairing trigger
- **StatusLed** (`utils/`): Onboard LED status indication

**Key Files**:
- `src/Models.h`: Shared data structures (NodeInfo, ZoneMapping, MmWaveEvent, ThermalEvent)
- `src/Logger.h`: Centralized logging (uses Serial for output)
- `src/config/`: Configuration defaults and constants

### Light Node (`node/src/`)

**LED Control** (`led/`): SK6812B RMT driver; supports RGBW patterns (uniform, indexed, gradient) and status modes
**Sensors** (`sensors/temperature/`): SPI temperature reading; local deration logic
**Power** (`power/`): Light sleep scheduling; jittered telemetry intervals
**Communication** (`..` shared library): Handles ESP-NOW protocol with acks/retries
**OTA** (`utils/OtaUpdater.cpp`): Dual-slot firmware update with rollback safety

### Shared Protocol (`shared/src/`)

**EspNowMessage.cpp/h**: Message schema factory supporting:
- `JoinRequestMessage`: Node announce with capabilities (RGBW, LED count, temp sensor, deep sleep)
- `JoinAcceptMessage`: Coordinator approval with node ID and LMK
- `SetLightMessage`: RGBW command with fade, override flags, TTL, reason field
- `NodeStatusMessage`: Telemetry (RGBW avg, status mode, voltage, firmware)
- `ErrorMessage`, `AckMessage`

## Key Concepts

### Pairing Flow
1. Hold node button → node enters pairing mode, broadcasts join_request
2. Coordinator detects request, user presses coordinator button
3. Coordinator sends join_accept with node_id + light_id
4. Node stores in NVS; both transition to operational
5. Stored peers loaded from NVS at boot

### Command Idempotency
- Each command has unique `cmd_id` (millis + MAC suffix)
- Prevents duplicate actions on retransmission

### Thermal Deration
Coordinator monitors node temperature:
- 70°C: 100% brightness
- 77.5°C: 65% brightness (linear interpolation)
- 85°C: 30% brightness (white channel reduced first to preserve color)

### NVS Storage
- **Coordinator**: Peers list (MAC strings), zone config in namespace "peers"
- **Node**: Pairing key (node_id, light_id, LMK), power config in namespace "config"

### MQTT Topics
Base: `smart_tile/`
- State: `nodes/{node_id}/state`, `zones/{zone_id}/presence`, `system/status`
- Commands: `nodes/{node_id}/cmd`, `zones/{zone_id}/cmd`
- Discovery: `discovery/nodes`, `discovery/zones`

See `docs/mqtt_api.md` for full payload schemas.

## Memory Safety & Recent Fixes

See `CODE_OPTIMIZATION_REPORT.md` for detailed issue history. Key fixes applied:
- **Null checks in destructors** and event handlers prevent crashes on failed initialization
- **Input validation** in ESP-NOW callbacks guards against malformed packets
- **JSON type safety**: Use `doc.containsKey()` before accessing; avoid bitwise ops as type coercion
- **NVS flash wear**: Only erase on corruption, not every boot
- **Stale node detection**: Skip nodes with `lastSeenMs == 0` to prevent false removal

## Debugging

### Serial Logging
All components log to Serial at 115200 baud. Use monitor commands above.
- Coordinator boot logs NVS init status, peer count, MQTT connection
- Node logs pairing handshake, LED state changes, temperature readings

### Key Log Markers
- `*** BOOT START ***` / `*** SETUP COMPLETE ***`: Initialization checkpoints
- `✗` prefix: Error conditions
- `✓` prefix: Success confirmations

### Common Issues
- **NVS errors**: Clear partition with `nvs_flash_erase()` only if corrupted; check `code/complete_nvs_fix.md` history
- **ESP-NOW peers not persistent**: Verify `loadPeersFromStorage()` called during `Coordinator::begin()`
- **LED not responding**: Ensure RMT pin configured correctly; check power supply for voltage sag on 4 LEDs

## Dependencies

**Coordinator**:
- `espressif32 @ ^6.5.0` (ESP32-S3 board support)
- `ArduinoJson @ ^6.21.3` (message serialization)
- `PubSubClient @ ^2.8.0` (MQTT)
- `Adafruit NeoPixel @ ^1.10.6` (LED control, used as reference)

**Node**:
- `espressif32 @ ^6.5.0` (ESP32-C3 board support)
- `ArduinoJson @ ^6.21.5`
- `Adafruit NeoPixel @ ^1.15.2`
- `ESP32Time @ ^2.0.6`

Shared library in `../shared` linked via `lib_extra_dirs`.

## Code Patterns & Guidelines

### Pointers & Memory
- Raw pointers used with manual `new`/`delete` (refactoring to smart pointers recommended)
- Always check for null in destructors and callbacks
- Avoid double-deletion; set to `nullptr` after delete

### Strings
- Pass by const reference in loops: `const String& nodeId`
- Use `snprintf` for single string building (faster than `String` concatenation)

### JSON Parsing
```cpp
// Correct
if (doc.containsKey("brightness")) {
    brightness = doc["brightness"].as<uint8_t>();
}

// WRONG: bitwise ops don't work as type coercion
brightness = doc["brightness"].as<uint8_t>() || 0;  // Wrong!
```

### Vector Pre-allocation
```cpp
std::vector<String> items;
items.reserve(4);  // Typical size to reduce allocations
```

### Configuration Defaults
Update `src/config/` constants for timing, thresholds, or retry logic; avoid magic numbers in code.

## Testing Notes

- Manual pairing flow validation on hardware with button interaction
- ESP-NOW range testing at 10m line-of-sight (target ≥98% delivery)
- MQTT subscription/publish via external broker (e.g., Mosquitto)
- Thermal deration by heating node (e.g., heat gun) and verifying brightness reduction
- NVS corruption recovery: trigger with `nvs_flash_erase()` before init and verify graceful boot

## Related Documentation

- `docs/mqtt_api.md` — MQTT topic structure and payload schemas
- `docs/ProductRequirementDocument.md` — Full specification (FR/NFR, state machines, timing targets)
- `CODE_OPTIMIZATION_REPORT.md` — Memory safety issues and architectural recommendations
- `ESP_NOW_V2_CHECKLIST.md` & `ESP_NOW_V2_CONVERSION_COMPLETE.md` — Migration history for ESP-NOW 2.0 API
