# Product Requirement Document

**Title:** Smart Tile Lighting System (Software Only, RGBW + Status Version)**
**Version:** 0.5 (Status Indicator Update)**
**Owner:** Daahir Hussein
**Last updated:** 2025-10-18

---

### Changelog

* **v0.5:** Added status-indicator behavior using SK6812B strip (pairing, OTA, error, etc.); updated telemetry schema and config keys.
* **v0.4:** SK6812B RGBW LED support (4 LEDs per node), RGBW message schemas.
* **v0.3:** Coordinator-only button approval; refined reliability, acks, and defaults.
* **v0.2:** Software-only scope; battery nodes; push-button pairing.

---

## 1. Summary

A **battery-powered smart indoor lighting system** with multiple **ESP32-C3 light nodes** and a **single ESP32-S3 coordinator**, communicating via **ESP-NOW**.
Each node drives a **4-pixel SK6812B RGBW LED strip**, which provides **both lighting and system status indication**, and monitors local temperature via SPI.
The coordinator processes **mmWave presence events**, decides lighting commands, and bridges telemetry/config/OTA to a backend via **MQTT**.

Pairing is local and secure: both node and coordinator must be physically button-pressed, and the coordinator approves joins.

---

## 2. Scope

Firmware, protocols, state machines, configuration, security, OTA, and backend contracts.
Excludes hardware or mechanical details.

---

## 3. Goals and Success Metrics

| Goal                                       | Target |
| ------------------------------------------ | ------ |
| Presence-driven, color-adaptive lighting   | ✓      |
| Secure push-button pairing                 | ✓      |
| Reliable ESP-NOW + MQTT bridge             | ✓      |
| Battery-aware and thermally safe operation | ✓      |
| OTA with signed images and rollback        | ✓      |

| Metric                    | Target         | Notes                      |
| ------------------------- | -------------- | -------------------------- |
| Presence→light latency    | ≤180 ms (P95)  | mmWave → LED update        |
| ESP-NOW cmd success       | ≥98 %          | 10 m LOS                   |
| Pairing time              | ≤30 s          | Buttons                    |
| Telemetry success         | ≥99 %/day      |                            |
| Idle radio duty cycle     | ≤20 %          |                            |
| Coordinator uptime        | ≥99 %/month    |                            |
| Color consistency         | ≤3 % deviation |                            |
| Status indication latency | ≤500 ms        | State → LED pattern update |

---

## 4. System Architecture

**Components**

* **Light Node (ESP32-C3)** — drives SK6812B LEDs, reads temperature, handles pairing/OTA.
* **Coordinator (ESP32-S3)** — mmWave presence logic, ESP-NOW control, MQTT bridge.
* **Backend** — config, telemetry, calibration, OTA orchestration.

**Data Flow:**
`mmWave → Decision → set_light (ESP-NOW) → Node → RGBW/Status output → Telemetry via MQTT`.

---

## 5. Functional Requirements

**FR-1** Presence-based RGBW control with fade in/out.
**FR-2** Push-button pairing (coordinator approval only).
**FR-3** ESP-NOW encrypted unicast control plane.
**FR-4** MQTT bridge with TLS and structured telemetry.
**FR-5** Temperature-based derating.
**FR-6** Safe OTA with rollback.
**FR-7** Low power operation (light sleep windows).
**FR-8** RGBW lighting patterns (uniform, gradient, indexed).
**FR-9** LED strip doubles as system status indicator when not in lighting mode.

---

## 6. Non-Functional Requirements

* Security: per-node LMK, signed firmware, TLS MQTT.
* Reliability: persistent config, watchdog, crash-safe writes.
* Observability: structured logs, telemetry counters.
* Maintainability: modular ESP-IDF 5.x codebase with CI.
* Color consistency ≤ 3 %.
* Status indication non-intrusive, ≤ 30 % brightness.

---

## 7. Light Node Firmware (ESP32-C3)

### Modules

| Module                  | Description                                                                                           |
| ----------------------- | ----------------------------------------------------------------------------------------------------- |
| **RGBW Service**        | Drives 4 × SK6812B via RMT; supports `set_color(r,g,b,w,fade_ms,pattern)` and system status patterns. |
| **Temperature Service** | SPI sampling, derate alert.                                                                           |
| **ESP-NOW Client**      | Encrypted unicast with retries/acks.                                                                  |
| **Power Manager**       | Light sleep scheduling, jittered telemetry, disable LEDs during sleep.                                |
| **Config (NVS)**        | Stores keys, thresholds, color profiles, calibration.                                                 |
| **OTA Client**          | Dual-slot, rollback safety.                                                                           |

### State Machine

`BOOT → PAIRING → OPERATIONAL → DERATE → UPDATE`

### Low-Power Defaults

rx_window 20 ms · rx_period 100 ms · telemetry 30 s ±10 %
Ack timeout 30 ms · Retries 3 · TTL 1500 ms

### Thermal Derating

Linear 100 % → 30 % brightness from 70 °C → 85 °C; white channel reduced first.

---

## 8. Coordinator Firmware (ESP32-S3)

### Tasks

* **mmWave:** parse → events {zone,presence,confidence}.
* **Decision:** map zone→light_ids; issue RGBW set_light commands.
* **ESP-NOW Server:** manage peers, resend within TTL.
* **MQTT Bridge:** telemetry/mmWave publish; commands/config subscribe.
* **Pairing Manager:** approve locally, persist registry.
* **Registry Store:** NVS persistence.

### Defaults

presence_debounce 150 ms · occupancy_hold 5000 ms
fade_in 150 ms · fade_out 1000 ms · pairing_window 120 s

---

## 9. Communication Contracts

### 9.1 ESP-NOW Reliability

Encrypted unicast; 3 retries @ 10 ms; TTL 1500 ms; `cmd_id` (UUID) for idempotency.

### 9.2 ESP-NOW Schemas

(join_request / join_accept / error / ack unchanged except LED caps)

#### set_light

```json
{
  "msg": "set_light",
  "cmd_id": "<uuid>",
  "light_id": "L12",
  "r": 255, "g": 180, "b": 120, "w": 255,
  "fade_ms": 500,
  "pattern": "uniform",
  "reason": "presence|backend|safety",
  "ttl_ms": 1500,
  "override_status": false
}
```

#### node_status

```json
{
  "msg": "node_status",
  "node_id": "C3DDEE",
  "light_id": "L12",
  "status_mode": "operational|pairing|ota|error",
  "avg_r": 240, "avg_g": 180, "avg_b": 120, "avg_w": 255,
  "temp_c": 24.2,
  "vbat_mv": 3700,
  "fw": "c3-1.0.2",
  "ts": 1693560000
}
```

---

### 9.3 MQTT Topics and Payloads (TLS, QoS 1)

| Topic                                     | Direction | Example Payload                                                                                                 |
| ----------------------------------------- | --------- | --------------------------------------------------------------------------------------------------------------- |
| `site/{siteId}/coord/{coordId}/mmwave`    | →         | `{ "ts":1693560000, "events":[{"zone":3,"presence":true,"confidence":0.82}]}`                                   |
| `site/{siteId}/coord/{coordId}/telemetry` | →         | `{ "ts":1693560000, "fw":"s3-1.2.1","nodes_online":18,"wifi_rssi":-58}`                                         |
| `site/{siteId}/node/{nodeId}/telemetry`   | →         | `{ "ts":1693560000,"avg_r":240,"avg_g":180,"avg_b":120,"avg_w":255,"status_mode":"operational","vbat_mv":3700}` |
| `site/{siteId}/coord/{coordId}/cmd`       | ←         | Backend commands                                                                                                |

**cmd.color_profile**

```json
{
  "action": "set_color_profile",
  "profile": {
    "presence": {"r":255,"g":180,"b":120,"w":255},
    "idle": {"r":0,"g":0,"b":0,"w":0}
  }
}
```

---

## 10. Pairing and Provisioning

Same secure flow (rotating token, local approval).
Adds **visual pairing indicator** via LED strip (§12.1).

---

## 11. Power Management

Unchanged; LED status limited to 30 % brightness during low-power modes; animations suspend deep sleep only when necessary.

---

## 12. Control Logic

* Presence → apply RGBW “presence” color, fade_in.
* Absence → fade_out to idle.
* Multi-zone: per-channel max.
* Temperature derating scales brightness.
* Backend override pins color.
* Optional animation patterns.

---

## 12.1 Status Indication Behavior (NEW)

Each node uses its 4 LED SK6812B strip for system status feedback when not showing active lighting.

| State                    | LED Pattern                | Notes                      |
| ------------------------ | -------------------------- | -------------------------- |
| **BOOT**                 | All white 20 % → off (1 s) | Power-on                   |
| **PAIRING window**       | Cyan slow breathe (1 Hz)   | Coordinator button pressed |
| **JOIN_REQUEST sent**    | Blue blink (2 Hz)          | Waiting for accept         |
| **PAIR_SUCCESS**         | Solid green 1 s → off      | Confirmation               |
| **OTA in progress**      | Amber chase                | Chunk transfer             |
| **OTA success**          | 3× green pulses            |                            |
| **OTA fail / rollback**  | 3× red pulses              |                            |
| **DERATE active**        | White flicker (0.5 Hz)     | Temp > 70 °C               |
| **ERROR**                | Solid red 50 %             | Until cleared              |
| **LOW BATTERY**          | Blue blink (10 s interval) | Subtle                     |
| **OPERATIONAL (normal)** | Presence lighting active   | Normal use                 |

**Behavior Rules**

* Status animation has priority unless `override_status=false` in incoming command.
* Brightness limited to 30 %.
* Disabled during deep sleep.
* Non-blocking animations implemented in RGBW service task.

**Config additions**

```json
{
  "status_mode_enabled": true,
  "status_brightness_pct": 30,
  "status_priority": "system>ota>lighting"
}
```

---

## 13. Configuration and Defaults

**Coordinator**

* presence_debounce 150 ms
* occupancy_hold 5000 ms
* fade_in 150 ms
* fade_out 1000 ms
* pairing_window 120 s
* color_profiles: presence/idle
* whitelist_prefixes: ["AA:BB"]

**Node**

* led_type: "SK6812B"
* led_count: 4
* color_mode: "rgbw"
* telemetry_interval 30 s
* rx_window 20 ms
* rx_period 100 ms
* derate_start 70 °C
* derate_min 30 %
* retry_count 3
* cmd_ttl 1500 ms
* status_mode_enabled true

---

## 14. Security

Per-node LMK encryption; PMK in secure partition.
TLS MQTT; signed firmware with monotonic version.
Pairing rate-limit & lockout.

---

## 15. OTA Strategy

Coordinator-mediated 1 KB chunks; rollback on telemetry absence.
Staged rollout by site.
Versions: `c3-x.y.z` (node), `s3-x.y.z` (coord).

---

## 16. Telemetry & Observability

**Node Metrics**

* avg_r/g/b/w
* temp_c
* vbat_mv
* fw
* status_mode
* uptime_s

**Coordinator Metrics**

* nodes_online
* mmwave_event_rate
* esp_now_queue_depth
* wifi_rssi
* fw

**Logs**
JSON: `{ts,level,component,msg,context}`
Levels: TRACE → ERROR.

---

## 17. Definition of Done

* Meets metrics (§3).
* Presence→light ≤ 180 ms P95.
* Status indicators function for all states.
* Pairing within 120 s window, coordinator-approved.
* Commands idempotent (`cmd_id`).
* OTA rollback verified.
* Telemetry includes RGBW + status.
* CI builds pass ESP-IDF 5.x.

---

✅ **PRD v0.5 — FINALIZED**
This version fully supports:

* 4-LED SK6812B RGBW strips
* Dual-purpose lighting + status indication
* Secure pairing, reliable ESP-NOW control
* OTA safety, telemetry, and backend integration

---
