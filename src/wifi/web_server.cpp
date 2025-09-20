#include "wifi_manager.h"
#include "control/input_controller.h"
#include "self_balancing/balance.h"

bool ledState = 0;
#define LED_PIN 2

// External declarations for global variables from main.cpp
extern GyroOffsets gyroOffsets;
extern AccelOffsets accelOffsets;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Network scan results
String scannedNetworks = "";

// Serial console buffer
String serialBuffer = "";
const int MAX_SERIAL_BUFFER = 1000; // Limit buffer size to prevent memory issues
WiFiClient sseClient;               // Client for Server-Sent Events
bool sseConnected = false;



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
        if (strcmp((char *)data, "toggle") == 0)
        {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
            notifyClients();
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

  ws.onEvent(onEvent);
  server.addHandler(&ws);

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

// Function to add message to serial buffer and broadcast to SSE clients
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

  // Broadcast to SSE client if connected
  if (sseConnected && sseClient.connected())
  {
    sseClient.println("data: " + timestampedMessage);
    sseClient.println();
    sseClient.flush();
  }
  else if (sseConnected)
  {
    SERIAL_PRINTLN("SSE client disconnected, cleaning up");
    sseConnected = false;
    sseClient.stop();
  }
}

// URL decode function
String urlDecode(String str)
{
  String decoded = "";
  for (int i = 0; i < str.length(); i++)
  {
    if (str[i] == '+')
    {
      decoded += ' ';
    }
    else if (str[i] == '%')
    {
      if (i + 2 < str.length())
      {
        String hex = str.substring(i + 1, i + 3);
        char c = (char)strtol(hex.c_str(), NULL, 16);
        decoded += c;
        i += 2;
      }
    }
    else
    {
      decoded += str[i];
    }
  }
  return decoded;
}

// Parse form data from POST body
bool parseFormData(String body, String &ssidValue, String &passValue)
{
  ssidValue = "";
  passValue = "";

  // Split by &
  int start = 0;
  int end = body.indexOf('&', start);

  while (end != -1 || start < body.length())
  {
    String pair;
    if (end == -1)
    {
      pair = body.substring(start);
    }
    else
    {
      pair = body.substring(start, end);
    }

    int eqIndex = pair.indexOf('=');
    if (eqIndex != -1)
    {
      String key = pair.substring(0, eqIndex);
      String value = urlDecode(pair.substring(eqIndex + 1));

      if (key == "ssid")
      {
        ssidValue = value;
      }
      else if (key == "password")
      {
        passValue = value;
      }
    }

    if (end == -1)
      break;
    start = end + 1;
    end = body.indexOf('&', start);
  }

  return ssidValue.length() > 0 && passValue.length() > 0;
}

// Function to generate HTML response
String generateHTML(bool credentialsUpdated, bool scanRequested)
{
  String html = "";
  html += "<!DOCTYPE html>";
  html += "<html lang='en'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>WiFi Configuration</title>";
  html += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
  html += "<style>";
  html += "#serialConsole {";
  html += "  height: 300px;";
  html += "  overflow-y: auto;";
  html += "  background-color: #000;";
  html += "  color: #0f0;";
  html += "  font-family: 'Courier New', monospace;";
  html += "  font-size: 12px;";
  html += "  padding: 10px;";
  html += "  border: 1px solid #ccc;";
  html += "  border-radius: 4px;";
  html += "}";
  html += "</style>";
  html += "</head>";
  html += "<body class='bg-light'>";
  html += "<div class='container mt-5'>";
  html += "<div class='row justify-content-center'>";
  html += "<div class='col-md-8'>";
  html += "<div class='card'>";
  html += "<div class='card-header'>";
  html += "<h1 class='card-title'>WiFi Configuration</h1>";
  if (credentialsUpdated)
  {
    html += "<div class='alert alert-success'>Credentials updated successfully! Attempting to reconnect...</div>";
  }
  else if (scanRequested)
  {
    html += "<div class='alert alert-info'>Network scan completed. Select a network from the dropdown.</div>";
  }
  html += "</div>";
  html += "<div class='card-body'>";

  // Status display
  if (WiFi.getMode() == WIFI_AP)
  {
    html += "<div class='alert alert-info'>";
    html += "<strong>Status:</strong> Access Point Mode<br>";
    html += "Network: " + String(ap_ssid) + "<br>";
    html += "IP Address: " + WiFi.softAPIP().toString();
    html += "</div>";
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    html += "<div class='alert alert-success'>";
    html += "<strong>Status:</strong> Connected<br>";
    html += "Network: " + WiFi.SSID() + "<br>";
    html += "IP Address: " + WiFi.localIP().toString();
    html += "</div>";
  }
  else
  {
    html += "<div class='alert alert-warning'>";
    html += "<strong>Status:</strong> Not Connected";
    html += "</div>";
  }

  html += "<form method='POST'>";
  html += "<div class='mb-3'>";
  html += "<label for='ssid' class='form-label'>SSID</label>";
  html += "<div class='input-group'>";
  html += "<input type='text' class='form-control' id='ssid' name='ssid' value='" + String(ssid) + "' required>";
  html += "<button type='submit' name='scan' value='1' class='btn btn-outline-secondary'>Scan Networks</button>";
  html += "</div>";
  html += "<select class='form-select mt-2' id='networkSelect' onchange='document.getElementById(\"ssid\").value = this.value;'>";
  html += "<option value=''>Select from scanned networks...</option>";
  html += scannedNetworks;
  html += "</select>";
  html += "</div>";
  html += "<div class='mb-3'>";
  html += "<label for='password' class='form-label'>Password</label>";
  html += "<input type='password' class='form-control' id='password' name='password' value='" + String(password) + "' required>";
  html += "</div>";
  html += "<button type='submit' class='btn btn-primary'>Save</button>";
  html += "</form>";

  // Serial Console Section
  html += "<hr>";
  html += "<h4>Serial Console</h4>";
  html += "<div id='serialConsole'>";
  html += serialBuffer; // Display current buffer contents
  html += "</div>";
  html += "<div class='mt-2'>";
  html += "<button type='button' class='btn btn-secondary btn-sm' onclick='clearConsole()'>Clear Console</button>";
  html += "<button type='button' class='btn btn-secondary btn-sm ms-2' onclick='scrollToBottom()'>Scroll to Bottom</button>";
  html += "</div>";

  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";

  // JavaScript for SSE and console management
  html += "<script>";
  html += "let eventSource;";
  html += "let consoleElement;";
  html += "";
  html += "function initConsole() {";
  html += "  consoleElement = document.getElementById('serialConsole');";
  html += "  connectSSE();";
  html += "}";
  html += "";
  html += "function connectSSE() {";
  html += "  eventSource = new EventSource('/events');";
  html += "  eventSource.onmessage = function(event) {";
  html += "    if (consoleElement) {";
  html += "      consoleElement.textContent += event.data;";
  html += "      consoleElement.scrollTop = consoleElement.scrollHeight;";
  html += "    }";
  html += "  };";
  html += "  eventSource.onerror = function() {";
  html += "    setTimeout(connectSSE, 5000); // Reconnect after 5 seconds";
  html += "  };";
  html += "}";
  html += "";
  html += "function clearConsole() {";
  html += "  if (consoleElement) {";
  html += "    consoleElement.textContent = '';";
  html += "  }";
  html += "  fetch('/clear-console');";
  html += "}";
  html += "";
  html += "function scrollToBottom() {";
  html += "  if (consoleElement) {";
  html += "    consoleElement.scrollTop = consoleElement.scrollHeight;";
  html += "  }";
  html += "}";
  html += "";
  html += "window.onload = initConsole;";
  html += "</script>";

  html += "</body>";
  html += "</html>";
  return html;
}

// Request handler enum for cleaner code organization
enum RequestType
{
  REQ_ROOT,
  REQ_SCRIPT,
  REQ_STATUS,
  REQ_SCAN_NETWORKS,
  REQ_SAVE_WIFI,
  REQ_CONTROL,
  REQ_FAVICON,
  REQ_GET_PID,
  REQ_SET_PID,
  REQ_CALIBRATE,
  REQ_UNKNOWN
};

// Helper function to determine request type
RequestType getRequestType(String path, String method)
{
  if (method == "GET")
  {
    if (path == "/" || path == "/index.html")
      return REQ_ROOT;
    if (path == "/script.js")
      return REQ_SCRIPT;
    if (path == "/status")
      return REQ_STATUS;
    if (path == "/scan-networks")
      return REQ_SCAN_NETWORKS;
    if (path == "/favicon.ico")
      return REQ_FAVICON;
    if (path == "/get-pid")
      return REQ_GET_PID;
  }
  else if (method == "POST")
  {
    if (path == "/save-wifi")
      return REQ_SAVE_WIFI;
    if (path == "/control")
      return REQ_CONTROL;
    if (path == "/set-pid")
      return REQ_SET_PID;
    if (path == "/calibrate")
      return REQ_CALIBRATE;
  }
  return REQ_UNKNOWN;
}

// Function prototypes
void serveFile(WiFiClient client, String path, String contentType);
void handleStatus(WiFiClient client);
void handleSaveWiFi(WiFiClient client, String body);
void handleScanNetworks(WiFiClient client);
void handleControl(WiFiClient client, String body);
void handleGetPID(WiFiClient client);
void handleSetPID(WiFiClient client, String body);
void handleCalibrate(WiFiClient client);

// Function to handle web server requests
void handleWebServer()
{
  WiFiClient client = server.available();
  if (client)
  {
    String request = client.readStringUntil('\r');
    client.readStringUntil('\n'); // consume \n

    // Check if this is an SSE request
    if (request.indexOf("GET /events") >= 0)
    {
      SERIAL_PRINTLN("SSE connection established");

      // Read headers to consume them
      String line;
      do
      {
        line = client.readStringUntil('\r');
        client.readStringUntil('\n');
      } while (line.length() > 0);

      // Send SSE headers
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/event-stream");
      client.println("Cache-Control: no-cache");
      client.println("Connection: keep-alive");
      client.println("Access-Control-Allow-Origin: *");
      client.println();
      client.flush();

      // Store the client for SSE
      sseClient = client;
      sseConnected = true;

      // Send current buffer contents
      if (serialBuffer.length() > 0)
      {
        sseClient.println("data: " + serialBuffer);
        sseClient.println();
        sseClient.flush();
      }

      // Send initial connection message
      sseClient.println("data: [SSE] Connection established");
      sseClient.println();
      sseClient.flush();

      return; // Don't close SSE connection
    }

    // Check if this is a clear-console request
    if (request.indexOf("GET /clear-console") >= 0)
    {
      // Read headers to consume them
      String line;
      do
      {
        line = client.readStringUntil('\r');
        client.readStringUntil('\n');
      } while (line.length() > 0);

      // Clear console request
      serialBuffer = "";
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("Console cleared");
      client.stop();
      return;
    }

    // Read and parse headers for regular requests
    String line;
    int contentLength = 0;
    do
    {
      line = client.readStringUntil('\r');
      client.readStringUntil('\n');
      if (line.startsWith("Content-Length: "))
      {
        contentLength = line.substring(16).toInt();
      }
    } while (line.length() > 0);

    String method = request.substring(0, request.indexOf(' '));
    String path = request.substring(request.indexOf(' ') + 1, request.indexOf(' ', request.indexOf(' ') + 1));

    // Determine request type
    RequestType reqType = getRequestType(path, method);

    // Handle GET requests
    if (method == "GET")
    {
      switch (reqType)
      {
      case REQ_ROOT:
        serveFile(client, "/index.html", "text/html");
        break;
      case REQ_SCRIPT:
        serveFile(client, "/script.js", "application/javascript");
        break;
      case REQ_STATUS:
        handleStatus(client);
        break;
      case REQ_SCAN_NETWORKS:
        handleScanNetworks(client);
        break;
      case REQ_GET_PID:
        handleGetPID(client);
        break;
      case REQ_FAVICON:
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Favicon not found");
        break;
      default:
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("404 Not Found");
        break;
      }
    }
    else if (method == "POST")
    {
      // Read POST body based on Content-Length
      String body = "";
      if (contentLength > 0)
      {
        int bytesRead = 0;
        unsigned long startTime = millis();
        while (bytesRead < contentLength && millis() - startTime < 5000)
        { // 5 second timeout
          if (client.available())
          {
            body += (char)client.read();
            bytesRead++;
          }
          delay(1); // Small delay to prevent busy waiting
        }
        if (bytesRead < contentLength)
        {
          SERIAL_PRINTLN("Timeout reading POST body");
        }
      }

      switch (reqType)
      {
      case REQ_SAVE_WIFI:
        handleSaveWiFi(client, body);
        break;
      case REQ_CONTROL:
        handleControl(client, body);
        break;
      case REQ_SET_PID:
        handleSetPID(client, body);
        break;
      case REQ_CALIBRATE:
        handleCalibrate(client);
        break;
      default:
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("404 Not Found");
        break;
      }
    }

    client.stop();
  }

  // Handle existing SSE connection keep-alive
  if (sseConnected && sseClient.connected())
  {
    // Send keep-alive every few seconds
    static unsigned long lastKeepAlive = 0;
    if (millis() - lastKeepAlive > 30000)
    { // 30 seconds
      sseClient.println(": keep-alive");
      sseClient.println();
      sseClient.flush();
      lastKeepAlive = millis();
    }
  }
  else if (sseConnected)
  {
    SERIAL_PRINTLN("SSE client disconnected");
    sseConnected = false;
  }
}

// Helper function to serve static files from LittleFS
void serveFile(WiFiClient client, String path, String contentType)
{
  SERIAL_PRINTLN("Serving file: " + path);
  if (LittleFS.exists(path))
  {
    SERIAL_PRINTLN("File exists, opening...");
    File file = LittleFS.open(path, "r");
    if (file)
    {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: " + contentType);
      client.println("Connection: close");
      client.println();
      size_t bytesSent = 0;
      while (file.available())
      {
        client.write(file.read());
        bytesSent++;
      }
      file.close();
      SERIAL_PRINTLN("File sent successfully, bytes: " + String(bytesSent));
    }
    else
    {
      SERIAL_PRINTLN("Error opening file");
      client.println("HTTP/1.1 500 Internal Server Error");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("Error opening file");
    }
  }
  else
  {
    SERIAL_PRINTLN("File not found: " + path);
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("File not found: " + path);
  }
}

// Handle status endpoint
void handleStatus(WiFiClient client)
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
    SERIAL_PRINTLN("STA Mode - IP: " + localIP.toString());
  }
  else
  {
    json += "\"mode\":\"STA\",";
    json += "\"connected\":false,";
    json += "\"ssid\":\"" + String(ssid) + "\"";
    SERIAL_PRINTLN("Not connected to WiFi");
  }
  json += "}";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  client.println(json);
}

// Handle save WiFi endpoint
void handleSaveWiFi(WiFiClient client, String body)
{
  // Parse JSON body
  String ssidValue = "";
  String passValue = "";

  int ssidStart = body.indexOf("\"ssid\":\"") + 8;
  int ssidEnd = body.indexOf("\"", ssidStart);
  if (ssidStart > 7 && ssidEnd > ssidStart)
  {
    ssidValue = body.substring(ssidStart, ssidEnd);
  }

  int passStart = body.indexOf("\"password\":\"") + 12;
  int passEnd = body.indexOf("\"", passStart);
  if (passStart > 11 && passEnd > passStart)
  {
    passValue = body.substring(passStart, passEnd);
  }

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
      server.begin(); // Start server after reconnection
    }
    else
    {
      SERIAL_PRINTLN("Reconnection failed");
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    client.println("{\"success\":true,\"message\":\"WiFi credentials saved\"}");
  }
  else
  {
    client.println("HTTP/1.1 400 Bad Request");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    client.println("{\"success\":false,\"message\":\"Invalid credentials\"}");
  }
}

// Handle scan networks endpoint
void handleScanNetworks(WiFiClient client)
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

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  client.println(json);
}

// Handle control endpoint (for robot commands)
void handleControl(WiFiClient client, String body)
{
  // Parse JSON body
  String command = "";
  String value = "";

  int cmdStart = body.indexOf("\"command\":\"") + 11;
  int cmdEnd = body.indexOf("\"", cmdStart);
  if (cmdStart > 10 && cmdEnd > cmdStart)
  {
    command = body.substring(cmdStart, cmdEnd);
  }

  int valStart = body.indexOf("\"value\":\"") + 9;
  int valEnd = body.indexOf("\"", valStart);
  if (valStart > 8 && valEnd > valStart)
  {
    value = body.substring(valStart, valEnd);
  }

  SERIAL_PRINTLN("Robot command received: " + command + (value.length() > 0 ? " " + value : ""));

  // Handle command using the controller
  handleRobotCommand(command, value);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  client.println("{\"success\":true,\"message\":\"Command received: " + command + "\"}");
}

// Handle get PID endpoint
void handleGetPID(WiFiClient client)
{
  String json = "{";
  json += "\"kp\":" + String(balancePID.kp, 3) + ",";
  json += "\"ki\":" + String(balancePID.ki, 3) + ",";
  json += "\"kd\":" + String(balancePID.kd, 3);
  json += "}";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  client.println(json);
}

// Handle set PID endpoint
void handleSetPID(WiFiClient client, String body)
{
  // Parse JSON body
  float kp = balancePID.kp;
  float ki = balancePID.ki;
  float kd = balancePID.kd;

  // Extract kp
  int kpStart = body.indexOf("\"kp\":") + 5;
  int kpEnd = body.indexOf(",", kpStart);
  if (kpEnd == -1)
    kpEnd = body.indexOf("}", kpStart);
  if (kpStart > 4 && kpEnd > kpStart)
  {
    kp = body.substring(kpStart, kpEnd).toFloat();
  }

  // Extract ki
  int kiStart = body.indexOf("\"ki\":") + 5;
  int kiEnd = body.indexOf(",", kiStart);
  if (kiEnd == -1)
    kiEnd = body.indexOf("}", kiStart);
  if (kiStart > 4 && kiEnd > kiStart)
  {
    ki = body.substring(kiStart, kiEnd).toFloat();
  }

  // Extract kd
  int kdStart = body.indexOf("\"kd\":") + 5;
  int kdEnd = body.indexOf("}", kdStart);
  if (kdStart > 4 && kdEnd > kdStart)
  {
    kd = body.substring(kdStart, kdEnd).toFloat();
  }

  // Update PID values
  balancePID.kp = kp;
  balancePID.ki = ki;
  balancePID.kd = kd;

  SERIAL_PRINTLN("PID values updated: Kp=" + String(kp, 3) + ", Ki=" + String(ki, 3) + ", Kd=" + String(kd, 3));

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  client.println("{\"success\":true,\"message\":\"PID values updated\"}");
}

// Handle calibrate endpoint
void handleCalibrate(WiFiClient client)
{
  SERIAL_PRINTLN("Starting sensor calibration...");

  // Perform calibration
  calibrateGyro(gyroOffsets);
  calibrateAccel(accelOffsets);

  SERIAL_PRINTLN("Calibration completed");

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  client.println("{\"success\":true,\"message\":\"Calibration completed\"}");
}