#include "MmWave.h"
#include "../utils/Logger.h"

MmWave::MmWave()
    : eventCallback(nullptr)
    , currentPresence(false)
    , lastEventTime(0) {
}

MmWave::~MmWave() {
}

bool MmWave::begin() {
    // Initialize UART for mmWave sensor
    Serial1.begin(Pins::MmWave::BAUD_RATE, SERIAL_8N1, Pins::MMWAVE_RX, Pins::MMWAVE_TX);
    Serial1.setRxBufferSize(Pins::MmWave::RX_BUF_SIZE);
    
    Logger::info("MmWave sensor initialized");
    return true;
}

void MmWave::loop() {
    if (Serial2.available()) {
        processSerialData();
    }
}

void MmWave::setEventCallback(void (*callback)(const MmWaveEvent& event)) {
    eventCallback = callback;
}

void MmWave::processSerialData() {
    while (Serial2.available()) {
        String data = Serial2.readStringUntil('\n');
        
        // Parse the mmWave sensor data format
        // Example format: "ZONE:1,PRESENCE:1,CONF:85,DIST:1.2"
        if (data.length() > 0) {
            String zone;
            bool presence = false;
            int confidence = 0;
            float distance = 0;

            // Parse comma-separated values
            int start = 0;
            int end = data.indexOf(',');
            while (end >= 0) {
                String pair = data.substring(start, end);
                int colon = pair.indexOf(':');
                if (colon >= 0) {
                    String key = pair.substring(0, colon);
                    String value = pair.substring(colon + 1);
                    
                    if (key == "ZONE") zone = value;
                    else if (key == "PRESENCE") presence = value.toInt() == 1;
                    else if (key == "CONF") confidence = value.toInt();
                    else if (key == "DIST") distance = value.toFloat();
                }
                start = end + 1;
                end = data.indexOf(',', start);
            }

            // Process last pair
            String pair = data.substring(start);
            int colon = pair.indexOf(':');
            if (colon >= 0) {
                String key = pair.substring(0, colon);
                String value = pair.substring(colon + 1);
                if (key == "DIST") distance = value.toFloat();
            }

            // Only emit events if confidence is high enough and zone is valid
            if (!zone.isEmpty() && confidence >= 70) {
                emitPresenceEvent(zone, presence);
            }
        }
    }
}

void MmWave::emitPresenceEvent(const String& zone, bool presence) {
    uint32_t now = millis();
    
    // Debounce events
    if (now - lastEventTime < PRESENCE_DEBOUNCE_MS) {
        return;
    }
    
    // Only emit event if state changed or it's a different zone
    if (zone != currentZone || presence != currentPresence) {
        currentZone = zone;
        currentPresence = presence;
        lastEventTime = now;
        
        if (eventCallback) {
            MmWaveEvent event;
            event.sensorId = zone;
            event.presence = presence;
            event.timestampMs = now;
            eventCallback(event);
            
            Logger::info("MmWave event: zone=%s, presence=%d", 
                        zone.c_str(), presence);
        }
    }
}
