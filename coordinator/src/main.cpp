#include <Arduino.h>
#include <nvs_flash.h>
#include "core/Coordinator.h"
#include "utils/Logger.h"

Coordinator coordinator;

void setup() {
    // Initialize Serial first for USB CDC
    Serial.begin(115200);
    // Wait up to ~5s for USB host to attach for reliable first prints
    unsigned long _t0 = millis();
    while (!Serial && (millis() - _t0) < 5000) { delay(10); }
    delay(500); // Longer delay for USB-CDC stability
    
    Serial.println("\n\n===========================================");
    Serial.println("ESP32-S3 SMART TILE COORDINATOR");
    Serial.println("===========================================\n");
    Serial.flush();
    
    // Initialize Logger BEFORE anything else
    Serial.println("Initializing Logger...");
    Serial.flush();
    Logger::begin(115200);
    Logger::info("*** BOOT START ***");
    Serial.flush();
    
    // Initialize NVS - Critical for Preferences and WiFi
    Logger::info("Initializing NVS flash...");
    Serial.println("Initializing NVS...");
    
    // Try to initialize NVS first
    esp_err_t ret = nvs_flash_init();
    
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        Logger::warn("NVS partition needs to be erased (error: %s)", esp_err_to_name(ret));
        Serial.println("Erasing NVS partition...");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            Logger::error("✗ NVS erase failed: 0x%x (%s)", ret, esp_err_to_name(ret));
            Serial.printf("✗ NVS erase failed: 0x%x\n", ret);
        }
        delay(500);
        
        // Retry init
        ret = nvs_flash_init();
    }
    
    if (ret == ESP_OK) {
        Logger::info("✓ NVS initialized successfully");
        Serial.println("✓ NVS initialized successfully");
    } else {
        Logger::error("✗ NVS init failed: 0x%x (%s)", ret, esp_err_to_name(ret));
        Serial.printf("✗ NVS init failed: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        Serial.println("WARNING: System will continue but data persistence may not work!");
    }
    
    Serial.flush();
    delay(1000); // Allow NVS to fully stabilize before using Preferences
    
    Logger::info("*** SETUP START ***");
    Serial.println("Starting Coordinator...");
    Serial.flush();
    
    
    bool success = false;
    success = coordinator.begin();
    if (!success) {
        Logger::error("*** COORDINATOR INITIALIZATION FAILED ***");
        Serial.println("\n*** COORDINATOR INITIALIZATION FAILED ***");
        Serial.println("System halted - please check error messages above");
        Serial.flush();
        while(1) {
            delay(5000);
            Serial.println("System halted due to initialization failure");
        }
    }
    
    Logger::info("*** SETUP COMPLETE ***");
    Serial.println("\n*** SETUP COMPLETE - System Ready ***\n");
    Serial.flush();
}

void loop() {
    coordinator.loop();
    delay(1);
}
