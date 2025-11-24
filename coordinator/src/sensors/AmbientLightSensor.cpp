#include "AmbientLightSensor.h"
#include "../utils/Logger.h"

AmbientLightSensor::AmbientLightSensor()
    : tsl(nullptr), initialized(false) {
}

AmbientLightSensor::~AmbientLightSensor() {
    if (tsl) {
        delete tsl;
        tsl = nullptr;
    }
}

bool AmbientLightSensor::begin() {
    if (!Wire.begin(Pins::External::I2C_SDA, Pins::External::I2C_SCL)) {
        Logger::error("Failed to initialize I2C bus (SDA=%d, SCL=%d)", 
                     Pins::External::I2C_SDA, Pins::External::I2C_SCL);
        initialized = false;
        return false;
    }
    
    Wire.setClock(100000);
    delay(100);

    // I2C Scanner to verify wiring
    Logger::info("Scanning I2C bus...");
    int devicesFound = 0;
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Logger::info("I2C device found at address 0x%02X", address);
            devicesFound++;
        }
    }
    if (devicesFound == 0) {
        Logger::warn("No I2C devices found! Check wiring (SDA=%d, SCL=%d)", 
                     Pins::External::I2C_SDA, Pins::External::I2C_SCL);
    }
    
    // Try all possible TSL2561 addresses
    const uint8_t addrs[] = {TSL2561_ADDR_FLOAT, TSL2561_ADDR_LOW, TSL2561_ADDR_HIGH};
    const char* addrNames[] = {"FLOAT (0x39)", "LOW (0x29)", "HIGH (0x49)"};

    for (int i = 0; i < 3; i++) {
        if (tsl) delete tsl;
        tsl = new Adafruit_TSL2561_Unified(addrs[i], 12345);

        if (tsl->begin(&Wire)) {
            Logger::info("TSL2561 sensor found at %s", addrNames[i]);
            
            tsl->enableAutoRange(true);
            tsl->setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);
            
            initialized = true;
            return true;
        }
    }
    
    Logger::warn("TSL2561 sensor not found at any address");
    if (tsl) {
        delete tsl;
        tsl = nullptr;
    }
    initialized = false;
    return false;
}

bool AmbientLightSensor::isConnected() {
    return initialized;
}

float AmbientLightSensor::readLux() {
    if (!initialized || !tsl) {
        return 0.0f;
    }
    
    sensors_event_t event;
    tsl->getEvent(&event);
    
    if (event.light == 0) {
        // It's possible it's just pitch black, but could be an error.
        // We'll return 0.0f anyway, but debug log it.
        // Logger::debug("TSL2561 read 0 lux");
        return 0.0f;
    }
    
    return event.light;
}
