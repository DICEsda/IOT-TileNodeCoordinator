# Pairing Visual Feedback Improvements

## Overview
Improved the LED visual feedback during pairing to make it clearer, more predictable, and provide better status indication.

## Changes Made

### 1. Node Pairing LED (Fixed Erratic Behavior)
**File:** `node/src/led/LedController.cpp`

**Before:**
- Fast, erratic blue breathing (1.6s cycle)
- Brightness varied wildly: 40-180
- Color intensity also varied: 80-255 blue
- Made it hard to tell what state the node was in

**After:**
- Slow, calm blue breathing (2.0s cycle)
- Stable brightness range: 50-120
- Fixed blue color intensity: 200 (only brightness varies)
- Smooth, predictable sine wave animation
- Much easier to recognize pairing state

### 2. Green Flash on Node When Sending JOIN_REQUEST
**Files:** `node/src/main.cpp` (multiple locations)

**New Behavior:**
- **50ms green flash** every time node sends a JOIN_REQUEST
- Provides clear visual confirmation that node is actively trying to pair
- Helps troubleshoot connectivity issues (if you don't see green flashes, node isn't sending)
- Flash is bright (120 brightness) for visibility
- Automatically restores pairing animation after flash

**Locations:**
- `sendJoinRequestNow()` - Direct manual send
- `handlePairing()` - Periodic automatic sends
- Response to pairing beacon

### 3. Coordinator Visual Feedback

#### A. Blue Pulse When Pairing Mode Opens
**File:** `coordinator/src/core/Coordinator.cpp`

**Behavior:**
- When pairing button pressed (short press)
- **500ms blue pulse** on all coordinator LEDs
- Confirms pairing window is now open
- Color: RGB(0, 0, 180)

#### B. Green "OK" Flash When Pairing Succeeds
**File:** `coordinator/src/core/Coordinator.cpp`

**Behavior:**
- When a node successfully pairs
- **300ms green pulse** on all coordinator LEDs
- Clear visual confirmation of successful pairing
- Color: RGB(0, 150, 0)
- Also extends the per-node LED flash to 400ms (from 200ms)

## Visual Sequence Summary

### Node Side (During Pairing):
1. **Steady blue breathing** - Node is in pairing mode, waiting
2. **Quick green flash** - Node sends JOIN_REQUEST
3. **Bright green flash (120ms)** - JOIN_ACCEPT received, pairing successful!
4. **Green trailing wave** - Connected and operational

### Coordinator Side (During Pairing):
1. **Blue pulse (500ms)** - Pairing window opened
2. **Green "OK" pulse (300ms)** - Node successfully paired!
3. **Per-node LED indicator turns green** - Shows which node just connected

## Timing Details

| Event | Node LED | Coordinator LED | Duration |
|-------|----------|----------------|----------|
| Pairing mode active | Blue breathing (2s cycle) | Per-node indicators | Continuous |
| JOIN_REQUEST sent | Green flash | - | 50ms |
| Pairing window opened | - | Blue pulse (all LEDs) | 500ms |
| Pairing success | Bright green flash | Green "OK" pulse (all LEDs) | Node: 120ms, Coord: 300ms |
| Connected/operational | Green trailing wave | Per-node green indicator | Continuous |

## Benefits

1. **Predictability**: Steady, calm animations instead of erratic flashing
2. **Clarity**: Each state has a distinct visual pattern
3. **Feedback**: User can see when messages are being sent
4. **Confirmation**: Clear "OK" signal when pairing succeeds
5. **Troubleshooting**: Green flashes help diagnose connectivity issues

## Technical Notes

- All LED updates respect the 50% brightness cap (BRIGHTNESS_CAP = 128)
- Animations are non-blocking and use `StatusMode` enum
- Green flashes temporarily disable status animations to ensure visibility
- Coordinator uses `StatusLed::pulse()` for whole-strip effects
- Node uses per-pixel `LedController` with status mode management
