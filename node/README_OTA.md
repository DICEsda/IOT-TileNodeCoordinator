# OTA Updates (Node - ESP32-C3)

This project supports simple HTTP OTA updates from a URL in standalone mode.

## Requirements
- Build your firmware to a .bin (use env `esp32-c3-mini-1-ota` for dual-slot partitions)
- Host the .bin over HTTP/HTTPS (local server, GitHub Releases raw URL, etc.)

## Steps
1. Flash current firmware and open Serial Monitor @ 115200
2. Connect to your device over USB
3. Run the OTA command in serial:

   Example:
   ota MySSID MyPass http://192.168.1.10:8000/firmware.bin

   Optionally include MD5:
   ota MySSID MyPass http://192.168.1.10:8000/firmware.bin d41d8cd98f00b204e9800998ecf8427e

4. The device will connect to WiFi, download the binary, update, and reboot.

## Notes
- Use the `esp32-c3-mini-1-ota` env for an OTA-friendly partition layout.
- For production, integrate coordinator-orchestrated OTA with HTTPS + SHA256 and rollback checks.
- If WiFi fails, check credentials and 2.4GHz availability.
