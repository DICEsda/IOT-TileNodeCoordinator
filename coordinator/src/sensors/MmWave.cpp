#include "MmWave.h"
#include "../utils/Logger.h"
#include <LD2450.h>

// Local LD2450 instance (we keep it file-local to avoid exposing in header)
static LD2450 gRadar;

MmWave::MmWave()
    : eventCallback(nullptr)
    , radarSerial(&Serial1)
    , currentPresence(false)
    , lastEventTime(0) {
}

MmWave::~MmWave() {
}

bool MmWave::begin() {
    // Configure UART BEFORE begin (buffer size)
    radarSerial->setRxBufferSize(Pins::MmWave::RX_BUF_SIZE);
    radarSerial->begin(Pins::MmWave::BAUD_RATE, SERIAL_8N1, Pins::MMWAVE_RX, Pins::MMWAVE_TX);

    // Initialize radar driver
    gRadar.begin(*radarSerial, true); // true = start stream immediately
    gRadar.setNumberOfTargets(MAX_SENSOR_TARGETS);

    // Wait briefly for first sensor frame (non-blocking fallback)
    uint32_t start = millis();
    bool gotFrame = false;
    while ((millis() - start) < 500) {
        if (gRadar.waitForSensorMessage(false)) {
            gotFrame = true;
            break;
        }
        delay(20);
    }
    radarReady = gotFrame;
    lastFrameMs = millis();
    consecutiveFailures = 0;
    restartAttempts = 0;
    totalRestarts = 0;
    sensorSuppressed = false;
    offlineHintPrinted = false;
    Logger::info("MmWave LD2450 %s", radarReady ? "stream detected" : "no stream yet - will retry asynchronously");
    return true; // Non-critical if not ready yet
}

void MmWave::loop() {
    // Read radar stream; radar.read() returns >=0 when targets updated
    ensureStreamHealth();
    int status = gRadar.read();
    if (status >= 0) {
        lastFrameMs = millis();
        consecutiveFailures = 0;
        radarReady = true;
        restartAttempts = 0;
        sensorSuppressed = false;
        offlineHintPrinted = false;
        processRadarFrame();
    } else {
        if (consecutiveFailures < 200) {
            consecutiveFailures++;
        }
        ensureStreamHealth();
    }
}

void MmWave::setEventCallback(std::function<void(const MmWaveEvent& event)> callback) {
    eventCallback = callback;
}

void MmWave::processRadarFrame() {
    // Build event from current targets
    MmWaveEvent evt;
    evt.sensorId = "radar1"; // Single sensor identifier (could map to zone later)
    evt.timestampMs = millis();
    buildEventFromTargets(evt);

    // Presence heuristic: any valid target within 5 meters
    bool presence = false;
    for (auto &t : evt.targets) {
        if (t.valid && t.distance_mm > 0 && t.distance_mm <= 5000) {
            presence = true;
            break;
        }
    }
    evt.presence = presence;

    // Debounce presence changes (but always publish full target frames at interval)
    uint32_t now = millis();
    bool stateChanged = (presence != currentPresence);
    bool publishNow = stateChanged || (now - lastPublishMs) >= MIN_PUBLISH_INTERVAL_MS;
    if (!publishNow) return;
    lastPublishMs = now;

    if (stateChanged) {
        currentPresence = presence;
        lastEventTime = now;
    }
    emitPresenceEvent(evt);
}

void MmWave::buildEventFromTargets(MmWaveEvent& evt) {
    evt.targets.clear();
    uint8_t count = MAX_SENSOR_TARGETS;
    uint8_t validCount = 0;
    for (uint8_t i = 0; i < count; ++i) {
        auto tRaw = gRadar.getTarget(i);
        MmWaveEvent::MmWaveTarget t{};
        t.id = i + 1;
        t.valid = tRaw.valid;
        t.x_mm = tRaw.x;
        t.y_mm = tRaw.y;
        t.distance_mm = tRaw.distance;
        t.speed_cm_s = tRaw.speed;
        t.resolution_mm = tRaw.resolution;
        // derive velocity components (m/s)
        t.vx_m_s = 0.0f;
        t.vy_m_s = 0.0f;
        if (t.valid && t.distance_mm > 0) {
            float dist = (float)t.distance_mm;
            float ux = (float)t.x_mm / dist;
            float uy = (float)t.y_mm / dist;
            float v_m_s = (float)t.speed_cm_s / 100.0f; // cm/s -> m/s
            t.vx_m_s = v_m_s * ux;
            t.vy_m_s = v_m_s * uy;
        }
        if (t.valid) validCount++;
        evt.targets.push_back(t);
    }
    evt.confidence = (evt.targets.empty()) ? 0.0f : ((float)validCount / (float)evt.targets.size());
}

void MmWave::emitPresenceEvent(const MmWaveEvent& evt) {
    // Debounce presence state changes; still allow periodic publishes handled upstream
    if (evt.presence == currentPresence) {
        // if same state, just forward (rate limiting done in processRadarFrame)
    }
    if (eventCallback) {
        eventCallback(evt);
        Logger::info("MmWave frame: targets=%d presence=%d conf=%.2f", (int)evt.targets.size(), evt.presence, evt.confidence);
    }
}

void MmWave::ensureStreamHealth() {
    uint32_t now = millis();
    bool stale = (now - lastFrameMs) > STREAM_STALE_MS;
    bool tooManyErrors = consecutiveFailures > 25;
    if (sensorSuppressed && (now - offlineSinceMs) > OFFLINE_RETRY_MS) {
        sensorSuppressed = false;
        restartAttempts = 0;
        Logger::info("mmWave retrying after offline holdoff");
    }

    if (!(stale || tooManyErrors) || !radarSerial) {
        return;
    }

    if (sensorSuppressed) {
        return;
    }

    if ((now - lastRestartMs) < RESTART_BACKOFF_MS) {
        return;
    }

    const char* reason = stale ? "no frames" : "read errors";
    bool restored = restartRadar(reason);
    if (restored) {
        restartAttempts = 0;
        sensorSuppressed = false;
        offlineHintPrinted = false;
        return;
    }

    if (restartAttempts < 0xFF) {
        restartAttempts++;
    }

    if (restartAttempts >= MAX_RESTARTS_BEFORE_OFFLINE) {
        sensorSuppressed = true;
        offlineSinceMs = now;
        if (!offlineHintPrinted) {
            Logger::error("mmWave offline after repeated restarts. Check LD2450 wiring (RX=GPIO44, TX=GPIO43, 3V3, GND).");
            offlineHintPrinted = true;
        }
    }
}

bool MmWave::restartRadar(const char* reason) {
    Logger::warn("mmWave stream stalled (%s) - restarting UART", reason ? reason : "unknown");
    lastRestartMs = millis();
    if (totalRestarts < 0xFFFF) {
        totalRestarts++;
    }
    radarSerial->end();
    delay(20);
    radarSerial->setRxBufferSize(Pins::MmWave::RX_BUF_SIZE);
    radarSerial->begin(Pins::MmWave::BAUD_RATE, SERIAL_8N1, Pins::MMWAVE_RX, Pins::MMWAVE_TX);
    gRadar.begin(*radarSerial, true);
    gRadar.setNumberOfTargets(MAX_SENSOR_TARGETS);

    uint32_t start = millis();
    bool gotFrame = false;
    while ((millis() - start) < 500) {
        if (gRadar.waitForSensorMessage(false)) {
            gotFrame = true;
            break;
        }
        delay(20);
    }

    radarReady = gotFrame;
    lastFrameMs = millis();
    consecutiveFailures = 0;
    if (gotFrame) {
        Logger::info("mmWave stream restored");
    } else {
        Logger::warn("mmWave stream still idle after restart");
    }
    return gotFrame;
}

bool MmWave::isOnline() const {
    if (!radarReady || sensorSuppressed) {
        return false;
    }
    uint32_t age = millis() - lastFrameMs;
    return age < (STREAM_STALE_MS * 2);
}
