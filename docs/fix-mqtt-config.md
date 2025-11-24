# Fix MQTT Connection Issue

## Problem
Coordinator has wrong MQTT broker IP stored: `10.140.61.195`  
Should be: `10.103.41.195` (your PC's WiFi IP on the hotspot)

## Solution: Reconfigure via Serial

### Step 1: Clear MQTT config and reboot
In the serial monitor, send these commands:

```
erase mqtt
reboot
```

### Step 2: When prompted, enter new broker IP
After reboot, it will prompt:
```
Configure MQTT broker now? (y/n): y
MQTT broker IP/hostname: 10.103.41.195
MQTT broker port [1883]: 1883
MQTT username [user1]: user1
MQTT password: user1
Site ID [site001]: site001
```

### Step 3: Verify connection
You should see:
```
MQTT broker target set to 10.103.41.195:1883
MQTT connected successfully
```

## Additional Issues to Check

### TSL2561 Lux Sensor (Reading 0.0)
1. **Verify wiring:**
   - TSL2561 VCC → ESP32 3.3V
   - TSL2561 GND → ESP32 GND
   - TSL2561 SDA → ESP32 GPIO17
   - TSL2561 SCL → ESP32 GPIO18

2. **Check I2C address:**
   - TSL2561 default: 0x39 (ADDR floating)
   - Alternative: 0x29 (ADDR → GND) or 0x49 (ADDR → VCC)

3. **If sensor not found:**
   - Try different I2C address in code
   - Check for loose connections
   - Verify sensor is TSL2561 (not TSL2560)

### mmWave Sensor (Excessive Restarts)
Current: 90+ restarts indicates:
- Possible loose UART connection
- Power supply issues
- Interference from other devices

Check:
- LD2450 RX → ESP32 GPIO44
- LD2450 TX → ESP32 GPIO43
- Stable 5V power supply
