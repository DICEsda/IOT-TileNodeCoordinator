#pragma once

#include <Arduino.h>

// ESP32-C3-MINI-1 Pin Configuration
namespace Pins {
    // LED Configuration - 3 strips, 4 LEDs each
    constexpr uint8_t LED_DATA_1 = 0;    // WS2812/SK6812 data pin strip 1
    constexpr uint8_t LED_DATA_2 = 1;    // WS2812/SK6812 data pin strip 2
    constexpr uint8_t LED_DATA_3 = 2;    // WS2812/SK6812 data pin strip 3
    constexpr uint8_t STATUS_LED = 8;    // Built-in RGB LED (WS2812) on GPIO8
    
    // Button Input
    constexpr uint8_t BUTTON = 3;        // GPIO3 for button input
    
    // Debug UART (Optional)
    constexpr uint8_t DEBUG_TX = 21;     // UART0 TX
    constexpr uint8_t DEBUG_RX = 20;     // UART0 RX
    
    // I2C for TMP177 Temperature Sensor
    constexpr uint8_t I2C_SDA = 1;       // I2C SDA for TMP177
    constexpr uint8_t I2C_SCL = 2;       // I2C SCL for TMP177
    
    // ESP32-C3-MINI-1 Built-in LED control
    struct RgbLed {
        static constexpr uint8_t PIN = 8;
        static constexpr uint8_t CHANNEL = 0;
        static constexpr uint8_t NUM_PIXELS = 1;
    };
}
