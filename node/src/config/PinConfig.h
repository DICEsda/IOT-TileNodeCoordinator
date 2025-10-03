#pragma once

#include <Arduino.h>

// ESP32-C3-MINI-1 Pin Configuration
namespace Pins {
    // LED Configuration
    constexpr uint8_t LED_DATA = 2;      // WS2812/SK6812 data pin (GPIO2 supports RMT)
    constexpr uint8_t STATUS_LED = 8;    // Built-in RGB LED (WS2812) on GPIO8
    
    // Temperature Sensor (SPI)
    constexpr uint8_t TEMP_SCK = 4;      // SPI Clock
    constexpr uint8_t TEMP_MISO = 5;     // SPI MISO
    constexpr uint8_t TEMP_MOSI = 6;     // SPI MOSI
    constexpr uint8_t TEMP_CS = 7;       // SPI Chip Select
    
    // Button Input
    constexpr uint8_t BUTTON = 9;        // GPIO9 for button input
    
    // Power Management
    constexpr uint8_t POWER_EN = 3;      // GPIO3 for power control (if needed)
    
    // Debug UART (Optional)
    constexpr uint8_t DEBUG_TX = 21;     // UART0 TX
    constexpr uint8_t DEBUG_RX = 20;     // UART0 RX
    
    // I2C (Reserved/Optional)
    constexpr uint8_t I2C_SDA = 10;      // I2C SDA
    constexpr uint8_t I2C_SCL = 1;       // I2C SCL
    
    // ESP32-C3-MINI-1 Built-in LED control
    struct RgbLed {
        static constexpr uint8_t PIN = 8;
        static constexpr uint8_t CHANNEL = 0;
        static constexpr uint8_t NUM_PIXELS = 1;
    };
}
