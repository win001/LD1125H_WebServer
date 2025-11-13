# Debug Logging Guide

## Overview
The code now includes comprehensive debug logging to help troubleshoot and monitor system operation. All logs are output to Serial at 115200 baud.

## Log Categories

### [INIT] - System Initialization
Logs during startup sequence:
```
[INIT] Starting radar serial communication...
[INIT] Radar serial initialized at 115200 baud
[INIT] Initializing EEPROM...
```

### [WiFi] - WiFi Access Point
WiFi setup and configuration:
```
[WiFi] Setting up Access Point...
[WiFi] Access Point created successfully
[WiFi] SSID: LD1125H_Sensor
[WiFi] Password: 12345678
[WiFi] IP address: 192.168.4.1
[WiFi] Connect to WiFi and open http://192.168.4.1
```

### [WEB] - Web Server Requests
HTTP endpoint access logs:
```
[WEB] Setting up web server routes...
[WEB] Web server started on port 80
[WEB] GET / - Serving main page
[WEB] GET /status - State: moving, Distance: 2.5m, Signal: 0
[WEB] GET /config - Sending current configuration
[WEB] POST /config - Updating configuration
[WEB] GET /stats - Total: 42, Moving: 30, Stationary: 12, Max Distance: 5.2m
[WEB] POST /testmode - Test mode ENABLED
[WEB] POST /reset - Resetting to default configuration
```

### [CONFIG] - Configuration Changes
Configuration updates and current values:
```
[CONFIG] rmax=8.0, mth1=60, mth2=30, mth3=20, timeout=30s, testMode=OFF
[CONFIG] Received JSON: {"rmax":6.5,"mth1":70,"mth2":35,"mth3":25,"timeout":45}
[CONFIG] New configuration:
  rmax=6.5m, mth1=70, mth2=35, mth3=25, timeout=45s
[CONFIG] Defaults: rmax=8.0m, mth1=60, mth2=30, mth3=20, timeout=30s
[CONFIG] ERROR: No JSON payload received
```

### [RADAR] - Radar Communication
Commands sent to radar and raw data received:
```
[RADAR] Waiting 1 second before configuring...
[RADAR] Configuring radar sensor...
[RADAR] Sending: rmax=8.0
[RADAR] Sending: mth1=60
[RADAR] Sending: mth2=30
[RADAR] Sending: mth3=20
[RADAR] Sending: save
[RADAR] Configuration complete
[RADAR] Sending command: test_mode=1
[RADAR] Raw data: mov, dis=2.50
[RADAR] Raw data: occ, dis=1.80
[RADAR] Info: received message: rmax=8
```

### [DETECT] - Presence Detection
Movement and stationary presence detection events:
```
[DETECT] MOVING at 2.5m
[DETECT] MOVING at 3.2m (signal: 125)
[DETECT] STATIONARY at 1.8m
[DETECT] STATIONARY at 2.1m (signal: 85)
```
*Note: Signal strength only shown when test mode is enabled*

### [TIMEOUT] - No Presence Timeout
When presence clears after timeout period:
```
[TIMEOUT] No detection for 30 seconds - clearing presence state
```

### [EEPROM] - Configuration Persistence
EEPROM read/write operations:
```
[EEPROM] Loading configuration from EEPROM...
[EEPROM] Invalid configuration detected, loading defaults
[EEPROM] Valid configuration loaded:
  rmax=8.0m, mth1=60, mth2=30, mth3=20, timeout=30s, testMode=OFF
[EEPROM] Saving configuration to EEPROM...
[EEPROM] Configuration saved successfully
```

### [READY] - System Ready
Final startup message:
```
========================================
[READY] System initialization complete!
========================================
```

## Startup Sequence Example

```
========================================
LD1125H Occupancy Sensor Web Server
========================================
[INIT] Starting radar serial communication...
[INIT] Radar serial initialized at 115200 baud
[INIT] Initializing EEPROM...
[EEPROM] Loading configuration from EEPROM...
[EEPROM] Valid configuration loaded:
  rmax=8.0m, mth1=60, mth2=30, mth3=20, timeout=30s, testMode=OFF
[WiFi] Setting up Access Point...
[WiFi] Access Point created successfully
[WiFi] SSID: LD1125H_Sensor
[WiFi] Password: 12345678
[WiFi] IP address: 192.168.4.1
[WiFi] Connect to WiFi and open http://192.168.4.1
[WEB] Setting up web server routes...
[WEB] Web server started on port 80
[RADAR] Waiting 1 second before configuring...
[RADAR] Configuring radar sensor...
[RADAR] Sending: rmax=8.0
[RADAR] Sending: mth1=60
[RADAR] Sending: mth2=30
[RADAR] Sending: mth3=20
[RADAR] Sending: save
[RADAR] Configuration complete
========================================
[READY] System initialization complete!
========================================
```

## Operation Example

```
[RADAR] Raw data: mov, dis=2.50
[DETECT] MOVING at 2.5m
[WEB] GET /status - State: moving, Distance: 2.5m, Signal: 0
[RADAR] Raw data: occ, dis=2.40
[DETECT] STATIONARY at 2.4m
[WEB] GET /stats - Total: 2, Moving: 1, Stationary: 1, Max Distance: 2.5m
[TIMEOUT] No detection for 30 seconds - clearing presence state
[WEB] GET /status - State: none, Distance: 0m, Signal: 0
```

## Configuration Change Example

```
[WEB] POST /config - Updating configuration
[CONFIG] Received JSON: {"rmax":6,"mth1":70,"mth2":35,"mth3":25,"timeout":45}
[CONFIG] New configuration:
  rmax=6.0m, mth1=70, mth2=35, mth3=25, timeout=45s
[EEPROM] Saving configuration to EEPROM...
[EEPROM] Configuration saved successfully
[RADAR] Configuring radar sensor...
[RADAR] Sending: rmax=6.0
[RADAR] Sending: mth1=70
[RADAR] Sending: mth2=35
[RADAR] Sending: mth3=25
[RADAR] Sending: save
[RADAR] Configuration complete
```

## Troubleshooting Tips

### No Radar Data Received
If you don't see `[RADAR] Raw data:` messages:
- Check SoftwareSerial pin connections (RX=GPIO12, TX=GPIO14)
- Verify radar power supply (5V)
- Ensure radar baud rate is 115200

### Configuration Not Saving
Look for:
```
[EEPROM] Configuration saved successfully
```
If missing, check EEPROM initialization

### Web Server Not Accessible
Check for:
```
[WiFi] IP address: 192.168.4.1
[WEB] Web server started on port 80
```
Ensure you're connected to the WiFi AP

### False Detections
Enable test mode to see signal strength:
```
[WEB] POST /testmode - Test mode ENABLED
[DETECT] STATIONARY at 1.2m (signal: 95)
```
Adjust sensitivity thresholds (mth1/mth2/mth3) if needed

## Reducing Log Verbosity

If logs are too verbose, you can comment out specific log categories:

1. **Reduce status polling logs**: Comment out lines in `handleStatus()` (LD1125H_WebServer.ino:301-306)
2. **Reduce radar raw data**: Comment out line in `processData()` (LD1125H_WebServer.ino:435-436)
3. **Reduce detection logs**: Comment out lines in `processData()` (LD1125H_WebServer.ino:466-486)

## Serial Monitor Settings

- **Baud Rate**: 115200
- **Line Ending**: Both NL & CR (for sending commands)
- **Buffer**: At least 256 bytes recommended
