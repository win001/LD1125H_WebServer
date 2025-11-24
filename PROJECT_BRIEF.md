# LD1125H Occupancy Sensor Web Server

## Project Overview
ESP32-based web server for monitoring and configuring an HLK-LD1125H-24G millimeter wave radar sensor for human presence detection.

## Hardware Components

### LD1125H-24G Radar Sensor
- **Technology**: 24GHz millimeter wave FMCW radar
- **Detection Range**:
  - Stationary presence: >4m
  - Moving targets: >8m
- **Coverage**: ±22° horizontal, ±24° vertical beam angle
- **Power**: 5V, 80mA
- **Interface**: TTL Serial (115200 baud, 8N1)

### ESP32 Dev Kit V1 Microcontroller
- **WiFi Mode**: Access Point (AP)
- **SSID**: `LD1125H_Sensor`
- **Password**: `12345678`
- **Serial Connection** (Hardware Serial2):
  - RX: GPIO 16
  - TX: GPIO 17
- **Debug Serial**: USB Serial (115200 baud)
- **Advantages over ESP8266**:
  - Hardware UART (no SoftwareSerial, no data loss)
  - Dual-core 240MHz processor
  - 520KB RAM vs 80KB
  - More reliable radar communication

## Key Features

### Real-Time Detection
- **Movement Detection**: Large-scale human motion
- **Stationary Presence**: Breathing and micro-movements
- **Distance Measurement**: Approximate target distance in meters
- **States**: Moving, Stationary, None

### Web Interface
Access via `http://192.168.4.1` after connecting to the sensor's WiFi network.

#### Dashboard
- Live occupancy status display
- Real-time distance readings
- Signal strength indicator (in test mode)
- Auto-refresh every 3 seconds
- Immediate state change detection

#### Configuration Panel
- **Max Detection Distance** (rmax): 0.4m to 12m (default: 8m)
- **Zone 1 Sensitivity** (mth1): 0-2.8m range (default: 60)
- **Zone 2 Sensitivity** (mth2): 2.8-8m range (default: 30)
- **Zone 3 Sensitivity** (mth3): >8m range (default: 20)
- **Timeout**: No-presence delay in seconds (default: 30s)
- **Note**: Lower sensitivity values = more sensitive detection

#### Statistics Tracking
- Total detection events
- Moving vs stationary counts
- Maximum detected distance

#### Test Mode
- Enable to view signal strength values
- Helps fine-tune sensitivity thresholds
- Command: `test_mode=1`

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Main web interface |
| `/status` | GET | Current sensor state (JSON) |
| `/config` | GET | Current configuration (JSON) |
| `/config` | POST | Update configuration |
| `/stats` | GET | Detection statistics (JSON) |
| `/testmode` | POST | Toggle test mode |
| `/reset` | POST | Reset to default settings |

## Serial Communication Protocol

### Sensor Output
```
mov, dis=2.50    # Movement detected at 2.5m
occ, dis=1.80    # Stationary presence at 1.8m
```

### Configuration Commands
```
rmax=8.0         # Set max range to 8 meters
mth1=60          # Set zone 1 sensitivity
mth2=30          # Set zone 2 sensitivity
mth3=20          # Set zone 3 sensitivity
save             # Save settings to sensor
get_all          # Query current settings
test_mode=1      # Enable test mode
```

## Configuration Storage
Settings are saved to ESP8266 EEPROM and persist across power cycles.

## Detection Algorithm

1. **Data Collection**: Serial data parsed from sensor
2. **State Detection**:
   - "mov" prefix → Moving state
   - "occ" prefix → Stationary state
3. **Distance Extraction**: Parsed from `dis=X.XX` format
4. **Timeout Handling**: After configured timeout with no detection, state resets to "none"
5. **Statistics Update**: Counters incremented on each detection

## Zone-Based Sensitivity

The sensor divides detection range into 3 zones with independent sensitivity thresholds:

- **Zone 1** (0-2.8m): Higher sensitivity needed due to stronger signal reflection
- **Zone 2** (2.8-8m): Medium sensitivity
- **Zone 3** (>8m): Lower sensitivity for distant targets

Higher mth values = less sensitive (reduces false positives but may miss detections)

## Installation Recommendations

From the datasheet:
- Can penetrate plastic, glass, acrylic (not metal)
- Keep 5-6mm distance from housing
- Mount height: 3m for optimal coverage (radius >2m)
- Avoid near air conditioners or fans
- Face antenna toward detection area

## Use Cases
- Smart lighting automation
- Occupancy-based HVAC control
- Security presence detection
- Elderly care monitoring
- Meeting room occupancy tracking

## Technical Specifications

| Parameter | Value |
|-----------|-------|
| Frequency | 23.5-24.5 GHz |
| Modulation | FMCW |
| Power Supply | 3.3-5V DC |
| Serial Level | 3.3V TTL |
| Detection Cycle | Adaptive |
| Data Format | ASCII text |

## Code Structure

- `LD1125H_WebServer.ino:43-65` - Setup and initialization
- `LD1125H_WebServer.ino:67-71` - Main loop
- `LD1125H_WebServer.ino:83-270` - Web interface HTML generation
- `LD1125H_WebServer.ino:340-352` - Serial data reading
- `LD1125H_WebServer.ino:354-388` - Data parsing and processing
- `LD1125H_WebServer.ino:390-398` - Timeout management
- `LD1125H_WebServer.ino:400-411` - Radar configuration
- `LD1125H_WebServer.ino:413-429` - EEPROM persistence

## Future Enhancements
- MQTT integration for smart home platforms
- Multi-sensor support
- Historical data logging
- Email/push notifications
- RESTful API expansion
