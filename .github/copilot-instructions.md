go run cmd/iot/main.go
# AI Coding Agent Instructions for IOT-TileNodeCoordinator

Purpose: keep the ESP32 firmware, Go backend, and Angular UI moving fast without breaking the tight real-time lighting loop. Favor incremental fixes, follow existing manager patterns, and document any workflow changes.

## Architecture snapshot
- Coordinator (ESP32-S3, PlatformIO/Arduino) orchestrates ESP-NOW nodes, MQTT uplink, Wi-Fi setup, and serial diagnostics; entry point lives in `coordinator/src/core/Coordinator.*` (ignore `SmartTileCoordinator.*`).
- Nodes (ESP32-C3) share code via `shared/` and talk ESP-NOW only; backend (`IOT-Backend-main`) and Angular frontend (`IOT-Frontend-main`) consume/publish MQTT + WebSockets.
- Data path: Node telemetry → ESP-NOW → Coordinator → MQTT (`site/{siteId}/coord|node/...`) → backend/front-end; frontend commands → MQTT `/cmd` → Coordinator → ESP-NOW downlink.

## Coordinator subsystems
- Startup (`src/main.cpp`): Serial banner → `Logger::begin` (single init) → NVS init (erase-on-error) → `coordinator.begin()` → tight loop with `delay(1)`.
- Subsystems follow the `*Manager` convention (`EspNow`, `Mqtt`, `WifiManager`, `NodeRegistry`, `ZoneControl`, `ThermalControl`, `ButtonControl`, `MmWave`, etc.); each allocates in `Coordinator::begin()` and exposes `loop()` ticks.
- `WifiManager` (new) handles stored credentials + interactive serial provisioning (“No Wi-Fi… Configure? y/n”), reconnect backoff, and offline mode. Always call `wifi->loop()` and rely on it before touching MQTT.
- `AmbientLightSensor` samples the analog divider on `Pins::External::AMBIENT_LIGHT_ADC` so coordinator can publish “light” telemetry even without nodes.
- `Mqtt` now depends on `WifiManager`, loads broker/site IDs from ConfigManager `"mqtt"`, publishes `CoordinatorSensorSnapshot`, node status frames, mmWave events, and listens for coordinator `/cmd` payloads.

## Pairing & telemetry conventions
- Coordinator boots in normal mode; pairing opens only via touch button short-press or MQTT command (`{"cmd":"pair","duration_ms":60000}`). `startPairingWindow()` ties `NodeRegistry::startPairing` with `EspNow::enablePairingMode`, pulses the status LED blue, and logs over Serial (`PAIRING MODE ...`).
- ESP-NOW pairing callback stores nodes in NVS (`NodeRegistry` namespace `nodes`), adds peers, flashes addressable LEDs (group of four pixels per node), and auto-closes the window.
- Serial output must keep the operator informed: `COORDINATOR DATA: light=..., temp=..., mmwave=...`, `STATUS: wifi=..., mqtt=..., pairing=...`, and one `FETCHED NODE [MAC] DATA: ...` line per fresh telemetry (see `Coordinator::printSerialTelemetry`).
- MQTT topology (see `docs/mqtt_api.md` + `docs/development/MQTT_TOPIC_ALIGNMENT_COMPLETE.md`):
	- `site/{siteId}/coord/{coordId}/telemetry` → coordinator light/temp/mmWave/wifi state.
	- `site/{siteId}/coord/{coordId}/mmwave` → mmWave frames (legacy-compatible payload).
	- `site/{siteId}/node/{nodeId}/telemetry` → NodeStatus mirror (avg RGBW, temperature, button, voltage).
	- `.../cmd` topics deliver downlink commands (pairing, light overrides). Keep schema additive.

## Developer workflows
- Build & flash coordinator: `cd coordinator && pio run -e esp32-s3-devkitc-1 -t upload -t monitor`.
- Build nodes: `cd node && pio run -e esp32-c3-mini-1` (use `-debug` env for verbose logging).
- Backend: `cd IOT-Backend-main/IOT-Backend-main && go run cmd/iot/main.go`; Frontend uses standard Angular CLI (`npm install && npm start`).
- Shared code is consumed via `lib_extra_dirs = ../shared`; update both coordinator/node as needed and keep APIs synchronized.

## Guardrails & best practices
- Never double-call `Logger::begin`; adjust verbosity via `Logger::setMinLevel` instead.
- When touching messaging schemas or MQTT topics, audit the docs + frontend expectations; prefer new optional fields over breaking changes.
- Keep ESP-NOW channel/power tweaks inside `EspNow` (it already enforces STA-only mode, PMK, channel=1). Don’t reconfigure Wi-Fi elsewhere.
- Use `ConfigManager` namespaces (`"wifi"`, `"mqtt"`, etc.) for persistence; handle the erase/re-init path described in `docs/development/COMPLETE_NVS_FIX.md`.
- For new hardware features, follow the manager pattern: allocate in `Coordinator::begin`, gate failures with `Logger::error`, and add a `loop()` pump plus serial/MQTT observability.

Questions or unclear workflows (e.g., OTA, pairing UX, telemetry additions)? Note them in `docs/development/` and update this playbook.
