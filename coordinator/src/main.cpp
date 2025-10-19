#include <Arduino.h>
#include "core/Coordinator.h"
#include "utils/Logger.h"

Coordinator coordinator;

void setup() {
    // Initialize logger first for early debugging
    Logger::begin(115200);
    Logger::info("*** SETUP START ***");
    
    bool success = coordinator.begin();
    if (!success) {
        Logger::error("*** COORDINATOR INITIALIZATION FAILED ***");
        while(1) {
            delay(1000);
            Logger::error("System halted due to initialization failure");
        }
    }
    Logger::info("*** SETUP COMPLETE ***");
}

void loop() {
    coordinator.loop();
    delay(1);
}
