#include "TempSensor.h"

TempSensor::TempSensor(uint8_t csPin) : csPin(csPin), currentTemp(0), lastSampleTime(0) {}

void TempSensor::begin(const Config& cfg) {
    config = cfg;
    
    // Configure SPI for the temperature sensor with specific pins
    SPI.begin(Pins::TEMP_SCK, Pins::TEMP_MISO, Pins::TEMP_MOSI);
    pinMode(Pins::TEMP_CS, OUTPUT);
    digitalWrite(Pins::TEMP_CS, HIGH); // Deselect the sensor
    
    // Initial temperature reading
    currentTemp = readTemperature();
}

void TempSensor::update() {
    uint32_t now = millis();
    if (now - lastSampleTime >= config.sampleInterval) {
        currentTemp = readTemperature();
        lastSampleTime = now;
    }
}

float TempSensor::readTemperature() {
    digitalWrite(csPin, LOW);
    
    // Example SPI read for a typical temperature sensor
    // Modify according to your specific sensor's protocol
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    uint16_t rawTemp = SPI.transfer16(0x00);
    SPI.endTransaction();
    
    digitalWrite(csPin, HIGH);
    
    // Convert raw value to temperature (example conversion)
    // Modify according to your sensor's specifications
    float temperature = (rawTemp * 0.0625); // Example conversion factor
    
    return temperature;
}

uint8_t TempSensor::getDeratedDuty(uint8_t requestedDuty) const {
    if (currentTemp < config.derateStartC) {
        return requestedDuty;
    }
    
    if (currentTemp >= config.derateMaxC) {
        return config.derateMinDuty;
    }
    
    // Linear interpolation between full duty and minimum duty
    float tempRange = config.derateMaxC - config.derateStartC;
    float tempProgress = (currentTemp - config.derateStartC) / tempRange;
    float dutyRange = requestedDuty - config.derateMinDuty;
    
    return requestedDuty - (dutyRange * tempProgress);
}
