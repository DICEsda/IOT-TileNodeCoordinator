#pragma once

#include <Arduino.h>

// ESP32-S3-DevKitC-1-N8 Pin Configuration
namespace Pins {
    // UART for mmWave Sensor
    constexpr uint8_t MMWAVE_RX = 18;    // UART1 RX
    constexpr uint8_t MMWAVE_TX = 17;    // UART1 TX
    
    // Status LEDs (ESP32-S3 has RGB LED on GPIO48)
    constexpr uint8_t STATUS_LED = 48;   // Built-in RGB LED
    constexpr uint8_t STATUS_R = 48;     // Red channel
    constexpr uint8_t STATUS_G = 35;     // Green channel (activity)
    constexpr uint8_t STATUS_B = 36;     // Blue channel (connection)
    
    // Button Input
    constexpr uint8_t PAIRING_BUTTON = 0;    // BOOT button for pairing
    
    // Optional External Sensors
    namespace External {
        // I2C Bus (for optional sensors)
        constexpr uint8_t I2C_SDA = 8;       // I2C SDA
        constexpr uint8_t I2C_SCL = 9;       // I2C SCL
        
        // SPI Bus (for optional sensors)
        constexpr uint8_t SPI_SCK = 12;      // SPI Clock
        constexpr uint8_t SPI_MISO = 13;     // SPI MISO
        constexpr uint8_t SPI_MOSI = 11;     // SPI MOSI
        constexpr uint8_t SPI_CS = 10;       // SPI Chip Select
    }
    
    // Debug UART (USB CDC)
    constexpr uint8_t DEBUG_TX = 43;     // UART0 TX
    constexpr uint8_t DEBUG_RX = 44;     // UART0 RX
    
    // ESP32-S3 Built-in RGB LED control
    struct RgbLed {
        static constexpr uint8_t PIN = STATUS_LED;
        static constexpr uint8_t CHANNEL = 0;
        static constexpr uint8_t NUM_PIXELS = 1;
    };
    
    // WiFi Status LED Control
    namespace WifiStatus {
        static constexpr uint8_t LED_CONNECTING = STATUS_B;    // Blue while connecting
        static constexpr uint8_t LED_CONNECTED = STATUS_G;     // Green when connected
        static constexpr uint8_t LED_ERROR = STATUS_R;         // Red on error
    }
    
    // mmWave UART Configuration
    namespace MmWave {
        static constexpr uint8_t UART_NUM = 1;
        static constexpr uint32_t BAUD_RATE = 115200;
        static constexpr uint8_t RX_BUF_SIZE = 256;
    }
}
