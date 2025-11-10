#pragma once

#include <Arduino.h>
#include <Wire.h>

// TMP177 I2C Temperature Sensor Driver
class TMP177Sensor {
public:
    // TMP177 I2C address (default: 0x48)
    static constexpr uint8_t I2C_ADDRESS = 0x48;
    
    // TMP177 Registers
    static constexpr uint8_t REG_TEMPERATURE = 0x00;
    static constexpr uint8_t REG_CONFIG = 0x01;
    
    TMP177Sensor() : initialized(false), lastTemp(0.0f) {}
    
    bool begin(uint8_t sda, uint8_t scl) {
        Wire.begin(sda, scl);
        Wire.setClock(100000); // 100kHz I2C
        
        // Check if device is present
        Wire.beginTransmission(I2C_ADDRESS);
        if (Wire.endTransmission() != 0) {
            Serial.println("TMP177: Device not found!");
            return false;
        }
        
        // Configure: 12-bit resolution, continuous conversion
        Wire.beginTransmission(I2C_ADDRESS);
        Wire.write(REG_CONFIG);
        Wire.write(0x60); // 12-bit, continuous
        Wire.write(0xA0);
        if (Wire.endTransmission() != 0) {
            Serial.println("TMP177: Configuration failed!");
            return false;
        }
        
        initialized = true;
        Serial.println("TMP177: Initialized successfully");
        return true;
    }
    
    float readTemperature() {
        if (!initialized) {
            return lastTemp;
        }
        
        // Request temperature register
        Wire.beginTransmission(I2C_ADDRESS);
        Wire.write(REG_TEMPERATURE);
        if (Wire.endTransmission() != 0) {
            Serial.println("TMP177: Read request failed!");
            return lastTemp;
        }
        
        // Read 2 bytes
        Wire.requestFrom(I2C_ADDRESS, (uint8_t)2);
        if (Wire.available() != 2) {
            Serial.println("TMP177: Data not available!");
            return lastTemp;
        }
        
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        
        // Convert to temperature (12-bit resolution, 0.0625°C per LSB)
        int16_t raw = (msb << 4) | (lsb >> 4);
        
        // Handle negative temperatures (12-bit two's complement)
        if (raw & 0x800) {
            raw |= 0xF000;
        }
        
        lastTemp = raw * 0.0625f;
        return lastTemp;
    }
    
    bool isInitialized() const {
        return initialized;
    }
    
private:
    bool initialized;
    float lastTemp;
};
