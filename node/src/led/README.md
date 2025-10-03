# LedController

Thin wrapper around Adafruit NeoPixel for SK6812B/WS2812 strips.

- Uses `Pins::LED_DATA` as data pin (GPIO2 on ESP32-C3-MINI-1)
- Supports brightness and color fades
- Call `update()` in loop to process fades
