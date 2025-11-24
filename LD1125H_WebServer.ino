#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// AP Configuration
const char* ssid = "LD1125H_Sensor";
const char* password = "12345678";

// Web Server
WebServer server(80);

// Serial for LD1125H (using Hardware Serial2 on ESP32)
// ESP32 Serial2 default pins: RX=GPIO16, TX=GPIO17
// You can change these pins if needed
#define RADAR_RX 16
#define RADAR_TX 17
HardwareSerial radarSerial(2); // Use UART2

// Output pin for load control (relay, LED, etc.)
// HIGH = Occupied (presence detected)
// LOW = No presence
#define OCCUPANCY_OUTPUT_PIN 2  // GPIO2 (D2)

// Sensor Data
struct {
  String state = "none";
  String previousState = "none";
  float distance = 0.0;
  unsigned long lastDetection = 0;
  unsigned long lastStateChange = 0;
  int signalStrength = 0;
} sensorData;

// Configuration
struct {
  float rmax = 8.0;        // Max detection distance (meters)
  int mth1_mov = 60;       // Zone 1 (0-2.8m) movement sensitivity
  int mth2_mov = 30;       // Zone 2 (2.8-8m) movement sensitivity
  int mth3_mov = 20;       // Zone 3 (>8m) movement sensitivity
  int mth1_occ = 60;       // Zone 1 (0-2.8m) occupancy sensitivity
  int mth2_occ = 30;       // Zone 2 (2.8-8m) occupancy sensitivity
  int mth3_occ = 20;       // Zone 3 (>8m) occupancy sensitivity
  int timeout = 30;        // No presence timeout (seconds)
  bool testMode = false;   // Test mode flag
} config;

// Statistics
unsigned long totalDetections = 0;
unsigned long movingCount = 0;
unsigned long stationaryCount = 0;
float maxDistance = 0;

String serialBuffer = "";

// Function to report state changes immediately
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

  // Control output pin based on occupancy state
  if (newState == "none") {
    digitalWrite(OCCUPANCY_OUTPUT_PIN, LOW);
    Serial.println("[OUTPUT] GPIO2 -> LOW (No Presence - Load OFF)");
  } else {
    // newState is "moving" or "stationary" - both mean occupied
    digitalWrite(OCCUPANCY_OUTPUT_PIN, HIGH);
    Serial.println("[OUTPUT] GPIO2 -> HIGH (Occupied - Load ON)");
  }

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

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n========================================");
  Serial.println("LD1125H Occupancy Sensor Web Server - ESP32");
  Serial.println("========================================");

  Serial.println("[INIT] Starting radar serial communication...");
  radarSerial.begin(115200, SERIAL_8N1, RADAR_RX, RADAR_TX);
  Serial.print("[INIT] Radar serial initialized at 115200 baud (RX=GPIO");
  Serial.print(RADAR_RX);
  Serial.print(", TX=GPIO");
  Serial.print(RADAR_TX);
  Serial.println(")");

  // Initialize occupancy output pin
  Serial.print("[INIT] Setting up occupancy output on GPIO");
  Serial.println(OCCUPANCY_OUTPUT_PIN);
  pinMode(OCCUPANCY_OUTPUT_PIN, OUTPUT);
  digitalWrite(OCCUPANCY_OUTPUT_PIN, LOW);  // Start with LOW (no presence)
  Serial.println("[INIT] Occupancy output initialized (LOW = No Presence)");

  Serial.println("[INIT] Initializing EEPROM...");
  EEPROM.begin(512);
  loadConfig();

  // Setup WiFi AP
  Serial.println("[WiFi] Setting up Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.println("[WiFi] Access Point created successfully");
  Serial.print("[WiFi] SSID: ");
  Serial.println(ssid);
  Serial.print("[WiFi] Password: ");
  Serial.println(password);
  Serial.print("[WiFi] IP address: ");
  Serial.println(IP);
  Serial.println("[WiFi] Connect to WiFi and open http://192.168.4.1");

  // Setup web server
  Serial.println("[WEB] Setting up web server routes...");
  setupWebServer();
  server.begin();
  Serial.println("[WEB] Web server started on port 80");

  // Configure radar
  Serial.println("[RADAR] Waiting 1 second before configuring...");
  delay(1000);
  configureRadar();

  Serial.println("========================================");
  Serial.println("[READY] System initialization complete!");
  Serial.println("========================================\n");
}

void loop() {
  server.handleClient();
  readRadarData();
  checkTimeout();
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/config", HTTP_GET, handleGetConfig);
  server.on("/config", HTTP_POST, handleSetConfig);
  server.on("/stats", HTTP_GET, handleStats);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/testmode", HTTP_POST, handleTestMode);
}

void handleRoot() {
  Serial.println("[WEB] GET / - Serving main page");
  String html = "";
  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>LD1125H Sensor</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#e0e0e0;}";
  html += ".container{max-width:800px;margin:0 auto;}";
  html += ".card{background:white;border-radius:8px;padding:20px;margin-bottom:20px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;margin:0 0 20px 0;}";
  html += ".status{text-align:center;padding:30px;background:#f5f5f5;border-radius:8px;margin:20px 0;}";
  html += ".status-text{font-size:24px;font-weight:bold;color:#333;}";
  html += ".distance{font-size:36px;color:#4CAF50;margin:10px 0;}";
  html += ".form-group{margin-bottom:20px;}";
  html += "label{display:block;margin-bottom:5px;color:#666;font-weight:bold;}";
  html += "input[type='range'],input[type='number']{width:100%;padding:8px;border:1px solid #ddd;border-radius:4px;}";
  html += "input[type='range']{padding:0;}";
  html += ".range-value{text-align:right;color:#666;font-size:14px;}";
  html += "button{background:#4CAF50;color:white;border:none;padding:10px 20px;border-radius:4px;cursor:pointer;margin:5px;}";
  html += "button:hover{background:#45a049;}";
  html += ".btn-danger{background:#f44336;}";
  html += ".btn-danger:hover{background:#da190b;}";
  html += ".btn-info{background:#2196F3;}";
  html += ".btn-info:hover{background:#0b79d0;}";
  html += ".stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;}";
  html += ".stat-item{text-align:center;padding:15px;background:#f9f9f9;border-radius:4px;}";
  html += ".stat-value{font-size:24px;font-weight:bold;color:#2196F3;}";
  html += ".stat-label{font-size:12px;color:#666;margin-top:5px;}";
  html += ".zone{background:#f9f9f9;padding:15px;border-radius:4px;margin-bottom:10px;}";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  
  // Status Card
  html += "<div class='card'>";
  html += "<h1>LD1125H Radar Sensor</h1>";
  html += "<div class='status'>";
  html += "<div class='status-text' id='status'>Loading...</div>";
  html += "<div class='distance' id='distance'>--</div>";
  html += "<div id='signal'></div>";
  html += "</div>";
  html += "</div>";
  
  // Configuration Card
  html += "<div class='card'>";
  html += "<h2>Configuration</h2>";
  
  // Max Distance
  html += "<div class='form-group'>";
  html += "<label>Max Detection Distance: <span id='rmax_val'>8.0</span>m</label>";
  html += "<input type='range' id='rmax' min='0.4' max='12' step='0.1' value='8'>";
  html += "</div>";
  
  // Movement Sensitivity Section
  html += "<h3>Movement Sensitivity (mov)</h3>";

  html += "<div class='zone'>";
  html += "<label>Zone 1 (0-2.8m): <span id='mth1_mov_val'>60</span></label>";
  html += "<input type='range' id='mth1_mov' min='10' max='600' step='10' value='60'>";
  html += "</div>";

  html += "<div class='zone'>";
  html += "<label>Zone 2 (2.8-8m): <span id='mth2_mov_val'>30</span></label>";
  html += "<input type='range' id='mth2_mov' min='5' max='300' step='5' value='30'>";
  html += "</div>";

  html += "<div class='zone'>";
  html += "<label>Zone 3 (>8m): <span id='mth3_mov_val'>20</span></label>";
  html += "<input type='range' id='mth3_mov' min='5' max='200' step='5' value='20'>";
  html += "</div>";

  // Occupancy Sensitivity Section
  html += "<h3>Occupancy Sensitivity (occ)</h3>";
  html += "<small>Lower = More Sensitive</small>";

  html += "<div class='zone'>";
  html += "<label>Zone 1 (0-2.8m): <span id='mth1_occ_val'>60</span></label>";
  html += "<input type='range' id='mth1_occ' min='10' max='600' step='10' value='60'>";
  html += "</div>";

  html += "<div class='zone'>";
  html += "<label>Zone 2 (2.8-8m): <span id='mth2_occ_val'>30</span></label>";
  html += "<input type='range' id='mth2_occ' min='5' max='300' step='5' value='30'>";
  html += "<small>Lower = More Sensitive</small>";
  html += "</div>";

  html += "<div class='zone'>";
  html += "<label>Zone 3 (>8m): <span id='mth3_occ_val'>20</span></label>";
  html += "<input type='range' id='mth3_occ' min='5' max='200' step='5' value='20'>";
  html += "</div>";

  // Timeout
  html += "<div class='form-group'>";
  html += "<label>No Presence Timeout (seconds)</label>";
  html += "<input type='number' id='timeout' min='5' max='300' value='30'>";
  html += "</div>";
  
  // Buttons
  html += "<button onclick='saveConfig()'>Save Settings</button>";
  html += "<button class='btn-info' onclick='toggleTest()'>Toggle Test Mode</button>";
  html += "<button class='btn-danger' onclick='resetConfig()'>Reset Defaults</button>";
  html += "</div>";
  
  // Statistics Card
  html += "<div class='card'>";
  html += "<h2>Statistics</h2>";
  html += "<div class='stats' id='stats'>";
  html += "<div class='stat-item'><div class='stat-value' id='total'>0</div><div class='stat-label'>Total Detections</div></div>";
  html += "<div class='stat-item'><div class='stat-value' id='moving'>0</div><div class='stat-label'>Moving</div></div>";
  html += "<div class='stat-item'><div class='stat-value' id='stationary'>0</div><div class='stat-label'>Stationary</div></div>";
  html += "<div class='stat-item'><div class='stat-value' id='max_dist'>0m</div><div class='stat-label'>Max Distance</div></div>";
  html += "</div>";
  html += "</div>";
  
  html += "</div>";
  
  // JavaScript
  html += "<script>";
  html += "function updateStatus(){";
  html += "  fetch('/status').then(r=>r.json()).then(d=>{";
  html += "    document.getElementById('status').innerText=d.state=='moving'?'Movement Detected':d.state=='stationary'?'Presence Detected':'No Presence';";
  html += "    document.getElementById('distance').innerText=d.state!='none'?d.distance.toFixed(2)+'m':'--';";
  html += "    document.getElementById('signal').innerText=d.signal>0?'Signal: '+d.signal:'';";
  html += "  });";
  html += "}";
  
  html += "function updateStats(){";
  html += "  fetch('/stats').then(r=>r.json()).then(d=>{";
  html += "    document.getElementById('total').innerText=d.total;";
  html += "    document.getElementById('moving').innerText=d.moving;";
  html += "    document.getElementById('stationary').innerText=d.stationary;";
  html += "    document.getElementById('max_dist').innerText=d.maxDist.toFixed(1)+'m';";
  html += "  });";
  html += "}";
  
  html += "function loadConfig(){";
  html += "  fetch('/config').then(r=>r.json()).then(d=>{";
  html += "    document.getElementById('rmax').value=d.rmax;";
  html += "    document.getElementById('rmax_val').innerText=d.rmax;";
  html += "    document.getElementById('mth1_mov').value=d.mth1_mov;";
  html += "    document.getElementById('mth1_mov_val').innerText=d.mth1_mov;";
  html += "    document.getElementById('mth2_mov').value=d.mth2_mov;";
  html += "    document.getElementById('mth2_mov_val').innerText=d.mth2_mov;";
  html += "    document.getElementById('mth3_mov').value=d.mth3_mov;";
  html += "    document.getElementById('mth3_mov_val').innerText=d.mth3_mov;";
  html += "    document.getElementById('mth1_occ').value=d.mth1_occ;";
  html += "    document.getElementById('mth1_occ_val').innerText=d.mth1_occ;";
  html += "    document.getElementById('mth2_occ').value=d.mth2_occ;";
  html += "    document.getElementById('mth2_occ_val').innerText=d.mth2_occ;";
  html += "    document.getElementById('mth3_occ').value=d.mth3_occ;";
  html += "    document.getElementById('mth3_occ_val').innerText=d.mth3_occ;";
  html += "    document.getElementById('timeout').value=d.timeout;";
  html += "  });";
  html += "}";
  
  html += "function saveConfig(){";
  html += "  var data={";
  html += "    rmax:parseFloat(document.getElementById('rmax').value),";
  html += "    mth1_mov:parseInt(document.getElementById('mth1_mov').value),";
  html += "    mth2_mov:parseInt(document.getElementById('mth2_mov').value),";
  html += "    mth3_mov:parseInt(document.getElementById('mth3_mov').value),";
  html += "    mth1_occ:parseInt(document.getElementById('mth1_occ').value),";
  html += "    mth2_occ:parseInt(document.getElementById('mth2_occ').value),";
  html += "    mth3_occ:parseInt(document.getElementById('mth3_occ').value),";
  html += "    timeout:parseInt(document.getElementById('timeout').value)";
  html += "  };";
  html += "  fetch('/config',{method:'POST',body:JSON.stringify(data)}).then(()=>{";
  html += "    alert('Settings saved!');";
  html += "    loadConfig();";
  html += "  });";
  html += "}";
  
  html += "function toggleTest(){";
  html += "  fetch('/testmode',{method:'POST'}).then(r=>r.json()).then(d=>{";
  html += "    alert('Test mode '+(d.enabled?'enabled':'disabled'));";
  html += "  });";
  html += "}";
  
  html += "function resetConfig(){";
  html += "  if(confirm('Reset to defaults?')){";
  html += "    fetch('/reset',{method:'POST'}).then(()=>{";
  html += "      alert('Reset complete');";
  html += "      loadConfig();";
  html += "    });";
  html += "  }";
  html += "}";
  
  // Update sliders
  html += "document.getElementById('rmax').oninput=function(){";
  html += "  document.getElementById('rmax_val').innerText=this.value;";
  html += "};";
  html += "document.getElementById('mth1_mov').oninput=function(){";
  html += "  document.getElementById('mth1_mov_val').innerText=this.value;";
  html += "};";
  html += "document.getElementById('mth2_mov').oninput=function(){";
  html += "  document.getElementById('mth2_mov_val').innerText=this.value;";
  html += "};";
  html += "document.getElementById('mth3_mov').oninput=function(){";
  html += "  document.getElementById('mth3_mov_val').innerText=this.value;";
  html += "};";
  html += "document.getElementById('mth1_occ').oninput=function(){";
  html += "  document.getElementById('mth1_occ_val').innerText=this.value;";
  html += "};";
  html += "document.getElementById('mth2_occ').oninput=function(){";
  html += "  document.getElementById('mth2_occ_val').innerText=this.value;";
  html += "};";
  html += "document.getElementById('mth3_occ').oninput=function(){";
  html += "  document.getElementById('mth3_occ_val').innerText=this.value;";
  html += "};";
  
  // Auto refresh - Update every 3 seconds
  html += "loadConfig();";
  html += "updateStatus();";
  html += "updateStats();";
  html += "setInterval(updateStatus,3000);";  // Changed from 1000ms to 3000ms
  html += "setInterval(updateStats,3000);";
  
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  String json = "{";
  json += "\"state\":\"" + sensorData.state + "\",";
  json += "\"previousState\":\"" + sensorData.previousState + "\",";
  json += "\"distance\":" + String(sensorData.distance) + ",";
  json += "\"signal\":" + String(sensorData.signalStrength) + ",";
  json += "\"lastStateChange\":" + String(sensorData.lastStateChange);
  json += "}";
  Serial.print("[WEB] GET /status - State: ");
  Serial.print(sensorData.state);
  Serial.print(", Distance: ");
  Serial.print(sensorData.distance);
  Serial.print("m, Signal: ");
  Serial.println(sensorData.signalStrength);
  server.send(200, "application/json", json);
}

void handleGetConfig() {
  Serial.println("[WEB] GET /config - Sending current configuration");
  String json = "{";
  json += "\"rmax\":" + String(config.rmax) + ",";
  json += "\"mth1_mov\":" + String(config.mth1_mov) + ",";
  json += "\"mth2_mov\":" + String(config.mth2_mov) + ",";
  json += "\"mth3_mov\":" + String(config.mth3_mov) + ",";
  json += "\"mth1_occ\":" + String(config.mth1_occ) + ",";
  json += "\"mth2_occ\":" + String(config.mth2_occ) + ",";
  json += "\"mth3_occ\":" + String(config.mth3_occ) + ",";
  json += "\"timeout\":" + String(config.timeout) + ",";
  json += "\"testMode\":" + String(config.testMode ? "true" : "false");
  json += "}";
  Serial.print("[CONFIG] rmax=");
  Serial.print(config.rmax);
  Serial.print("m, mth_mov=");
  Serial.print(config.mth1_mov);
  Serial.print("/");
  Serial.print(config.mth2_mov);
  Serial.print("/");
  Serial.print(config.mth3_mov);
  Serial.print(", mth_occ=");
  Serial.print(config.mth1_occ);
  Serial.print("/");
  Serial.print(config.mth2_occ);
  Serial.print("/");
  Serial.print(config.mth3_occ);
  Serial.print(", timeout=");
  Serial.print(config.timeout);
  Serial.print("s, testMode=");
  Serial.println(config.testMode ? "ON" : "OFF");
  server.send(200, "application/json", json);
}

void handleSetConfig() {
  Serial.println("[WEB] POST /config - Updating configuration");
  if (server.hasArg("plain")) {
    Serial.print("[CONFIG] Received JSON: ");
    Serial.println(server.arg("plain"));

    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));

    config.rmax = doc["rmax"] | config.rmax;
    config.mth1_mov = doc["mth1_mov"] | config.mth1_mov;
    config.mth2_mov = doc["mth2_mov"] | config.mth2_mov;
    config.mth3_mov = doc["mth3_mov"] | config.mth3_mov;
    config.mth1_occ = doc["mth1_occ"] | config.mth1_occ;
    config.mth2_occ = doc["mth2_occ"] | config.mth2_occ;
    config.mth3_occ = doc["mth3_occ"] | config.mth3_occ;
    config.timeout = doc["timeout"] | config.timeout;

    Serial.println("[CONFIG] New configuration:");
    Serial.print("  rmax=");
    Serial.print(config.rmax);
    Serial.print("m, mth_mov=");
    Serial.print(config.mth1_mov);
    Serial.print("/");
    Serial.print(config.mth2_mov);
    Serial.print("/");
    Serial.print(config.mth3_mov);
    Serial.print(", mth_occ=");
    Serial.print(config.mth1_occ);
    Serial.print("/");
    Serial.print(config.mth2_occ);
    Serial.print("/");
    Serial.print(config.mth3_occ);
    Serial.print(", timeout=");
    Serial.print(config.timeout);
    Serial.println("s");

    saveConfig();
    configureRadar();
  } else {
    Serial.println("[CONFIG] ERROR: No JSON payload received");
  }
  server.send(200, "text/plain", "OK");
}

void handleStats() {
  Serial.print("[WEB] GET /stats - Total: ");
  Serial.print(totalDetections);
  Serial.print(", Moving: ");
  Serial.print(movingCount);
  Serial.print(", Stationary: ");
  Serial.print(stationaryCount);
  Serial.print(", Max Distance: ");
  Serial.print(maxDistance);
  Serial.println("m");

  String json = "{";
  json += "\"total\":" + String(totalDetections) + ",";
  json += "\"moving\":" + String(movingCount) + ",";
  json += "\"stationary\":" + String(stationaryCount) + ",";
  json += "\"maxDist\":" + String(maxDistance);
  json += "}";
  server.send(200, "application/json", json);
}

void handleTestMode() {
  config.testMode = !config.testMode;
  Serial.print("[WEB] POST /testmode - Test mode ");
  Serial.println(config.testMode ? "ENABLED" : "DISABLED");

  String cmd = "test_mode=" + String(config.testMode ? 1 : 0) + "\r\n";
  Serial.print("[RADAR] Sending command: ");
  Serial.print(cmd);
  radarSerial.print(cmd);
  delay(100);

  String json = "{\"enabled\":" + String(config.testMode ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleReset() {
  Serial.println("[WEB] POST /reset - Resetting to default configuration");
  config.rmax = 8.0;
  config.mth1_mov = 60;
  config.mth2_mov = 30;
  config.mth3_mov = 20;
  config.mth1_occ = 60;
  config.mth2_occ = 30;
  config.mth3_occ = 20;
  config.timeout = 30;
  config.testMode = false;
  Serial.println("[CONFIG] Defaults: rmax=8.0m, mth_mov=60/30/20, mth_occ=60/30/20, timeout=30s");
  saveConfig();
  configureRadar();
  server.send(200, "text/plain", "OK");
}

void readRadarData() {
  while (radarSerial.available()) {
    char c = radarSerial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        processData(serialBuffer);
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
    }
  }
}

void processData(String data) {
  Serial.print("[RADAR] Raw data: ");
  Serial.println(data);

  if (data.startsWith("mov") || data.startsWith("occ")) {
    sensorData.lastDetection = millis();
    totalDetections++;

    // Extract distance
    int idx = data.indexOf("dis=");
    if (idx != -1) {
      String dist = data.substring(idx + 4);
      sensorData.distance = dist.toFloat();
      // if (sensorData.distance > maxDistance) {
      //   maxDistance = sensorData.distance;
      // }
      maxDistance = sensorData.distance;
    }

    // Extract signal strength
    if (config.testMode) {
      idx = data.indexOf("str=");
      if (idx != -1) {
        String str = data.substring(idx + 4);
        sensorData.signalStrength = str.toInt();
      }
    }

    // Update state
    String newState = "";
    if (data.startsWith("mov")) {
      newState = "moving";
      movingCount++;
      Serial.print("[DETECT] MOVING at ");
      Serial.print(sensorData.distance);
      Serial.print("m");
      if (config.testMode) {
        Serial.print(" (signal: ");
        Serial.print(sensorData.signalStrength);
        Serial.print(")");
      }
      Serial.println();
    } else {
      newState = "stationary";
      stationaryCount++;
      Serial.print("[DETECT] STATIONARY at ");
      Serial.print(sensorData.distance);
      Serial.print("m");
      if (config.testMode) {
        Serial.print(" (signal: ");
        Serial.print(sensorData.signalStrength);
        Serial.print(")");
      }
      Serial.println();
    }

    // Check for state change and report immediately
    if (newState != sensorData.state) {
      reportStateChange(newState, sensorData.state, sensorData.distance);
      sensorData.previousState = sensorData.state;
    }
    sensorData.state = newState;
  } else if (data.length() > 0) {
    // Log any non-detection data (could be radar configuration responses)
    Serial.print("[RADAR] Info: ");
    Serial.println(data);
  }
}

void checkTimeout() {
  if (sensorData.lastDetection > 0) {
    if (millis() - sensorData.lastDetection > (config.timeout * 1000)) {
      if (sensorData.state != "none") {
        Serial.print("[TIMEOUT] No detection for ");
        Serial.print(config.timeout);
        Serial.println(" seconds - clearing presence state");

        // Report state change to "none"
        reportStateChange("none", sensorData.state, 0);
        sensorData.previousState = sensorData.state;
      }
      sensorData.state = "none";
      sensorData.distance = 0;
      sensorData.signalStrength = 0;
    }
  }
}

void configureRadar() {
  Serial.println("[RADAR] Configuring radar sensor...");

  // Max detection distance
  String cmd = "rmax=" + String(config.rmax, 1) + "\r\n";
  Serial.print("[RADAR] Sending: ");
  Serial.print(cmd);
  radarSerial.print(cmd);
  delay(100);

  // Movement sensitivity thresholds
  cmd = "mth1_mov=" + String(config.mth1_mov) + "\r\n";
  Serial.print("[RADAR] Sending: ");
  Serial.print(cmd);
  radarSerial.print(cmd);
  delay(100);

  cmd = "mth2_mov=" + String(config.mth2_mov) + "\r\n";
  Serial.print("[RADAR] Sending: ");
  Serial.print(cmd);
  radarSerial.print(cmd);
  delay(100);

  cmd = "mth3_mov=" + String(config.mth3_mov) + "\r\n";
  Serial.print("[RADAR] Sending: ");
  Serial.print(cmd);
  radarSerial.print(cmd);
  delay(100);

  // Occupancy sensitivity thresholds
  cmd = "mth1_occ=" + String(config.mth1_occ) + "\r\n";
  Serial.print("[RADAR] Sending: ");
  Serial.print(cmd);
  radarSerial.print(cmd);
  delay(100);

  cmd = "mth2_occ=" + String(config.mth2_occ) + "\r\n";
  Serial.print("[RADAR] Sending: ");
  Serial.print(cmd);
  radarSerial.print(cmd);
  delay(100);

  cmd = "mth3_occ=" + String(config.mth3_occ) + "\r\n";
  Serial.print("[RADAR] Sending: ");
  Serial.print(cmd);
  radarSerial.print(cmd);
  delay(100);

  // Save configuration to radar
  Serial.println("[RADAR] Sending: save");
  radarSerial.print("save\r\n");
  delay(100);
  Serial.println("[RADAR] Configuration complete");
}

void saveConfig() {
  Serial.println("[EEPROM] Saving configuration to EEPROM...");
  EEPROM.put(0, config);
  EEPROM.commit();
  Serial.println("[EEPROM] Configuration saved successfully");
}

void loadConfig() {
  Serial.println("[EEPROM] Loading configuration from EEPROM...");
  EEPROM.get(0, config);
  if (config.rmax < 0.4 || config.rmax > 12) {
    Serial.println("[EEPROM] Invalid configuration detected, loading defaults");
    config.rmax = 8.0;
    config.mth1_mov = 60;
    config.mth2_mov = 30;
    config.mth3_mov = 20;
    config.mth1_occ = 60;
    config.mth2_occ = 30;
    config.mth3_occ = 20;
    config.timeout = 30;
    config.testMode = false;
    saveConfig();
  } else {
    Serial.println("[EEPROM] Valid configuration loaded:");
    Serial.print("  rmax=");
    Serial.print(config.rmax);
    Serial.print("m, mth_mov=");
    Serial.print(config.mth1_mov);
    Serial.print("/");
    Serial.print(config.mth2_mov);
    Serial.print("/");
    Serial.print(config.mth3_mov);
    Serial.print(", mth_occ=");
    Serial.print(config.mth1_occ);
    Serial.print("/");
    Serial.print(config.mth2_occ);
    Serial.print("/");
    Serial.print(config.mth3_occ);
    Serial.print(", timeout=");
    Serial.print(config.timeout);
    Serial.print("s, testMode=");
    Serial.println(config.testMode ? "ON" : "OFF");
  }
}  
