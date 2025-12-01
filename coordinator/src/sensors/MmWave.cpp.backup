#include "MmWave.h"
#include "../utils/Logger.h"
#include <Ld2450.h>
#include <cmath>

// Forward declare callback
static void radarDataCallback(hilink::Ld2450::SensorTarget target, const hilink::Ld2450::SensorData& data);

// Local LD2450 instance (we keep it file-local to avoid exposing in header)
static hilink::Ld2450* gRadar = nullptr;
static MmWave* gMmWaveInstance = nullptr;

// Target data cache (LD2450 supports 3 targets: TARGET_1, TARGET_2, TARGET_3)
struct TargetCache {
    bool valid = false;
    hilink::Ld2450::SensorData data;
    uint32_t lastUpdateMs = 0;
};
static TargetCache gTargets[3];

MmWave::MmWave()
    : eventCallback(nullptr)
    , radarSerial(&Serial1)
    , currentPresence(false)
    , lastEventTime(0) {
    gMmWaveInstance = this;
}

MmWave::~MmWave() {
    if (gRadar) {
        gRadar->stopData();
        delete gRadar;
        gRadar = nullptr;
    }
    gMmWaveInstance = nullptr;
}

bool MmWave::begin() {
    // Configure UART BEFORE creating radar instance
    radarSerial->setRxBufferSize(Pins::MmWave::RX_BUF_SIZE);
    radarSerial->begin(Pins::MmWave::BAUD_RATE, SERIAL_8N1, Pins::MMWAVE_RX, Pins::MMWAVE_TX);

    // Create LD2450 instance with callback (new v1.0.1 API)
    gRadar = new hilink::Ld2450(*radarSerial, radarDataCallback);
    
    // Initialize radar driver
    gRadar->init();
    
    // Start data streaming (callback will be invoked on tick())
    gRadar->startData();
    
    // Initialize state
    radarReady = true;  // Assume ready, will mark offline if no data
    lastFrameMs = millis();
    consecutiveFailures = 0;
    restartAttempts = 0;
    totalRestarts = 0;
    sensorSuppressed = false;
    offlineHintPrinted = false;
    
    // Clear target cache
    for (int i = 0; i < 3; i++) {
        gTargets[i].valid = false;
        gTargets[i].lastUpdateMs = 0;
    }
    
    Logger::info("MmWave LD2450 initialized - streaming via callback");
    return true;
}

void MmWave::loop() {
    if (!gRadar) return;
    
    // Tick the radar (processes serial data and triggers callbacks)
    gRadar->tick();
    
    // Check stream health periodically
    ensureStreamHealth();
    
    // Process cached radar data periodically
    uint32_t now = millis();
    if ((now - lastPublishMs) >= MIN_PUBLISH_INTERVAL_MS) {
        processRadarFrame();
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
    uint8_t validCount = 0;
    uint32_t now = millis();
    
    // LD2450 supports 3 targets (TARGET_1, TARGET_2, TARGET_3)
    for (uint8_t i = 0; i < 3; ++i) {
        MmWaveEvent::MmWaveTarget t{};
        t.id = i + 1;
        
        // Check if target data is recent (within 500ms)
        if (gTargets[i].valid && (now - gTargets[i].lastUpdateMs) < 500) {
            t.valid = true;
            t.x_mm = gTargets[i].data.x;
            t.y_mm = gTargets[i].data.y;
            t.speed_cm_s = gTargets[i].data.speed;
            t.resolution_mm = gTargets[i].data.resolution;
            
            // Calculate distance from x,y
            t.distance_mm = (int)sqrt((float)(t.x_mm * t.x_mm + t.y_mm * t.y_mm));
            
            // Derive velocity components (m/s)
            t.vx_m_s = 0.0f;
            t.vy_m_s = 0.0f;
            if (t.distance_mm > 0) {
                float dist = (float)t.distance_mm;
                float ux = (float)t.x_mm / dist;
                float uy = (float)t.y_mm / dist;
                float v_m_s = (float)t.speed_cm_s / 100.0f; // cm/s -> m/s
                t.vx_m_s = v_m_s * ux;
                t.vy_m_s = v_m_s * uy;
            }
            validCount++;
        } else {
            t.valid = false;
        }
        evt.targets.push_back(t);
    }
    evt.confidence = (evt.targets.empty()) ? 0.0f : ((float)validCount / 3.0f);
}

void MmWave::emitPresenceEvent(const MmWaveEvent& evt) {
    // Debounce presence state changes; still allow periodic publishes handled upstream
    if (evt.presence == currentPresence) {
        // if same state, just forward (rate limiting done in processRadarFrame)
    }
    if (eventCallback) {
        eventCallback(evt);
        
        // Log to serial only every Nth frame to reduce clutter (but always log state changes)
        frameCounter++;
        bool stateChanged = (evt.presence != currentPresence);
        if (stateChanged || (frameCounter % SERIAL_LOG_DIVIDER == 0)) {
            Logger::info("MmWave frame: targets=%d presence=%d conf=%.2f%s", 
                        (int)evt.targets.size(), evt.presence, evt.confidence,
                        stateChanged ? " [STATE CHANGE]" : "");
        }
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
    Logger::warn("mmWave stream stalled (%s) - restarting", reason ? reason : "unknown");
    lastRestartMs = millis();
    if (totalRestarts < 0xFFFF) {
        totalRestarts++;
    }
    
    // Stop and delete old instance
    if (gRadar) {
        gRadar->stopData();
        delete gRadar;
        gRadar = nullptr;
    }
    
    // Restart UART
    radarSerial->end();
    delay(20);
    radarSerial->setRxBufferSize(Pins::MmWave::RX_BUF_SIZE);
    radarSerial->begin(Pins::MmWave::BAUD_RATE, SERIAL_8N1, Pins::MMWAVE_RX, Pins::MMWAVE_TX);
    
    // Create new instance
    gRadar = new hilink::Ld2450(*radarSerial, radarDataCallback);
    gRadar->init();
    gRadar->startData();
    
    // Clear target cache
    for (int i = 0; i < 3; i++) {
        gTargets[i].valid = false;
        gTargets[i].lastUpdateMs = 0;
    }
    
    radarReady = true;
    lastFrameMs = millis();
    consecutiveFailures = 0;
    
    Logger::info("mmWave restarted");
    return true;
}

bool MmWave::isOnline() const {
    if (!radarReady || sensorSuppressed) {
        return false;
    }
    uint32_t age = millis() - lastFrameMs;
    return age < (STREAM_STALE_MS * 2);
}

// Callback function invoked by LD2450 library when target data arrives
static void radarDataCallback(hilink::Ld2450::SensorTarget target, const hilink::Ld2450::SensorData& data) {
    // Cache target data for processing in loop()
    uint8_t idx = static_cast<uint8_t>(target);
    if (idx < 3) {
        gTargets[idx].valid = true;
        gTargets[idx].data = data;
        gTargets[idx].lastUpdateMs = millis();
        
        // Mark that we've received data (update lastFrameMs via instance if available)
        if (gMmWaveInstance) {
            gMmWaveInstance->lastFrameMs = millis();
            gMmWaveInstance->consecutiveFailures = 0;
            gMmWaveInstance->radarReady = true;
        }
    }
}
