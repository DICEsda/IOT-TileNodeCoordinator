# Product Requirement Document

Title: Smart Tile Lighting System (Software Only)
Version: 0.3 (refined)
Owner: Daahir Hussein
Last updated: 2025-09-01

Changelog
• v0.3: Refined structure; coordinator-only button approval; renumbered sections; clarified power-saving, message acks, and defaults.
• v0.2: Software-only scope; battery nodes; push-button pairing.

# 1. Summary

Software design for a battery powered smart indoor lighting system with many ESP32‑C3 light nodes controlled via ESP‑NOW by an ESP32‑S3 coordinator. Each node drives a light via PWM and reads a temperature sensor over SPI. The coordinator processes mmWave presence events, decides which lights to turn on, and bridges telemetry/commands with a backend over MQTT for calibration, monitoring, and configuration. Pairing is push‑button on both node and coordinator; the coordinator locally approves joins.

# 2. Scope

Firmware, protocols, state machines, configuration, security, and backend contracts. No hardware/BOM/mechanical content.

# 3. Goals and success metrics

Goals
• Presence‑driven lighting with low perceived latency.
• Push‑button pairing (coordinator approves locally).
• Reliable telemetry/control integration with backend via MQTT.
• Safe OTA with rollback.
• Battery‑aware operation on nodes.

Success metrics
• End‑to‑end presence→light‑on latency ≤ 150 ms P95 (mmWave event to PWM apply).
• ESP‑NOW command success ≥ 98 percent at 10 m LOS with retry/ack scheme.
• Pairing time ≤ 30 s per node with button workflow.
• Node idle radio duty cycle ≤ 20 percent; telemetry success ≥ 99 percent/day.
• Coordinator availability ≥ 99 percent/month.

# 4. System architecture

Components
• Light node firmware (ESP32‑C3)
• Coordinator firmware (ESP32‑S3)
• mmWave driver + presence processing (coordinator)
• MQTT bridge (coordinator)
• Backend services (approval UI optional, config, calibration, OTA orchestration, telemetry store)

Data flow
mmWave → Decision → set\_light over ESP‑NOW → Node PWM + status. Coordinator ↔ Backend via MQTT (telemetry, config, calibration, OTA control).

# 5. Functional requirements

FR‑1 Presence‑based control: mapped lights fade to target brightness on presence; fade out after hold time on absence.
FR‑2 Push‑button pairing: pairing requires physical button press on both node and coordinator within an active window; coordinator approves locally.
FR‑3 ESP‑NOW control plane: encrypted unicast for commands/acks; limited broadcast for discovery during pairing.
FR‑4 MQTT integration: TLS client; publishes telemetry/mmWave events; subscribes to config/commands.
FR‑5 Temperature derating: nodes reduce brightness above configurable thresholds and report events.
FR‑6 OTA: signed dual‑slot update with automatic rollback on failure.
FR‑7 Low power: nodes use light sleep with scheduled RX windows; coordinator repeats commands until ack or TTL expiry.

# 6. Non‑functional requirements

NFR‑1 Security: per‑node keys; TLS to broker; signed firmware; pairing rate limits.
NFR‑2 Reliability: persistent registry/mapping/config; watchdogs; crash‑safe writes.
NFR‑3 Observability: structured logs; counters; time‑series telemetry.
NFR‑4 Maintainability: clear module boundaries; code style and CI build for ESP‑IDF 5.x.

# 7. Light node firmware (ESP32‑C3)

Modules
• PWM service: set\_duty(value 0..255, fade\_ms).
• SPI temperature service: periodic sample and threshold alerts.
• ESP‑NOW client: encrypted unicast, ack/retry, clockless receive windows.
• Power manager: light sleep scheduling; jittered telemetry; deep idle option.
• Config (NVS): node\_id, light\_id, coord\_mac, LMK, defaults/thresholds, intervals.
• OTA client: image receive (proxied by coordinator) and slot management.

State machine
• BOOT → if keys present: OPERATIONAL else: PAIRING.
• PAIRING → listens; on button press within window sends join\_request; on join\_accept stores keys/config → OPERATIONAL.
• OPERATIONAL → low‑power with RX windows; on cmd set\_light apply PWM; periodic node\_status; on over‑temp enter DERATE; on OTA enter UPDATE → REBOOT.

Low‑power defaults (tunable per site)
• rx\_window\_ms: 20
• rx\_period\_ms: 100
• telemetry\_interval\_s: 30 (±10 percent jitter)
• command ack timeout: 30 ms; retries: 3; command TTL: 1500 ms (coordinator repeats within TTL)

Thermal behavior (configurable)
• derate\_start\_c: 70; derate\_min\_duty: 30 percent at 85 C; linear ramp.

# 8. Coordinator firmware (ESP32‑S3)

Tasks
• mmWave: parse frames → events {zone, presence, confidence, distance}.
• Decision: map zone→light\_id\[]; emit set\_light {value, fade\_ms, reason, ttl}.
• ESP‑NOW server: manage peers; unicast, ack, retry; repeat within TTL to bridge RX windows.
• MQTT bridge: publish telemetry/mmWave; consume config/commands; manage LWT.
• Pairing: on button press open pairing window; process join\_request; locally approve; send join\_accept; persist registry.
• Registry/config store: NVS persistence for nodes and zone→light mapping.

Parameters (defaults)
• presence\_debounce\_ms: 150
• occupancy\_hold\_ms: 5000
• fade\_in\_ms: 150; fade\_out\_ms: 1000
• multi‑zone: higher brightness wins when overlapping commands target same light.

# 9. Communication contracts

9.1 ESP‑NOW addressing and reliability
• node\_id: last 3 bytes of MAC (hex). light\_id: configurable.
• Control/status over encrypted unicast; discovery during pairing via broadcast beacon.
• Retries: 3; retry spacing: 10 ms; coordinator repeats command frames until ack or ttl\_ms elapsed.
• Every command carries cmd\_id (UUID) for idempotency; acks echo cmd\_id.

9.2 ESP‑NOW message schemas (UTF‑8 JSON serialized to bytes)
join\_request
{ "msg":"join\_request", "mac":"AA\:BB\:CC\:DD\:EE\:FF", "caps":{ "pwm":1, "temp\_spi"\:true }, "fw":"c3-1.0.0", "token":"<rotating>" }

join\_accept
{ "msg":"join\_accept", "node\_id":"C3DDEE", "lmk":"<base64>", "light\_id":"L12", "cfg":{ "pwm\_freq":1000, "rx\_window\_ms":20, "rx\_period\_ms":100 } }

set\_light
{ "msg":"set\_light", "cmd\_id":"<uuid>", "light\_id":"L12", "value":0..255, "fade\_ms":0..5000, "reason":"presence|backend|safety", "ttl\_ms":1500 }

node\_status
{ "msg":"node\_status", "node\_id":"C3DDEE", "light\_id":"L12", "temp\_c":23.1, "duty":128, "fw":"c3-1.0.0", "vbat\_mv":3700, "ts":1693560000 }

error
{ "msg":"error", "node\_id":"C3DDEE", "code":"over\_temp|comm\_timeout|ota\_fail", "info":"optional" }

ack
{ "msg":"ack", "cmd\_id":"<uuid>" }

9.3 MQTT topics and payloads (TLS, QoS 1)
Topics
• site/{siteId}/coord/{coordId}/telemetry
• site/{siteId}/coord/{coordId}/mmwave
• site/{siteId}/node/{nodeId}/telemetry
• site/{siteId}/coord/{coordId}/cmd
• site/{siteId}/coord/{coordId}/evt

Payload examples
mmwave
{ "ts":1693560000, "events":\[ { "zone":3, "presence"\:true, "confidence":0.82, "distance\_m":1.6 } ] }

telemetry.coord
{ "ts":1693560000, "fw":"s3-1.2.0", "nodes\_online":18, "wifi\_rssi":-58 }

telemetry.node
{ "ts":1693560000, "node\_id":"C3DDEE", "temp\_c":23.4, "duty":128, "last\_cmd":"presence", "vbat\_mv":3700 }

cmd.calibrate
{ "action":"calibrate", "zones":\[ { "id":1, "enable"\:true, "min\_dist":0.5, "max\_dist":3.5 } ] }

cmd.map
{ "action":"set\_mapping", "mappings":\[ { "zone":1, "lights":\["L01"] }, { "zone":2, "lights":\["L02","L03"] } ] }

cmd.ota
{ "action":"ota\_start", "target":"node|coord", "version":"c3-1.0.1", "url":"https\://...", "sha256":"..." }

LWT
• \$state retained online/offline for coordinator.

# 10. Pairing and provisioning (button + coordinator approval)

Flow

1. Coordinator button pressed → pairing window opens (pairing\_window\_s default 120); coordinator broadcasts pairing beacons with short‑lived rotating token.
2. Node button pressed → node enters pairing for 120 s, listens for beacons, sends join\_request including token and capabilities.
3. Coordinator validates token, locally approves (optional MAC whitelist), assigns node\_id/light\_id if pre‑provisioned, generates LMK, sends join\_accept.
4. Node stores LMK/config, replies node\_status, switches to OPERATIONAL.
5. Coordinator persists registry; emits evt pair\_success over MQTT.

Security
• Token rotates every 5 s; rate‑limited; only join\_request/accept allowed in pairing window.
• LMK unique per node; PMK stored in secure storage.

# 11. Power management (battery nodes)

• Light sleep with RX windows (rx\_window\_ms / rx\_period\_ms) aligns latency/battery. Defaults: 20/100 ms.
• Coordinator repeats commands within ttl\_ms to traverse RX gaps; expects ack; if none, drops after TTL.
• Telemetry cadence decoupled from control; jittered to avoid bursts.
• Quiet‑hours profile: extend rx\_period\_ms during expected idle times via config.
• Optional adjacent‑zone pre‑warm at low brightness to mask latency.

# 12. Control logic

• On presence true for zone Z → set\_light for mapped lights (value, fade\_in\_ms). Maintain Z as occupied for occupancy\_hold\_ms after last presence.
• On absence for all zones affecting a light → fade\_out\_ms to 0.
• Multiple zones → union → highest brightness wins per light.
• Temperature derating clamps commanded value before PWM apply.
• Backend manual override pins a light for a duration and suppresses presence commands until release.

# 13. Configuration and defaults

Config keys (coordinator)
• presence\_debounce\_ms: 150
• occupancy\_hold\_ms: 5000
• fade\_in\_ms: 150
• fade\_out\_ms: 1000
• map: \[{ zone, lights\[] }]
• pairing\_window\_s: 120
• whitelist\_prefixes: \["AA\:BB"] (optional)

Config keys (node)
• pwm\_freq\_hz: 1000
• pwm\_resolution\_bits: 12
• telemetry\_interval\_s: 30
• rx\_window\_ms: 20
• rx\_period\_ms: 100
• derate\_start\_c: 70
• derate\_min\_duty\_pct: 30
• retry\_count: 3
• cmd\_ttl\_ms: 1500

# 14. Security

• ESP‑NOW encrypted with per‑node LMK; PMK protected in secure partition.
• LMK rotation supported via backend‑initiated refresh through coordinator.
• MQTT over TLS with per‑device credentials and topic ACLs.
• Signed firmware with monotonic version to block rollback.
• Pairing rate limit and lockout on repeated invalid tokens.

# 15. OTA update strategy

• Staged rollout by site/group.
• Coordinator‑mediated download and chunked stream to nodes (preferred) or direct HTTPS when allowed.
• Health‑check window 5 minutes post‑boot; rollback if telemetry missing.
• Version tags: c3‑x.y.z (nodes), s3‑x.y.z (coordinator).

# 16. Telemetry and observability

Node metrics
• temp\_c, duty, vbat\_mv, fw, uptime\_s, last\_error, optional rssi\_to\_coord.

Coordinator metrics
• nodes\_online, mmwave\_event\_rate, esp\_now\_queue\_depth, wifi\_rssi, fw.

Logging
• Structured JSON: {ts, level, component, msg, context}. Levels: TRACE/DEBUG/INFO/WARN/ERROR.

Definition of done (embedded acceptance embedded in sections)
• Meets success metrics in Section 3 with defaults in Sections 11 and 13.
• Pairing flow works within pairing\_window\_s with coordinator approval only.
• Commands are idempotent via cmd\_id with positive ack.
• OTA completes with rollback safety and version reporting over telemetry.
