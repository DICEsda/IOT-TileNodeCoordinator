# Fix: Set Coordinator ID to coord001

## Problem
The coordinator is using MAC address `74:4D:BD:AB:A9:F4` as its ID because `coord_id` is empty in NVS.
The frontend is hardcoded to look for `coord001`, causing a 404 error.

## Solution
Set the coordinator ID via serial console

### Steps:

1. **Open serial monitor** to coordinator (baudrate 115200)

2. **Trigger MQTT reconfiguration:**
   - Press any key during boot when you see "No Wi-Fi... Configure? y/n"
   - OR manually reset NVS flash and reboot

3. **During MQTT setup wizard**, when prompted:
   ```
   Coordinator ID (blank = use MAC): coord001
   ```

4. **Restart coordinator** - it will now publish to:
   - `site/site001/coord/coord001/telemetry`
   - `site/site001/coord/coord001/mmwave`  
   - Subscribe to: `site/site001/coord/coord001/cmd`

5. **Verify** backend receives telemetry and creates coordinator in database

## Alternative: Manual NVS fix via PlatformIO
```bash
cd coordinator
pio run -t erase  # Erase flash
pio run -t upload # Re-flash
# Then go through setup wizard and set coord_id = coord001
```
