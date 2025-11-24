# Settings Tab Visual Guide

## 🎨 User Interface Overview

### 1. Password Protection Screen
```
┌────────────────────────────────────────────┐
│                                            │
│              🔒 Lock Icon                  │
│                                            │
│           Settings Access                  │
│   Enter password to access system settings │
│                                            │
│   ┌──────────────────────────────────┐   │
│   │  Enter password...                │   │
│   └──────────────────────────────────┘   │
│                                            │
│   ┌──────────────────────────────────┐   │
│   │     Access Settings              │   │
│   └──────────────────────────────────┘   │
│                                            │
└────────────────────────────────────────────┘
```

**Features:**
- Centered authentication screen
- Lock icon (SVG)
- Password input (masked)
- Error message display
- Access button

---

### 2. Main Settings Interface (After Authentication)

```
┌─────────────────────────────────────────────────────────────────────┐
│ ⚙️ System Settings         [✓] Developer Mode                       │
├─────────────────────────────────────────────────────────────────────┤
│  [Success: Settings saved]  [Save All] [Reset] [Lock]              │
└─────────────────────────────────────────────────────────────────────┘

┌───────────────┬───────────────┬───────────────┐
│   Section 1   │   Section 2   │   Section 3   │  ← Grid Layout
├───────────────┼───────────────┼───────────────┤
│   Section 4   │   Section 5   │   Section 6   │
├───────────────┼───────────────┼───────────────┤
│   Section 7   │   Section 8   │   Section 9   │
└───────────────┴───────────────┴───────────────┘
```

**Header Elements:**
- Page title with gear icon
- Developer mode toggle checkbox
- Save message (appears temporarily)
- Action buttons: Save All, Reset, Lock

**Layout:**
- Responsive grid (3 columns on desktop)
- Adapts to 2 columns on tablet
- Single column on mobile
- Each section is a card

---

### 3. Section Structure (Collapsible)

```
┌────────────────────────────────────────────────────┐
│ 👤 System Identity              [Save Section]    │ ← Click to expand/collapse
├────────────────────────────────────────────────────┤
│                                                    │
│  Default Site ID                                   │
│  ┌──────────────────────────────────────────┐    │
│  │ site001                                   │    │
│  └──────────────────────────────────────────┘    │
│                                                    │
│  Default Coordinator ID                            │
│  ┌──────────────────────────────────────────┐    │
│  │ coord001                                  │    │
│  └──────────────────────────────────────────┘    │
│                                                    │
│  Max Coordinators Per Site                         │
│  ┌──────────────────────────────────────────┐    │
│  │ 10                                        │    │
│  └──────────────────────────────────────────┘    │
│                                                    │
│  Max Nodes Per Coordinator                         │
│  ┌──────────────────────────────────────────┐    │
│  │ 250                                       │    │
│  └──────────────────────────────────────────┘    │
│                                                    │
└────────────────────────────────────────────────────┘
```

**Section Features:**
- Collapsible header with icon
- Individual "Save Section" button
- Form controls (inputs, toggles, sliders)
- Smooth expand/collapse animation

---

### 4. Control Types

#### Text Input
```
Label Name
┌──────────────────────────────────┐
│ Current value...                 │
└──────────────────────────────────┘
```

#### Number Input with Slider
```
Brightness (128/255)
┌──────────────────────────────────┐
│ 128                              │
└──────────────────────────────────┘
●─────────────────○──────────────── ← Slider
0                128              255
```

#### Toggle Switch
```
Feature Name                    ⚪─────  OFF
Feature Name                    ────⚪   ON
```

#### Dropdown Select
```
MQTT QoS Level
┌──────────────────────────────────┐
│ QoS 1 - At least once        ▼  │
└──────────────────────────────────┘
```

#### Read-Only Field
```
API URL (read-only)
┌──────────────────────────────────┐
│ http://localhost:3000   🔒       │
└──────────────────────────────────┘
```

#### Zone List Management
```
Default Zones

┌───────────────────────────────┬───┐
│ Living Room                   │ ✕ │
└───────────────────────────────┴───┘
┌───────────────────────────────┬───┐
│ Bedroom                       │ ✕ │
└───────────────────────────────┴───┘
┌───────────────────────────────┬───┐
│ Kitchen                       │ ✕ │
└───────────────────────────────┴───┘

┌─────────────────────────────────────┐
│        + Add Zone                    │
└─────────────────────────────────────┘
```

---

## 📱 Responsive Behavior

### Desktop (1920px)
```
┌─────────┬─────────┬─────────┐
│ Section │ Section │ Section │
│    1    │    2    │    3    │
├─────────┼─────────┼─────────┤
│ Section │ Section │ Section │
│    4    │    5    │    6    │
├─────────┼─────────┼─────────┤
│ Section │ Section │ Section │
│    7    │    8    │    9    │
└─────────┴─────────┴─────────┘
```

### Tablet (768px)
```
┌───────────┬───────────┐
│  Section  │  Section  │
│     1     │     2     │
├───────────┼───────────┤
│  Section  │  Section  │
│     3     │     4     │
├───────────┼───────────┤
│  Section  │  Section  │
│     5     │     6     │
├───────────┼───────────┤
│  Section  │  Section  │
│     7     │     8     │
├───────────┴───────────┤
│      Section 9        │
└───────────────────────┘
```

### Mobile (375px)
```
┌─────────────────┐
│    Section 1    │
├─────────────────┤
│    Section 2    │
├─────────────────┤
│    Section 3    │
├─────────────────┤
│    Section 4    │
├─────────────────┤
│    Section 5    │
├─────────────────┤
│    Section 6    │
├─────────────────┤
│    Section 7    │
├─────────────────┤
│    Section 8    │
├─────────────────┤
│    Section 9    │
└─────────────────┘
```

---

## 🎭 UI States

### Loading State
```
┌────────────────────────────────┐
│                                │
│         ⟳ Spinner              │
│                                │
│    Loading settings...         │
│                                │
└────────────────────────────────┘
```

### Success Message
```
┌────────────────────────────────────┐
│ ✓ Settings saved successfully!    │  ← Green background, fades after 3s
└────────────────────────────────────┘
```

### Error Message
```
┌────────────────────────────────────┐
│ ✗ Failed to save settings          │  ← Red background, fades after 3s
└────────────────────────────────────┘
```

### Disabled Button
```
┌─────────────────────┐
│   Saving...         │  ← Grayed out, cursor not-allowed
└─────────────────────┘
```

---

## 🔄 User Interactions

### 1. Authentication Flow
```
User arrives → Password screen
    ↓
Enter password → Validate
    ↓
Valid? → Load settings → Show main interface
    ↓
Invalid? → Show error → Stay on password screen
```

### 2. Settings Modification Flow
```
User changes value
    ↓
Value updates in UI (instant)
    ↓
User clicks "Save Section" or "Save All"
    ↓
Show loading state
    ↓
Send to backend API
    ↓
Backend saves to MongoDB
    ↓
Backend publishes MQTT update
    ↓
Show success message
    ↓
Other clients receive update via WebSocket
```

### 3. Real-Time Update Flow
```
Backend updates settings
    ↓
MQTT message published
    ↓
WebSocket forwards to frontend
    ↓
Frontend receives update
    ↓
UI automatically refreshes (no page reload)
    ↓
User sees new values
```

### 4. Developer Mode Toggle Flow
```
User clicks "Enable Developer Mode"
    ↓
Checkbox toggles ON
    ↓
Developer Tools section appears at bottom
    ↓
User can now edit developer settings
    ↓
User clicks checkbox OFF
    ↓
Developer Tools section disappears
```

### 5. Zone Management Flow
```
Add Zone:
  Click "+ Add Zone" → Enter name → Zone added to list

Edit Zone:
  Click in zone name input → Type new name → Update on blur

Remove Zone:
  Click "✕" button → Confirm → Zone removed from list
```

---

## 🎨 Color Scheme

```
Primary Background:   var(--bg-primary)    Dark blue-gray
Secondary Background: var(--bg-secondary)  Slightly lighter
Tertiary Background:  var(--bg-tertiary)   Input backgrounds
Border Color:         var(--border-color)  Subtle borders
Text Primary:         var(--text-primary)  White/Light
Text Secondary:       var(--text-secondary) Muted gray
Text Muted:           var(--text-muted)    Dimmed gray
Accent Primary:       var(--accent-primary) Cyan (#00FFBF)
Accent Secondary:     var(--accent-secondary) Purple
Accent Success:       var(--accent-success) Green
Accent Danger:        var(--accent-danger)  Red
Accent Warning:       var(--accent-warning) Yellow/Orange
```

---

## 📐 Spacing & Sizing

```
Card Padding:      1.5rem
Section Gap:       1.5rem
Input Height:      40px
Button Height:     36px
Border Radius:     8px (cards), 6px (inputs)
Font Size (Label): 0.875rem
Font Size (Input): 0.875rem
Icon Size:         20px (section headers)
Icon Size:         16px (buttons)
```

---

## 🔔 Notifications

### Success Notification
```
┌─────────────────────────────────────────┐
│ ✓ Automation settings saved successfully│  ← Appears top-right of header
└─────────────────────────────────────────┘  ← Fades after 3 seconds
     Soft green background
     White text
     Subtle slide-in animation
```

### Error Notification
```
┌─────────────────────────────────────────┐
│ ✗ Failed to save network settings       │  ← Appears top-right of header
└─────────────────────────────────────────┘  ← Fades after 3 seconds
     Soft red background
     White text
     Subtle shake animation
```

---

## 🎯 Section Icons

```
System Identity:     👤 User icon
Network:             📡 WiFi icon
MQTT:                📊 Activity icon
Coordinator:         📦 Box with target icon
Node:                🎯 Concentric circles
Zones:               🏠 3D box icon
Automation:          ⚡ Lightning bolt
Telemetry:           📄 Document icon
Developer:           👨‍💻 Code brackets icon
```

---

## ⌨️ Keyboard Navigation

```
Tab:        Navigate between inputs
Shift+Tab:  Navigate backwards
Enter:      Submit form / Save
Space:      Toggle switches
Arrow keys: Adjust sliders
Esc:        Close modals (if any)
```

---

## 📊 Data Flow Diagram

```
┌──────────┐
│ Frontend │
└────┬─────┘
     │ 1. Password validation
     ↓
┌──────────┐
│ Backend  │
└────┬─────┘
     │ 2. Verify password
     ↓
┌──────────┐
│   Env    │  SETTINGS_PASSWORD
└──────────┘

┌──────────┐
│ Frontend │
└────┬─────┘
     │ 3. Get settings
     ↓
┌──────────┐
│ Backend  │
└────┬─────┘
     │ 4. Query database
     ↓
┌──────────┐
│ MongoDB  │  system_settings collection
└──────────┘

┌──────────┐
│ Frontend │
└────┬─────┘
     │ 5. Update settings
     ↓
┌──────────┐
│ Backend  │
└────┬─────┴────┬──────────┐
     │          │          │
     ↓          ↓          ↓
┌──────────┐ ┌─────┐ ┌──────────┐
│ MongoDB  │ │MQTT │ │WebSocket │
└──────────┘ └──┬──┘ └────┬─────┘
                │         │
                │         ↓
                │    ┌──────────┐
                │    │Other     │
                │    │Clients   │
                │    └──────────┘
                ↓
           ┌──────────┐
           │Coordinator│
           │  /Node    │
           └──────────┘
```

---

## 🎬 Animation Details

### Section Expand/Collapse
- Duration: 0.3s ease-out (collapse), 0.5s ease-in (expand)
- Property: max-height
- Smooth transition

### Save Message
- Fade in: 0.3s
- Stay: 3s
- Fade out: 0.3s
- Slide animation: translateY(-10px) to translateY(0)

### Loading Spinner
- Rotation: 360° per 1s
- Smooth infinite loop
- CSS animation

### Button Hover
- Transform: translateY(-1px)
- Duration: 0.2s
- Smooth transition

---

## 💡 Tips for Users

1. **Use Individual Save**: Save sections one at a time if making many changes
2. **Developer Mode**: Only visible when checkbox is enabled
3. **Zones**: Add zones as needed, no limit (up to max_zones)
4. **Read-Only Fields**: Cannot be edited (API URL, firmware version)
5. **Lock Button**: Re-secure settings when done
6. **Reset**: Confirmation required, cannot be undone

---

## 🚀 Performance Considerations

- Collapsible sections reduce initial render load
- WebSocket updates are throttled to prevent UI flicker
- Form values are debounced for sliders
- MongoDB queries are indexed
- MQTT messages use QoS 1 for reliability

---

This visual guide complements the technical documentation and provides a clear picture of the user interface and user experience.
