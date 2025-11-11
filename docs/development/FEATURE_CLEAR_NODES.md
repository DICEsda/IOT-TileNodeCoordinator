# Feature: Clear All Nodes (10-Second Hold)

## Overview
Added a "factory reset" feature that clears all paired nodes when the pairing button is held for 10 seconds.

## Usage
1. **Press and hold** the pairing button on the coordinator
2. **Wait for 4 seconds** - the normal long press (flash test) will trigger
3. **Continue holding for 6 more seconds** (total 10 seconds)
4. When 10 seconds is reached, all nodes will be cleared
5. **Release the button**

## What Gets Cleared
- All registered nodes in NodeRegistry
- All ESP-NOW peers
- Node-to-LED group mappings
- All persisted data in NVS (non-volatile storage)
- Connection status indicators on the LED strip

## Visual Feedback
- Serial console will show:
  ```
  ===========================================
  CLEARING ALL NODES (10s hold detected)
  ===========================================
  Clearing all ESP-NOW peers...
  Cleared N ESP-NOW peers
  Cleared all nodes (count: N)
  All nodes cleared. Release button to continue.
  ===========================================
  ```
- LED strip will update to show no connected nodes

## Implementation Details

### Files Modified

1. **ButtonControl.h/cpp**
   - Added `VERY_LONG_PRESS_MS = 10000` constant
   - Added `setVeryLongPressCallback()` method
   - Added `veryLongPressTriggered` state tracking
   - Detection hierarchy: 4s (long press) → 10s (very long press)

2. **NodeRegistry.h/cpp**
   - Added `clearAllNodes()` method
   - Clears in-memory registry and NVS storage
   - Logs number of nodes cleared

3. **EspNow.h/cpp**
   - Added `clearAllPeers()` method
   - Removes all peers from ESP-NOW subsystem
   - Clears peer statistics
   - Clears NVS peer storage

4. **Coordinator.cpp**
   - Wired up `veryLongPressCallback` to clear operation
   - Calls `clearAllNodes()` on NodeRegistry
   - Calls `clearAllPeers()` on EspNow
   - Rebuilds LED mapping and updates display

## Button Press Timing
- **< 4 seconds**: Short press → Opens pairing window
- **≥ 4 seconds**: Long press → Flash test on all nodes (while held)
- **≥ 10 seconds**: Very long press → Clear all nodes (one-time trigger)

## Safety Notes
- **Non-destructive to firmware**: Only clears pairing data
- **Requires deliberate action**: 10-second hold prevents accidental triggering
- **Logged operation**: All actions are logged to serial console
- **Nodes unaffected**: Nodes retain their configuration; they simply need to re-pair

## Recovery
After clearing:
1. Nodes will still have their stored configuration
2. Press the pairing button (short press) to open pairing window
3. Nodes will need to be put into pairing mode and will re-pair
4. New light IDs will be assigned based on their MAC addresses

## Use Cases
- Development/testing: Quick reset between test runs
- Troubleshooting: Clear corrupted pairing data
- Deployment: Factory reset before shipping
- Maintenance: Start fresh after hardware changes
