# Node Standalone Test (ESP32-C3-MINI-1)

This firmware lets you test the node without a coordinator. It drives an SK6812B/WS2812 strip and supports pairing-mode entry via button.

## Build config
- Target: `esp32-c3-mini-1`
- Framework: Arduino
- Build flags: `-DSTANDALONE_TEST` (already enabled in `platformio.ini`)

## Wiring
- LED strip data: `Pins::LED_DATA` (GPIO2)
- Button (active low): `Pins::BUTTON` (GPIO9) with internal pull-up
- Power as per your board specs

## Controls
- Short press: cycles modes (Off -> Warm Low -> Warm High -> Rainbow)
- Long press (hold 2s): toggle Pairing Mode
  - Pairing mode: cyan smooth blinking. Long press again to exit.
  - If pairing times out (2 minutes) without success, the strip flashes red-ish twice, then returns to the previous mode.

## Notes
- Adjust LED count in `node/src/main.cpp` via `LED_NUM_PIXELS`.
- Pairing mode here is visual-only for hardware validation.
- You can change timeout and failure color in `node/src/main.cpp`:
  - `pairingTimeoutMs` (default 120000)
  - `pairingFailedExit()` flash sequence and color (currently warm red-orange `255,50,20`).

## Flash & Monitor
1. Connect the board in bootloader mode if needed
2. Upload using PlatformIO
3. Open serial monitor at 115200 baud
