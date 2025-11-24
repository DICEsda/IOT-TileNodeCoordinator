# mmWave Radar Visualization - Before & After

## Visual Comparison

### Before (360° Circle)
```
        ◯ Target
       ╱│╲
      ╱ │ ╲
     ╱  │  ╲
    ╱   │   ╲
   │    │    │  ← Full 360° coverage (incorrect)
   │    ●────│  ← Sweep rotates full circle
   │         │
    ╲       ╱
     ╲     ╱
      ╲   ╱
       ╲_╱
```
**Issues:**
- Implies omnidirectional coverage
- Misleading for sensor placement
- Doesn't match hardware capabilities
- Creates false expectations

---

### After (120° Sector)
```
        60°  │  60°
          ╲  │  ╱
           ╲ │ ╱
        ◯───╲│╱───◯  ← FOV boundaries
      Target ●    Target
           ╱ │ ╲
          ╱  │  ╲     ← 120° coverage sector
         ╱   │   ╲
        ╱    │    ╲
       ╱     │     ╲
      ▓▓▓▓▓▓●▓▓▓▓▓▓  ← Sensor position
     
    Dead zone │ Dead zone
```
**Improvements:**
- ✓ Accurate FOV representation
- ✓ Clear coverage boundaries
- ✓ Realistic scanning pattern
- ✓ Visible dead zones

---

## Radar Display Elements

### Grid Structure
```
        6.0m ─── ◯ ←─── Range arc (within FOV only)
             ╱   │   ╲
        4.5m ─  ╱ │ ╲  ─
             ╱   │   ╲
        3.0m ─ ╱  │  ╲ ─
             ╱    │    ╲
        1.5m ─╱   │   ╲─
            ╱     │     ╲
           ▓▓▓▓▓▓●▓▓▓▓▓▓
           
    FOV boundary lines (thicker, prominent)
    Center axis (forward direction)
    Range labels (0-6m typical)
```

### Sweep Animation
```
Frame 1:              Frame 2:              Frame 3:
    60°│60°              60°│60°              60°│60°
      ╲│╱                 ╲│╱                 ╲│╱
   ≈≈≈≈●                   ●                   ●≈≈≈≈
      ╱│╲                 ╱│╲                 ╱│╲
     ▓▓●▓▓               ▓▓●▓▓               ▓▓●▓▓
   
   Sweep at left       Sweep at center      Sweep at right
   (starts here)       (moving right)       (then resets)
   
   ≈≈≈≈ = Sweep beam with gradient fade
```

---

## Target Detection Zones

### Coverage Map (Top View)
```
                  [Forward]
                      ↑
                      │
         \            │            /
          \           │           /
           \    Active Zone     /
            \    (120° FOV)    /
             \                /
              \      ●       /  ← Sensor
               \   60° 60° /
                \    │    /
                 \   │   /
    [Dead Zone]   \  │  /   [Dead Zone]
                   \ │ /
                    \│/
                     ●
                  [Sensor]
                     
    ◯ = Target detected (within FOV)
    ✗ = Target NOT detected (outside FOV)
```

### Side View (Vertical FOV - not visualized in 2D)
```
    Ceiling
    ─────────────────────
        ╱│╲  30° vertical FOV
       ╱ │ ╲ (typical)
      ╱  ●  ╲  ← Sensor
     ╱   │   ╲
    ▓▓▓▓▓▓▓▓▓▓▓
    Floor
```

---

## Real-World Mounting Example

### Wall-Mounted Configuration
```
    Ceiling
    ═══════════════════════════
           │  Room
           │
        ◯──┼──◯  ← Detected targets
           │
           │
      ╱────●────╲  ← Sensor on wall
     ╱     │     ╲
    ╱      │      ╲
   ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
   Wall
   
   Coverage fans out from wall
   Dead zones to left/right sides
```

### Ceiling-Mounted Configuration
```
    Ceiling
    ═══════════════════════════
           ●  ← Sensor
          ╱│╲
         ╱ │ ╲
        ╱  │  ╲
       ╱   │   ╲  Coverage downward
      ╱    │    ╲
     ◯─────┼─────◯
          │
    Floor
    ═══════════════════════════
    
    Coverage directly below
    Blind spots near edges
```

---

## Coordinate System Details

### Polar Coordinates (Sensor-Centric)
```
        0° (Forward)
            │
            │
    +60°    │    -60°
        \   │   /
         \  │  /
          \ │ /
           \│/
            ●  Sensor origin
           ╱│╲
          ╱ │ ╲
    -90° ─  │  ─ +90°
           ╱│╲
    Angles measured from forward axis
    
    Valid range: -60° to +60° (120° total)
```

### Cartesian Mapping
```
         Y (Range)
         ↑
         │    ◯ Target (x=2m, y=4m)
       6m│   ╱
         │  ╱
       4m│ ◯
         │╱
         ●────────→ X (Lateral)
      Origin   2m   4m
      
    Origin = Sensor position
    Y = Distance from sensor (range)
    X = Lateral offset (left/right)
```

---

## Configuration Guide

### Adjusting FOV for Different Sensors

#### Narrow FOV (60°) - Long Range
```typescript
private readonly FOV_ANGLE = (60 * Math.PI) / 180;
```
```
      30°│30°    ← Narrower beam
        ╲│╱
         ●       ← Better range
        ╱│╲
       ▓▓●▓▓
```
Use for: Hallways, corridors, focused monitoring

#### Wide FOV (180°) - Short Range
```typescript
private readonly FOV_ANGLE = (180 * Math.PI) / 180;
```
```
    90°   │   90°  ← Wider coverage
      ───╲│╱───
          ●       ← Shorter range
         ╱│╲
        ▓▓●▓▓
```
Use for: Small rooms, corner mounting, wide-area detection

---

## Performance Considerations

### Rendering Optimization
- Only draws arcs within FOV (fewer draw calls)
- Sweep resets instead of full rotation (smoother animation)
- FOV boundaries cached as constants (no recalculation)
- Target culling outside FOV (reduced processing)

### Memory Usage
- Sector rendering: ~60% less canvas operations vs. full circle
- Target trail: Only tracks positions within FOV
- Grid calculations: Simplified angular math

---

## Troubleshooting

### Targets Not Appearing
✓ Check target position is within ±60° from center axis
✓ Verify range is within 0-6m (configurable)
✓ Ensure sensor orientation matches visualization

### Sweep Not Animating
✓ Check browser supports requestAnimationFrame
✓ Verify canvas context is acquired successfully
✓ Check browser tab is active (animation pauses when hidden)

### FOV Looks Wrong
✓ Confirm FOV_ANGLE constant matches sensor specs
✓ Check sensor mounting orientation
✓ Verify coordinate system transformation
