# ESP-NOW v2.0 Implementation Checklist

## ✅ Completed Changes for ESP32-S3 Coordinator

### 1. ✓ ESP-NOW Version
- **Status**: Using ESP-NOW v2.0 API
- **Implementation**: Changed callback signature from `(const uint8_t* mac, ...)` to `(const esp_now_recv_info_t* recv_info, ...)`
- **Location**: `coordinator/src/comm/EspNow.cpp` line 14-20

### 2. ✓ Board Definitions
- **Status**: Using latest ESP-IDF via PlatformIO
- **Board**: ESP32-S3-DevKitC-1
- **Platform**: espressif32 (latest)
- **Location**: `coordinator/platformio.ini`

### 3. ✓ Wi-Fi Mode
- **Status**: Set to WIFI_STA only
- **Implementation**: `WiFi.mode(WIFI_STA)` + `WiFi.disconnect()` to disable SoftAP
- **Verification**: Mode check added to confirm STA mode
- **Location**: `EspNow::begin()` line 66-75

### 4. ✓ MAC Addresses
- **Status**: Retrieved programmatically
- **Implementation**: `WiFi.macAddress(mac)` and logged on startup
- **Location**: `EspNow::begin()` line 78-83
- **Example Output**: `Coordinator MAC: 74:4D:BD:AB:A9:F4`

### 5. ✓ Peer Registration
- **Status**: Broadcast peer + dynamic node peer registration
- **Implementation**: 
  - Broadcast (FF:FF:FF:FF:FF:FF) registered at startup
  - Node peers registered on-demand during pairing
  - Peers persisted to NVS storage
- **Location**: `EspNow::begin()` line 123-145, `EspNow::addPeer()` line 368-388

### 6. ✓ Message Size
- **Status**: Enforced 250 byte limit
- **Implementation**: Size check before sending and warning on receive
- **Location**: `EspNow::sendToMac()` line 355-359, `EspNow::handleEspNowReceive()` line 336-340

### 7. ✓ Encryption
- **Status**: Disabled (unencrypted)
- **Implementation**: `peerInfo.encrypt = false` in all peer registrations
- **Note**: Can be enabled later by setting encrypt=true and sharing PMK/LMK
- **Location**: `EspNow::begin()` line 130, `EspNow::addPeer()` line 373

### 8. ✓ Callbacks
- **Status**: Both send and receive callbacks implemented
- **Implementation**: 
  - Receive: `staticRecvCallback()` with ESP-NOW v2 signature
  - Send: `staticSendCallback()` with delivery status
- **Location**: `coordinator/src/comm/EspNow.cpp` line 14-31

### 9. ✓ Testing
- **Status**: Test broadcast sent on startup
- **Implementation**: Sends `{"test":"coordinator_alive"}` to verify TX
- **Verification**: Serial output shows send status
- **Location**: `EspNow::begin()` line 148-162

### 10. ✓ Error Handling
- **Status**: Comprehensive error checking and logging
- **Implementation**:
  - All ESP-NOW API calls check return codes
  - Failed sends logged with error codes
  - Callback validates recv_info before processing
- **Location**: Throughout `EspNow.cpp`

## 📋 Pin Configuration (ESP32-S3)

| Function | GPIO | Notes |
|----------|------|-------|
| Pairing Button | GPIO0 | Active-low with pull-up (BOOT button) |
| LED Strip | GPIO15 | SK6812B RGBW strip |
| mmWave RX | GPIO18 | UART2 RX |
| mmWave TX | GPIO17 | UART2 TX |

## 🔧 Configuration Settings

- **WiFi Channel**: 1 (both coordinator and node must match)
- **WiFi Protocol**: 802.11b/g/n (for S3↔C3 compatibility)
- **TX Power**: 19.5dBm (maximum)
- **WiFi Sleep**: Disabled (for reliable RX)
- **Flash**: 8MB (QIO mode)
- **Baud Rate**: 115200

## 📊 Expected Serial Output

```
===========================================
ESP-NOW V2.0 INITIALIZATION CHECKLIST
===========================================
✓ [1/9] Setting WiFi mode to STA only...
  ✓ WiFi mode confirmed as STA
✓ [2/9] Getting MAC address programmatically...
  ✓ Coordinator MAC: 74:4D:BD:AB:A9:F4
✓ [3/9] Board definitions: ESP32-S3 via latest ESP-IDF
✓ [4/9] Disabling WiFi sleep for reliable reception...
✓ [5/9] Setting TX power to maximum (19.5dBm)...
✓ [6/9] Initializing ESP-NOW v2.0...
  ✓ ESP-NOW v2.0 initialized successfully
✓ [7/9] Setting WiFi channel to 1...
  ✓ Channel set to 1 (secondary: 0)
  ✓ WiFi protocol set successfully
✓ [8/9] Registering ESP-NOW v2.0 callbacks...
  ✓ Send and receive callbacks registered
✓ [9/9] Registering broadcast peer...
  ✓ Broadcast peer (FF:FF:FF:FF:FF:FF) added on channel 1
===========================================
TESTING: Sending test broadcast...
  Message size: 30 bytes (within 250 byte limit)
  ✓ Test broadcast queued successfully
===========================================
✓ ESP-NOW V2.0 READY - All checks passed!
✓ Coordinator listening for node messages
===========================================
```

## 🐛 Troubleshooting

### If RX callback never triggers:
1. Verify both devices use ESP-NOW v2.0 (check callback signatures)
2. Confirm both on same WiFi channel (use `esp_wifi_get_channel()`)
3. Check node is using native ESP-NOW, not ESPNowW wrapper
4. Verify node MAC is registered as peer on coordinator
5. Test with simple broadcast from node to verify TX works

### If messages fail to send:
1. Check message size < 250 bytes
2. Verify peer is registered before sending
3. Confirm WiFi mode is STA (not AP or STA+AP)
4. Check send callback for delivery status

### For ESP32-S3 ↔ ESP32-C3 compatibility:
1. Both must use WiFi protocol 802.11b/g/n
2. Both must be on same channel
3. Both should use native ESP-NOW v2.0 API (not wrappers)
4. Test each direction separately (S3→C3, then C3→S3)

## 📝 Next Steps

1. **Upload to ESP32-S3**: Build and upload coordinator firmware
2. **Test pairing**: Press GPIO0 button to enable pairing mode (LED turns blue)
3. **Monitor serial**: Watch for "ESP-NOW V2 RX CALLBACK TRIGGERED" message
4. **Update node**: Convert node to also use ESP-NOW v2.0 native API
5. **Test bidirectional**: Verify coordinator→node and node→coordinator both work

## 🔗 Node Requirements ✅ COMPLETE

The node has been updated to use ESP-NOW v2.0 with:
- ✅ Native `esp_now.h` API (ESPNowW library removed)
- ✅ Same v2 callback signatures as coordinator (`esp_now_recv_info_t*`)
- ✅ WiFi channel 1 (verified on startup)
- ✅ Broadcast to FF:FF:FF:FF:FF:FF during pairing
- ✅ Register coordinator MAC as peer after pairing
- ✅ Message size checks (≤250 bytes)
- ✅ Complete initialization checklist with logging

**See `ESP_NOW_V2_CONVERSION_COMPLETE.md` for full details.**

---
**Last Updated**: Based on ESP-NOW v2.0 checklist
**Hardware**: ESP32-S3-DevKitC-1 (Coordinator) + ESP32-C3 (Node)
