# ESP32 Migration Guide

## Overview
The code has been updated to work with **ESP32 Dev Kit V1** instead of ESP8266. The main improvement is using **Hardware Serial2** instead of SoftwareSerial for more reliable radar communication.

## Why ESP32?

### Problems with ESP8266
- **SoftwareSerial limitations**: Not as reliable for high-speed communication (115200 baud)
- **Missed data**: SoftwareSerial can drop bytes, especially with concurrent WiFi operations
- **Single hardware UART**: Serial0 is used for USB/debugging

### Benefits of ESP32
- ✅ **Multiple hardware UARTs**: 3 hardware serial ports (Serial, Serial1, Serial2)
- ✅ **Reliable communication**: No data loss at 115200 baud
- ✅ **Better performance**: Dual-core processor
- ✅ **More memory**: 520KB SRAM vs 80KB on ESP8266
- ✅ **Bluetooth support**: Future expansion possibility

---

## Hardware Changes

### Pin Connections

#### ESP32 Dev Kit V1 to LD1125H Radar

| ESP32 Pin | LD1125H Pin | Function | Notes |
|-----------|-------------|----------|-------|
| **3.3V** or **5V** | Pin 1 (VCC) | Power | Radar accepts 3.3-5V |
| **GND** | Pin 2 (GND) | Ground | Common ground |
| **GPIO 16** | Pin 4 (UTX) | RX (receive) | Serial2 RX |
| **GPIO 17** | Pin 3 (URX) | TX (transmit) | Serial2 TX |

**Important**:
- Cross the connections: ESP32 RX → Radar TX, ESP32 TX → Radar RX
- Serial2 uses GPIO16 (RX) and GPIO17 (TX) by default
- You can use different pins if needed by modifying the code

#### Wiring Diagram
```
LD1125H Radar                    ESP32 Dev Kit V1
┌─────────────┐                  ┌──────────────┐
│             │                  │              │
│  1. VCC ────┼──────────────────┼─ 5V or 3.3V │
│  2. GND ────┼──────────────────┼─ GND         │
│  3. URX ────┼──────────────────┼─ GPIO 17 (TX)│
│  4. UTX ────┼──────────────────┼─ GPIO 16 (RX)│
│             │                  │              │
└─────────────┘                  └──────────────┘
                                 │              │
                                 └─ USB ← Debug │
```

### Custom Pin Configuration

If you need to use different pins, modify these lines in the code:

```cpp
// Change these to your preferred GPIO pins
#define RADAR_RX 16  // Change to your RX pin
#define RADAR_TX 17  // Change to your TX pin
```

**Available GPIO pins on ESP32**:
- GPIO 0-5, 12-15, 16-19, 21-23, 25-27, 32-33

**Avoid these pins**:
- GPIO 6-11: Connected to flash
- GPIO 34-39: Input only, cannot be used for TX

---

## Code Changes

### 1. Library Includes
**Before (ESP8266)**:
```cpp
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
SoftwareSerial radarSerial(12, 14);
```

**After (ESP32)**:
```cpp
#include <WiFi.h>
#include <WebServer.h>
#define RADAR_RX 16
#define RADAR_TX 17
HardwareSerial radarSerial(2);
```

### 2. Serial Initialization
**Before (ESP8266)**:
```cpp
radarSerial.begin(115200);
```

**After (ESP32)**:
```cpp
radarSerial.begin(115200, SERIAL_8N1, RADAR_RX, RADAR_TX);
```

### 3. WiFi Functions
No changes needed! The following work identically on ESP32:
- `WiFi.mode(WIFI_AP)`
- `WiFi.softAP(ssid, password)`
- `WiFi.softAPIP()`

### 4. EEPROM
No changes needed! Works the same way:
- `EEPROM.begin(512)`
- `EEPROM.put()`, `EEPROM.get()`
- `EEPROM.commit()`

---

## Arduino IDE Setup

### 1. Install ESP32 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32"
6. Install "**esp32 by Espressif Systems**"

### 2. Select Board

1. Go to **Tools → Board → ESP32 Arduino**
2. Select "**ESP32 Dev Module**" or "**DOIT ESP32 DEVKIT V1**"

### 3. Configure Settings

```
Board: "ESP32 Dev Module"
Upload Speed: "115200"
CPU Frequency: "240MHz"
Flash Frequency: "80MHz"
Flash Mode: "QIO"
Flash Size: "4MB (32Mb)"
Partition Scheme: "Default 4MB with spiffs"
Core Debug Level: "None"
PSRAM: "Disabled"
Port: [Select your COM port]
```

### 4. Install Required Libraries

All libraries are already compatible with ESP32:
- **ArduinoJson** (install from Library Manager)
- WiFi, WebServer, EEPROM (built-in with ESP32 core)

---

## Uploading Code

### 1. Connect ESP32
- Connect ESP32 to computer via USB cable
- Drivers should install automatically (CP2102 or CH340)

### 2. Upload
- Click **Upload** button
- Some boards require holding the "BOOT" button during upload
- Wait for "Hard resetting via RTS pin..."

### 3. Monitor Serial Output
- Open Serial Monitor (115200 baud)
- Press **EN** (reset) button on ESP32
- You should see:

```
========================================
LD1125H Occupancy Sensor Web Server - ESP32
========================================
[INIT] Radar serial initialized at 115200 baud (RX=GPIO16, TX=GPIO17)
[WiFi] IP address: 192.168.4.1
```

---

## Troubleshooting

### "No radar data received"

**Check wiring**:
- Verify RX/TX are crossed (ESP32 RX → Radar TX)
- Check power connection (radar LED should be on)
- Ensure common ground

**Check Serial Monitor**:
```
[RADAR] Raw data: occ, dis=1.50  ← Good!
```

If you don't see `[RADAR] Raw data:` messages, the radar isn't communicating.

**Try different pins**:
```cpp
#define RADAR_RX 25  // Try GPIO 25
#define RADAR_TX 26  // Try GPIO 26
```

### "Upload failed" or "Timed out"

1. **Hold BOOT button** during upload
2. Check correct **COM port** is selected
3. Try lower **upload speed** (115200 → 9600)
4. Install **USB drivers** (CP2102 or CH340)

### "WiFi not working"

ESP32 WiFi works the same as ESP8266. Check:
- Serial Monitor shows: `[WiFi] IP address: 192.168.4.1`
- SSID "LD1125H_Sensor" appears in WiFi list
- Connect with password "12345678"

### "Brownout detector was triggered"

**Power issue**:
- Use quality USB cable
- Try external 5V power supply
- Add 100µF capacitor between VCC and GND

---

## Performance Comparison

| Feature | ESP8266 | ESP32 |
|---------|---------|-------|
| CPU | 80 MHz | 240 MHz (dual-core) |
| RAM | 80 KB | 520 KB |
| Flash | 4 MB | 4 MB |
| UARTs | 1.5 (Serial + SoftwareSerial) | 3 hardware |
| Radar reliability | ⚠️ Occasional drops | ✅ Perfect |
| WiFi speed | Good | Better |
| Power consumption | ~80mA | ~160mA |
| Price | $ | $$ |

---

## Testing Checklist

After migrating to ESP32, verify:

- [ ] Code compiles without errors
- [ ] Serial Monitor shows startup messages
- [ ] WiFi Access Point created (SSID visible)
- [ ] Can connect to http://192.168.4.1
- [ ] Web interface loads correctly
- [ ] Radar data appears: `[RADAR] Raw data: ...`
- [ ] Movement detection works: `[DETECT] MOVING at X.Xm`
- [ ] Stationary detection works: `[DETECT] STATIONARY at X.Xm`
- [ ] State changes reported: `[STATE CHANGE] IMMEDIATE UPDATE`
- [ ] Configuration can be saved
- [ ] Settings persist after reset
- [ ] Test mode shows signal strength

---

## Advanced: Using Different Serial Ports

ESP32 has 3 hardware serial ports. You can use any of them:

### Serial (UART0)
```cpp
// Used for USB/Debug - don't change this
Serial.begin(115200);
```

### Serial1 (UART1)
```cpp
// Default pins: RX=GPIO9, TX=GPIO10 (usually used for flash)
// Can be remapped to other pins
HardwareSerial radarSerial(1);
radarSerial.begin(115200, SERIAL_8N1, 25, 26); // RX=25, TX=26
```

### Serial2 (UART2) - **Recommended**
```cpp
// Default pins: RX=GPIO16, TX=GPIO17
HardwareSerial radarSerial(2);
radarSerial.begin(115200, SERIAL_8N1, 16, 17);
```

---

## Backward Compatibility

**ESP8266 code will NOT work** on ESP32 without modifications due to:
- Different library names (`ESP8266WiFi.h` vs `WiFi.h`)
- Different WebServer class
- SoftwareSerial not needed

However, **all features** remain the same:
- ✅ Same web interface
- ✅ Same API endpoints
- ✅ Same configuration options
- ✅ Same EEPROM storage
- ✅ Same radar commands

---

## Next Steps

1. **Test basic functionality**: Upload code and verify serial output
2. **Test radar communication**: Wave hand in front of sensor
3. **Test web interface**: Access http://192.168.4.1
4. **Test state changes**: Monitor Serial for state transitions
5. **Configure sensitivity**: Adjust mth1/mth2/mth3 if needed
6. **Add external reporting**: Implement HTTP/MQTT in `reportStateChange()`

---

## Getting Help

If you encounter issues:

1. **Check Serial Monitor** at 115200 baud for error messages
2. **Review wiring**: Verify all connections are correct
3. **Test radar separately**: Connect directly to USB-Serial adapter
4. **Check power supply**: Ensure stable 5V power
5. **Verify ESP32 board**: Try blinking LED example first
6. **Read debug logs**: Look for [ERROR] or [RADAR] messages

All debugging features from the ESP8266 version are retained in the ESP32 version!

---

## Summary

✅ **Switched to ESP32 Dev Kit V1**
✅ **Using Hardware Serial2** (GPIO16 RX, GPIO17 TX)
✅ **Reliable radar communication** at 115200 baud
✅ **All features retained** from ESP8266 version
✅ **Better performance** and stability
✅ **Same web interface** and API

The migration is complete and ready to use!
