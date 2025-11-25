# WiFi Channel Synchronization Fix

## Problem
Node and Coordinator were on different WiFi channels, causing ESP-NOW communication failure:
- **Error**: `Peer channel is not equal to the home channel, send fail!`
- **Root Cause**: Coordinator uses router's WiFi channel (e.g., channel 6 or 11), but node was hardcoded to channel 1

## Solution
Added `wifi_channel` field to `JoinAcceptMessage` so coordinator tells node which channel to use during pairing.

### Changes Made

#### 1. Shared Message Structure (`shared/src/EspNowMessage.h`)
```cpp
struct JoinAcceptMessage : public EspNowMessage {
    String node_id;
    String light_id;
    String lmk;
    uint8_t wifi_channel; // ← NEW: WiFi channel coordinator is using
    struct Cfg { ... } cfg;
    ...
};
```

#### 2. Message Serialization (`shared/src/EspNowMessage.cpp`)
- Added `wifi_channel` to JSON serialization in `toJson()`
- Added `wifi_channel` parsing in `fromJson()` with default value of 1

#### 3. Coordinator (`coordinator/src/core/Coordinator.cpp`)
```cpp
// Get current WiFi channel and include in join_accept
uint8_t currentChannel = 1;
wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
esp_wifi_get_channel(&currentChannel, &second);

JoinAcceptMessage accept;
accept.wifi_channel = currentChannel; // Tell node which channel to use
```

#### 4. Node (`node/src/main.cpp`)
```cpp
case MessageType::JOIN_ACCEPT: {
    JoinAcceptMessage* accept = static_cast<JoinAcceptMessage*>(message);
    
    // Switch to coordinator's WiFi channel
    if (accept->wifi_channel > 0 && accept->wifi_channel != coordChannel) {
        coordChannel = accept->wifi_channel;
        esp_wifi_set_channel(coordChannel, WIFI_SECOND_CHAN_NONE);
    }
    ...
}
```

## How It Works

1. **Coordinator boots**: Connects to WiFi router on channel X (e.g., channel 6)
2. **Node boots**: Starts on default channel 1, enters pairing mode
3. **Pairing window opens**: Coordinator listens for JOIN_REQUEST on its channel
4. **Node scans**: Finds coordinator via beacon, learns coordinator is on channel X
5. **Node switches**: Temporarily switches to channel X to send JOIN_REQUEST
6. **Coordinator responds**: Sends JOIN_ACCEPT with `wifi_channel: X`
7. **Node locks in**: Permanently switches to channel X and stays there
8. **Communication established**: Both devices now on same channel

## Testing
1. Flash updated firmware to both coordinator and node
2. Erase NVS on node to trigger fresh pairing
3. Verify in serial logs:
   - Coordinator: `Adding peer XX:XX:XX:XX:XX:XX on channel 6`
   - Node: `Switching to coordinator's WiFi channel: 6`
   - Node: `Channel now: 6`
4. Confirm telemetry flows without channel errors

## Status
✅ **FIXED** - Nodes now automatically match coordinator's WiFi channel during pairing
