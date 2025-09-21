#include "wifi_manager.h"
#include "control/input_controller.h"
#include "self_balancing/balance.h"
#include <ArduinoJson.h>

bool ledState = 0;
#define LED_PIN 2

// External declarations for global variables from main.cpp
extern GyroOffsets gyroOffsets;
extern AccelOffsets accelOffsets;
extern float currentAngle;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Network scan results
String scannedNetworks = "";

// Serial console buffer
String serialBuffer = "";
const int MAX_SERIAL_BUFFER = 1000; // Limit buffer size to prevent memory issues

// Function prototypes for async handlers
void handleStatus(AsyncWebServerRequest *request);
void handleSaveWiFi(AsyncWebServerRequest *request);
void handleScanNetworks(AsyncWebServerRequest *request);
void handleControl(AsyncWebServerRequest *request);
void handleGetPID(AsyncWebServerRequest *request);
void handleSetPID(AsyncWebServerRequest *request);
void handleCalibrate(AsyncWebServerRequest *request);
void handleClearConsole(AsyncWebServerRequest *request);
void initRoutes();

void notifyClients()
{
  ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    String message = String((char *)data);

    if (message == "toggle")
    {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
      ws.textAll(String(ledState));
    }
    else if (message == "get-buffer")
    {
      // Send current serial buffer to the client
      if (serialBuffer.length() > 0)
      {
        ws.textAll(serialBuffer);
      }
    }
    else
    {
      // Try to parse as JSON
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, message);
      if (!error)
      {
        String type = doc["type"];
        if (type == "get-pid")
        {
          String json = "{";
          json += "\"type\":\"pid-values\",";
          json += "\"kp\":" + String(balancePID.kp, 3) + ",";
          json += "\"ki\":" + String(balancePID.ki, 3) + ",";
          json += "\"kd\":" + String(balancePID.kd, 3);
          json += "}";
          ws.textAll(json);
        }
        else if (type == "set-pid")
        {
          float kp = doc["kp"];
          float ki = doc["ki"];
          float kd = doc["kd"];
          balancePID.kp = kp;
          balancePID.ki = ki;
          balancePID.kd = kd;
          SERIAL_PRINTLN("PID values updated via WS: Kp=" + String(kp, 3) + ", Ki=" + String(ki, 3) + ", Kd=" + String(kd, 3));
          String response = "{\"type\":\"pid-updated\",\"success\":true}";
          ws.textAll(response);
        }
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    // Send current serial buffer to new client
    if (serialBuffer.length() > 0)
    {
      client->text(serialBuffer);
    }
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebServerWithWebSocket()
{
  // WebSocket handler
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  // Route handlers
  initRoutes();

  // Start server
  server.begin();
}

// Function to scan available networks
void scanNetworks()
{
  SERIAL_PRINTLN("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();
  scannedNetworks = "";
  if (n == 0)
  {
    scannedNetworks = "<option>No networks found</option>";
  }
  else
  {
    for (int i = 0; i < n; ++i)
    {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      String encryption = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      scannedNetworks += "<option value='" + ssid + "'>" + ssid + " (" + String(rssi) + "dBm) " + encryption + "</option>";
    }
  }
  SERIAL_PRINTLN("Scan complete. Found " + String(n) + " networks.");
}

// Add message to serial buffer and broadcast to WebSocket clients
void addToSerialBuffer(String message)
{
  // Add timestamp
  String timestampedMessage = "[" + String(millis()) + "] " + message + "\n";

  // Add to buffer
  serialBuffer += timestampedMessage;

  // Keep buffer size manageable
  if (serialBuffer.length() > MAX_SERIAL_BUFFER)
  {
    int excess = serialBuffer.length() - MAX_SERIAL_BUFFER;
    serialBuffer = serialBuffer.substring(excess);
  }

  // Broadcast to WebSocket clients (only if queue not full)
  if (ws.availableForWriteAll())
  {
    ws.textAll(timestampedMessage);
  }
}

// Send angle data to WebSocket clients
void sendAngleData(float angle, float target)
{
  String jsonData = "{";
  jsonData += "\"type\":\"angle\",";
  jsonData += "\"current\":" + String(angle, 2) + ",";
  jsonData += "\"target\":" + String(target, 2);
  jsonData += "}";

  // Only send if WebSocket can accept messages (prevents queue overflow)
  if (ws.availableForWriteAll())
  {
    ws.textAll(jsonData);
  }
}

// Handle status endpoint
void handleStatus(AsyncWebServerRequest *request)
{
  String json = "{";
  if (WiFi.getMode() == WIFI_AP)
  {
    IPAddress apIP = WiFi.softAPIP();
    json += "\"mode\":\"AP\",";
    json += "\"ssid\":\"" + String(ap_ssid) + "\",";
    json += "\"ip\":\"" + apIP.toString() + "\"";
    SERIAL_PRINTLN("AP Mode - IP: " + apIP.toString());
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    IPAddress localIP = WiFi.localIP();
    json += "\"mode\":\"STA\",";
    json += "\"connected\":true,";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ip\":\"" + localIP.toString() + "\"";
    // Serial.println("STA Mode - IP: " + localIP.toString());
  }
  else
  {
    json += "\"mode\":\"STA\",";
    json += "\"connected\":false,";
    json += "\"ssid\":\"" + String(ssid) + "\"";
    SERIAL_PRINTLN("Not connected to WiFi");
  }
  json += "}";

  request->send(200, "application/json", json);
}

// Handle save WiFi endpoint
void handleSaveWiFi(AsyncWebServerRequest *request)
{
  if (!request->hasParam("ssid", true) || !request->hasParam("password", true))
  {
    request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing ssid or password\"}");
    return;
  }

  String ssidValue = request->getParam("ssid", true)->value();
  String passValue = request->getParam("password", true)->value();

  if (ssidValue.length() > 0)
  {
    ssidValue.toCharArray(ssid, sizeof(ssid));
    passValue.toCharArray(password, sizeof(password));
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);

    SERIAL_PRINTLN("WiFi credentials updated: " + ssidValue);

    // Attempt reconnection
    WiFi.disconnect(true);
    delay(1000);
    WiFi.mode(WIFI_STA);
    delay(500);
    WiFi.begin(ssid, password);

    // Wait for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
      delay(500);
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      SERIAL_PRINTLN("Reconnected successfully!");
      SERIAL_PRINT("New IP Address: ");
      SERIAL_PRINTLN(WiFi.localIP().toString());
      // Server is already started
    }
    else
    {
      SERIAL_PRINTLN("Reconnection failed");
    }

    request->send(200, "application/json", "{\"success\":true,\"message\":\"WiFi credentials saved\"}");
  }
  else
  {
    request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid credentials\"}");
  }
}

// Handle scan networks endpoint
void handleScanNetworks(AsyncWebServerRequest *request)
{
  scanNetworks();
  String json = "{\"networks\":[";
  // Parse scannedNetworks to JSON
  int start = 0;
  int end = scannedNetworks.indexOf("<option", start);
  bool first = true;
  while (end != -1)
  {
    int valueStart = scannedNetworks.indexOf("value='", end) + 7;
    int valueEnd = scannedNetworks.indexOf("'", valueStart);
    String ssid = scannedNetworks.substring(valueStart, valueEnd);

    int textStart = scannedNetworks.indexOf(">", valueEnd) + 1;
    int textEnd = scannedNetworks.indexOf("</option>", textStart);
    String text = scannedNetworks.substring(textStart, textEnd);

    if (!first)
      json += ",";
    json += "{\"ssid\":\"" + ssid + "\",\"rssi\":\"" + text.substring(text.indexOf("(") + 1, text.indexOf(")")) + "\",\"encryption\":\"" + (text.indexOf("Open") != -1 ? "Open" : "Secured") + "\"}";
    first = false;

    start = textEnd + 9;
    end = scannedNetworks.indexOf("<option", start);
  }
  json += "]}";

  request->send(200, "application/json", json);
}

// Handle control endpoint (for robot commands)
void handleControl(AsyncWebServerRequest *request)
{
  if (!request->hasParam("command", true))
  {
    request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing command\"}");
    return;
  }

  String command = request->getParam("command", true)->value();
  String value = request->hasParam("value", true) ? request->getParam("value", true)->value() : "";

  SERIAL_PRINTLN("Robot command received: " + command + (value.length() > 0 ? " " + value : ""));

  // Handle command using the controller
  handleRobotCommand(command, value);

  request->send(200, "application/json", "{\"success\":true,\"message\":\"Command received: " + command + "\"}");
}

// Handle get PID endpoint
void handleGetPID(AsyncWebServerRequest *request)
{
  String json = "{";
  json += "\"kp\":" + String(balancePID.kp, 3) + ",";
  json += "\"ki\":" + String(balancePID.ki, 3) + ",";
  json += "\"kd\":" + String(balancePID.kd, 3);
  json += "}";

  request->send(200, "application/json", json);
}

// Handle set PID endpoint
void handleSetPID(AsyncWebServerRequest *request)
{
  if (!request->hasParam("kp", true) || !request->hasParam("ki", true) || !request->hasParam("kd", true))
  {
    request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing PID parameters\"}");
    return;
  }

  float kp = request->getParam("kp", true)->value().toFloat();
  float ki = request->getParam("ki", true)->value().toFloat();
  float kd = request->getParam("kd", true)->value().toFloat();

  // Update PID values
  balancePID.kp = kp;
  balancePID.ki = ki;
  balancePID.kd = kd;

  SERIAL_PRINTLN("PID values updated: Kp=" + String(kp, 3) + ", Ki=" + String(ki, 3) + ", Kd=" + String(kd, 3));

  request->send(200, "application/json", "{\"success\":true,\"message\":\"PID values updated\"}");
}

// Handle calibrate endpoint
void handleCalibrate(AsyncWebServerRequest *request)
{
  SERIAL_PRINTLN("Starting sensor calibration...");

  // Perform calibration
  calibrateGyro(gyroOffsets);
  calibrateAccel(accelOffsets);

  SERIAL_PRINTLN("Calibration completed");

  request->send(200, "application/json", "{\"success\":true,\"message\":\"Calibration completed\"}");
}

// Handle clear console endpoint
void handleClearConsole(AsyncWebServerRequest *request)
{
  serialBuffer = "";
  request->send(200, "application/json", "{\"success\":true,\"message\":\"Console cleared\"}");
}

void initRoutes()
{

  // API endpoints
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/scan-networks", HTTP_GET, handleScanNetworks);
  server.on("/get-pid", HTTP_GET, handleGetPID);
  server.on("/clear-console", HTTP_GET, handleClearConsole);

  server.on("/save-wifi", HTTP_POST, handleSaveWiFi);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/set-pid", HTTP_POST, handleSetPID);
  server.on("/calibrate", HTTP_POST, handleCalibrate);

  // Serve other static files
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/script.js", "application/javascript"); });
  server.on("/plotter.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/plotter.js", "application/javascript"); });
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/styles.css", "text/css"); });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/favicon.ico", "image/x-icon"); });
}