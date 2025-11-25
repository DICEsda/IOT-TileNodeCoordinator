# ESP-NOW Channel Fix

## Problem
The node and coordinator were getting "Peer channel is not equal to the home channel, send fail!" error because:
- Node was hardcoded to WiFi channel 1
- Coordinator connects to WiFi router which could be on any channel (1-13)
- ESP-NOW requires both devices to be on the **same** WiFi channel

## Solution Implemented

### Coordinator Changes (`coordinator/src/comm/EspNow.cpp`)
- **Dynamic Channel Detection**: When adding a peer, coordinator now reads its current WiFi channel using `esp_wifi_get_channel()`
- This ensures peers are registered on the actual router channel, not hardcoded channel 1
- Added logging to show which channel peers are being added on

### Node Changes (`node/src/main.cpp`)
- **WiFi Scanning**: Node now scans for WiFi networks before initializing ESP-NOW
- Uses the strongest network's channel as the coordinator channel
- This works because the coordinator connects to a local router, and the node can detect that router's channel
- Falls back to channel 1 if no networks found

## How It Works
1. Coordinator boots and connects to WiFi router (e.g., channel 6)
2. Node boots and scans WiFi networks
3. Node finds the router on channel 6 (strongest signal)
4. Node sets its WiFi channel to 6
5. Node sends JOIN_REQUEST to coordinator
6. Coordinator adds node as peer on channel 6 (its current channel)
7. Communication succeeds! ✓

## Testing
After flashing:
1. Monitor coordinator serial output - look for "Adding peer ... on channel X"
2. Monitor node serial output - look for "Using strongest network channel: X"
3. Verify both show the same channel number
4. Node telemetry should now reach coordinator successfully

## Alternative Approaches (if current solution doesn't work)
If the WiFi scanning approach fails, consider:
1. **Broadcast beacon**: Have coordinator broadcast its channel number
2. **Channel hopping**: Node cycles through all channels until it finds coordinator
3. **Fixed channel mode**: Set router to channel 1 permanently

## Related Files
- `coordinator/src/comm/EspNow.cpp` - Line 503+ (addPeer function)
- `node/src/main.cpp` - Line 642+ (WiFi scanning and channel setup)
