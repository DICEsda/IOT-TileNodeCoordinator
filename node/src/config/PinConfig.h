#pragma once

#include <Arduino.h>

// ESP32-C3-MINI-1 Pin Configuration
namespace Pins {
    // LED Configuration - SK6812B RGBW strip
    // GPIO0 is a strapping pin and doesn't work reliably for NeoPixel!
    // Use GPIO4 (or GPIO5, GPIO6, GPIO7, GPIO10) for LED data instead
    constexpr uint8_t LED_DATA_1 = 4;    // SK6812B data pin (was GPIO0, now GPIO4)
    constexpr uint8_t LED_DATA_2 = 5;    // WS2812/SK6812 data pin strip 2 (optional)
    constexpr uint8_t LED_DATA_3 = 6;    // WS2812/SK6812 data pin strip 3 (optional)
    constexpr uint8_t STATUS_LED = 8;    // Built-in RGB LED (WS2812) on GPIO8
    
    // Button Input
    constexpr uint8_t BUTTON = 3;        // GPIO3 for button input
    
    // Debug UART (Optional)
    constexpr uint8_t DEBUG_TX = 21;     // UART0 TX
    constexpr uint8_t DEBUG_RX = 20;     // UART0 RX
    
    // I2C for TMP117 Temperature Sensor
    constexpr uint8_t I2C_SDA = 1;       // I2C SDA for TMP117
    constexpr uint8_t I2C_SCL = 2;       // I2C SCL for TMP117
    
    // ESP32-C3-MINI-1 Built-in LED control
    struct RgbLed {
        static constexpr uint8_t PIN = 8;
        static constexpr uint8_t CHANNEL = 0;
        static constexpr uint8_t NUM_PIXELS = 1;
    };
}
