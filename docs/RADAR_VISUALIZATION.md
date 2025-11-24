# mmWave Radar Visualization - Sector-Based Display

## Overview
Updated the radar visualization to accurately represent the limited field of view (FOV) of mmWave sensors instead of showing a full 360° circular scan.

## Changes Made

### Previous Behavior
- Radar displayed as full 360° circle
- Sweep line rotated continuously around entire circle
- Gave impression of omnidirectional coverage

### Current Behavior
- Radar displays as 120° sector (±60° from center)
- Sweep line oscillates within FOV range only
- Accurately represents mmWave sensor capabilities
- FOV boundary lines clearly visible

## Technical Details

### Field of View Configuration
**File**: `room-visualizer.component.ts`

```typescript
private readonly FOV_ANGLE = (120 * Math.PI) / 180; // 120° field of view
private sweepAngle = -(120 * Math.PI / 180) / 2 - Math.PI / 2; // Start at left edge
```

### Visual Elements

#### Range Arcs
- Only drawn within 120° sector
- 4 concentric arcs showing distance increments
- Labels show distance in meters (0.25m, 0.5m, 0.75m, 1.0m intervals up to max range)

#### FOV Boundary Lines
- Two prominent lines at ±60° from center
- Thicker stroke (2px) with higher opacity
- Angle labels "60°" at each boundary
- Clearly demarcates sensor coverage area

#### Center Axis
- Vertical line from origin to max range
- Shows sensor forward-facing direction
- Reference point for angular measurements

#### Sweep Animation
- 5° wide sweep beam (matching original)
- Sweeps only within FOV range (left to right)
- Resets to left boundary when reaching right edge
- Continuous scanning pattern

### Coordinate System
- Origin at bottom center of sector
- Y-axis points upward (sensor forward direction)
- X-axis spans left (-60°) to right (+60°)
- Angles measured from vertical (standard polar convention)

## mmWave Sensor Specifications

### Typical FOV Values
- **Horizontal**: 120° (±60°) - implemented
- **Vertical**: 30° (±15°) - not visualized (2D view)
- **Range**: 0.2m to 6m configurable

### Detection Characteristics
- Targets within FOV show as glowing markers
- Position based on actual X/Y coordinates from sensor
- Trail effect shows movement history
- Out-of-FOV targets not displayed (realistic behavior)

## Visual Design

### Color Scheme (unchanged)
- Background: Dark blue gradient (#06121f to #091a2b)
- Grid lines: Cyan/teal rgba(0, 255, 191, 0.15)
- Sweep beam: Radial gradient with cyan highlight
- FOV boundaries: rgba(0, 255, 191, 0.3) - more prominent
- Target markers: Bright cyan with glow effect

### Layout
- Sensor positioned at bottom center
- Coverage area fans upward
- Realistic representation of wall/ceiling-mounted sensor
- Natural viewing angle for room monitoring

## Benefits

1. **Accurate representation** - Matches real mmWave sensor limitations
2. **Clearer expectations** - Users understand coverage area
3. **Professional appearance** - Industry-standard radar display
4. **Better UX** - Dead zones visible, no false sense of omnidirectional coverage
5. **Educational** - Teaches users about sensor technology

## Future Enhancements

### Configurable FOV
Allow FOV angle to be configured per sensor model:
```typescript
@Input() fovAngle: number = 120; // degrees
```

### Multiple Sensors
Support visualization of multiple sensors with overlapping coverage:
- Different colors per sensor
- Coverage overlap highlighting
- Aggregate target tracking

### Vertical FOV
Add 3D perspective showing vertical coverage:
- Side-view radar display
- Height-based target filtering
- Ceiling/floor proximity warnings

### Dead Zone Indicators
Show areas explicitly outside coverage:
- Shaded regions beyond FOV
- Warning indicators for critical blind spots
- Sensor placement recommendations

## Testing

### Verify Display
1. Navigate to `http://localhost:4200`
2. Open Live Monitor tab
3. Observe radar display:
   - Should show 120° sector (not full circle)
   - FOV boundary lines at ±60°
   - Sweep oscillates left-to-right only
   - Grid arcs follow sector shape

### With Live Data
1. Connect mmWave sensor
2. Move within FOV (±60° from center)
3. Targets should appear within sector
4. Movement outside FOV should not show targets
5. Trail effect shows path within coverage area

## Related Files

- `room-visualizer.component.ts` - Main radar rendering logic
- `room-visualizer.component.html` - Canvas element
- `room-visualizer.component.scss` - Container styling
- `api.models.ts` - MmwaveFrame and MmwaveTarget interfaces

## Notes

- FOV angle (120°) is typical for common mmWave modules
- Adjust `FOV_ANGLE` constant if using different sensor
- Coordinate transformation assumes sensor faces upward (Y-axis)
- Range circles scaled to `maxRangeMm` (default 6000mm = 6m)
