# State Change Reporting System

## Overview
The system now includes immediate state change detection and reporting, plus periodic updates every 3 seconds through the web interface.

## Features

### 1. Immediate State Change Detection
The system detects and reports state changes in real-time when:
- Person starts moving: `none` → `moving`
- Person becomes stationary: `moving` → `stationary`
- Person leaves area: `moving`/`stationary` → `none`

### 2. Periodic Updates
Web interface polls for status every **3 seconds** (changed from 1 second to reduce network traffic).

## State Change Event Log

When a state change occurs, you'll see this in the Serial Monitor:

```
========================================
[STATE CHANGE] IMMEDIATE UPDATE
  Previous: none -> New: moving
  Distance: 2.5m
  Timestamp: 12345678
========================================
```

### Possible State Transitions

| From | To | Trigger |
|------|-----|---------|
| none | moving | Large movement detected |
| none | stationary | Breathing/micro-movement detected |
| moving | stationary | Person stopped moving |
| stationary | moving | Person started moving again |
| moving | none | No detection for timeout period |
| stationary | none | No detection for timeout period |

## API Updates

### Enhanced Status Endpoint

**GET `/status`** now returns additional fields:

```json
{
  "state": "moving",
  "previousState": "stationary",
  "distance": 2.5,
  "signal": 125,
  "lastStateChange": 12345678
}
```

**New Fields:**
- `previousState`: The state before the current one
- `lastStateChange`: Millisecond timestamp of the last state change

## Integrating with External Servers

### Option 1: Using the reportStateChange() Function

The `reportStateChange()` function in **LD1125H_WebServer.ino:46-70** is called immediately when state changes occur. Add your server reporting code here:

```cpp
void reportStateChange(String newState, String oldState, float distance) {
  sensorData.lastStateChange = millis();

  Serial.println("========================================");
  Serial.println("[STATE CHANGE] IMMEDIATE UPDATE");
  Serial.print("  Previous: ");
  Serial.print(oldState);
  Serial.print(" -> New: ");
  Serial.println(newState);
  Serial.print("  Distance: ");
  Serial.print(distance);
  Serial.println("m");
  Serial.print("  Timestamp: ");
  Serial.println(millis());
  Serial.println("========================================");

  // TODO: Add your server reporting code here
  // Example: Send HTTP POST to external server
  // HTTPClient http;
  // http.begin("http://your-server.com/api/state");
  // http.addHeader("Content-Type", "application/json");
  // String payload = "{\"state\":\"" + newState + "\",\"distance\":" + String(distance) + "}";
  // http.POST(payload);
  // http.end();
}
```

### Option 2: HTTP POST Example

To send state changes to an external server via HTTP POST:

1. **Add HTTPClient library** at the top of the file:
```cpp
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
```

2. **Update reportStateChange()** function:
```cpp
void reportStateChange(String newState, String oldState, float distance) {
  sensorData.lastStateChange = millis();

  Serial.println("[STATE CHANGE] Reporting to server...");

  WiFiClient client;
  HTTPClient http;

  // Replace with your server URL
  http.begin(client, "http://192.168.1.100:3000/api/occupancy");
  http.addHeader("Content-Type", "application/json");

  // Build JSON payload
  String payload = "{";
  payload += "\"deviceId\":\"LD1125H_001\",";
  payload += "\"state\":\"" + newState + "\",";
  payload += "\"previousState\":\"" + oldState + "\",";
  payload += "\"distance\":" + String(distance) + ",";
  payload += "\"timestamp\":" + String(millis());
  payload += "}";

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.print("[HTTP] POST Response code: ");
    Serial.println(httpCode);
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("[HTTP] State change reported successfully");
    }
  } else {
    Serial.print("[HTTP] POST failed, error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}
```

### Option 3: MQTT Integration

For MQTT-based IoT platforms (Home Assistant, Node-RED, etc.):

1. **Add PubSubClient library**:
```cpp
#include <PubSubClient.h>
```

2. **Setup MQTT client** (add to global variables):
```cpp
WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;
const char* mqtt_topic = "home/occupancy/sensor1";
```

3. **Connect to MQTT** (add to setup()):
```cpp
mqttClient.setServer(mqtt_server, mqtt_port);
```

4. **Update reportStateChange()**:
```cpp
void reportStateChange(String newState, String oldState, float distance) {
  sensorData.lastStateChange = millis();

  Serial.println("[STATE CHANGE] Publishing to MQTT...");

  // Reconnect if needed
  if (!mqttClient.connected()) {
    if (mqttClient.connect("LD1125H_Sensor")) {
      Serial.println("[MQTT] Connected");
    } else {
      Serial.println("[MQTT] Connection failed");
      return;
    }
  }

  // Build JSON payload
  String payload = "{";
  payload += "\"state\":\"" + newState + "\",";
  payload += "\"previousState\":\"" + oldState + "\",";
  payload += "\"distance\":" + String(distance);
  payload += "}";

  if (mqttClient.publish(mqtt_topic, payload.c_str())) {
    Serial.println("[MQTT] State change published successfully");
  } else {
    Serial.println("[MQTT] Publish failed");
  }
}
```

5. **Add to loop()**:
```cpp
void loop() {
  server.handleClient();
  mqttClient.loop();  // Add this line
  readRadarData();
  checkTimeout();
}
```

### Option 4: Webhook to Cloud Services

Send to services like IFTTT, Zapier, or custom webhooks:

```cpp
void reportStateChange(String newState, String oldState, float distance) {
  sensorData.lastStateChange = millis();

  WiFiClient client;
  HTTPClient http;

  // IFTTT Webhook example
  String url = "https://maker.ifttt.com/trigger/occupancy_changed/with/key/YOUR_KEY";
  url += "?value1=" + newState;
  url += "&value2=" + oldState;
  url += "&value3=" + String(distance);

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("[WEBHOOK] Triggered successfully");
  }

  http.end();
}
```

## Web Interface Polling

The web interface now updates every **3 seconds** instead of 1 second:

```javascript
setInterval(updateStatus, 3000);  // Status updates
setInterval(updateStats, 3000);   // Statistics updates
```

### Detecting State Changes in Web Interface

You can detect state changes in the web interface by comparing `lastStateChange` timestamps:

```javascript
let lastStateChangeTime = 0;

function updateStatus() {
  fetch('/status').then(r => r.json()).then(data => {
    // Check if state changed since last poll
    if (data.lastStateChange > lastStateChangeTime) {
      console.log('State changed!', data.previousState, '->', data.state);
      lastStateChangeTime = data.lastStateChange;

      // Trigger immediate action (e.g., notification, update UI)
      handleStateChange(data);
    }

    // Update UI
    document.getElementById('status').innerText =
      data.state == 'moving' ? 'Movement Detected' :
      data.state == 'stationary' ? 'Presence Detected' :
      'No Presence';
    document.getElementById('distance').innerText =
      data.state != 'none' ? data.distance.toFixed(2) + 'm' : '--';
  });
}

function handleStateChange(data) {
  // Play sound, show notification, etc.
  console.log('Immediate state change detected!');
  // Could show browser notification:
  // new Notification('Occupancy Changed', {body: 'New state: ' + data.state});
}
```

## Testing State Change Detection

1. **Monitor Serial Output** (115200 baud):
   - Watch for `[STATE CHANGE] IMMEDIATE UPDATE` messages
   - Verify state transitions are logged correctly

2. **Walk in front of sensor**:
   - Should see: `none` → `moving`

3. **Sit still in front of sensor**:
   - Should see: `moving` → `stationary`

4. **Leave the detection area**:
   - After timeout period, should see: `stationary`/`moving` → `none`

5. **Check API response**:
```bash
curl http://192.168.4.1/status
```

Expected response:
```json
{
  "state": "moving",
  "previousState": "none",
  "distance": 2.5,
  "signal": 0,
  "lastStateChange": 12345678
}
```

## Performance Considerations

- **Immediate reporting**: State changes are detected in real-time within the `loop()` cycle (~milliseconds)
- **Periodic polling**: Web interface updates every 3 seconds
- **Network traffic**: Reduced by 66% compared to 1-second polling
- **HTTP POST overhead**: Add delays if sending to external servers to avoid blocking

## Troubleshooting

### State changes not detected
- Check Serial Monitor for `[DETECT]` messages
- Verify radar is sending data: look for `[RADAR] Raw data:` logs
- Ensure timeout value isn't too short

### External server not receiving updates
- Check network connectivity from ESP8266
- Verify server URL and port are correct
- Look for `[HTTP]` error messages in Serial Monitor
- Test server endpoint manually with curl/Postman

### Too many state changes
- Increase sensitivity thresholds (mth1, mth2, mth3)
- Enable test mode to see signal strength values
- Adjust timeout value to avoid flickering

## Example Use Cases

1. **Smart Lighting**: Turn lights on/off based on state changes
2. **HVAC Control**: Adjust temperature when room becomes occupied/vacant
3. **Security Alerts**: Send notification when movement detected in restricted area
4. **Occupancy Analytics**: Log state changes to database for usage analysis
5. **Home Automation**: Trigger scenes in Home Assistant/OpenHAB when state changes
