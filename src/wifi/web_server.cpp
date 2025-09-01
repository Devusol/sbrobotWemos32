#include "wifi_manager.h"

// Network scan results
String scannedNetworks = "";

// Serial console buffer
String serialBuffer = "";
const int MAX_SERIAL_BUFFER = 1000; // Limit buffer size to prevent memory issues
WiFiClient sseClient; // Client for Server-Sent Events
bool sseConnected = false;

// Function to scan available networks
void scanNetworks() {
  SERIAL_PRINTLN("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();
  scannedNetworks = "";
  if (n == 0) {
    scannedNetworks = "<option>No networks found</option>";
  } else {
    for (int i = 0; i < n; ++i) {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      String encryption = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      scannedNetworks += "<option value='" + ssid + "'>" + ssid + " (" + String(rssi) + "dBm) " + encryption + "</option>";
    }
  }
  SERIAL_PRINTLN("Scan complete. Found " + String(n) + " networks.");
}

// Function to add message to serial buffer and broadcast to SSE clients
void addToSerialBuffer(String message) {
  // Add timestamp
  String timestampedMessage = "[" + String(millis()) + "] " + message + "\n";

  // Add to buffer
  serialBuffer += timestampedMessage;

  // Keep buffer size manageable
  if (serialBuffer.length() > MAX_SERIAL_BUFFER) {
    int excess = serialBuffer.length() - MAX_SERIAL_BUFFER;
    serialBuffer = serialBuffer.substring(excess);
  }

  // Broadcast to SSE client if connected
  if (sseConnected && sseClient.connected()) {
    sseClient.println("data: " + timestampedMessage);
    sseClient.println();
    sseClient.flush();
  }
}

// URL decode function
String urlDecode(String str) {
  String decoded = "";
  for (int i = 0; i < str.length(); i++) {
    if (str[i] == '+') {
      decoded += ' ';
    } else if (str[i] == '%') {
      if (i + 2 < str.length()) {
        String hex = str.substring(i + 1, i + 3);
        char c = (char)strtol(hex.c_str(), NULL, 16);
        decoded += c;
        i += 2;
      }
    } else {
      decoded += str[i];
    }
  }
  return decoded;
}

// Parse form data from POST body
bool parseFormData(String body, String& ssidValue, String& passValue) {
  ssidValue = "";
  passValue = "";

  // Split by &
  int start = 0;
  int end = body.indexOf('&', start);

  while (end != -1 || start < body.length()) {
    String pair;
    if (end == -1) {
      pair = body.substring(start);
    } else {
      pair = body.substring(start, end);
    }

    int eqIndex = pair.indexOf('=');
    if (eqIndex != -1) {
      String key = pair.substring(0, eqIndex);
      String value = urlDecode(pair.substring(eqIndex + 1));

      if (key == "ssid") {
        ssidValue = value;
      } else if (key == "password") {
        passValue = value;
      }
    }

    if (end == -1) break;
    start = end + 1;
    end = body.indexOf('&', start);
  }

  return ssidValue.length() > 0 && passValue.length() > 0;
}

// Function to generate HTML response
String generateHTML(bool credentialsUpdated, bool scanRequested) {
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
  if (credentialsUpdated) {
    html += "<div class='alert alert-success'>Credentials updated successfully! Attempting to reconnect...</div>";
  } else if (scanRequested) {
    html += "<div class='alert alert-info'>Network scan completed. Select a network from the dropdown.</div>";
  }
  html += "</div>";
  html += "<div class='card-body'>";

  // Status display
  if (WiFi.getMode() == WIFI_AP) {
    html += "<div class='alert alert-info'>";
    html += "<strong>Status:</strong> Access Point Mode<br>";
    html += "Network: " + String(ap_ssid) + "<br>";
    html += "IP Address: " + WiFi.softAPIP().toString();
    html += "</div>";
  } else if (WiFi.status() == WL_CONNECTED) {
    html += "<div class='alert alert-success'>";
    html += "<strong>Status:</strong> Connected<br>";
    html += "Network: " + WiFi.SSID() + "<br>";
    html += "IP Address: " + WiFi.localIP().toString();
    html += "</div>";
  } else {
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

// Function to handle SSE connections
void handleSSE() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');

    if (request.indexOf("GET /events") >= 0) {
      // SSE connection
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/event-stream");
      client.println("Cache-Control: no-cache");
      client.println("Connection: keep-alive");
      client.println("Access-Control-Allow-Origin: *");
      client.println();

      sseClient = client;
      sseConnected = true;

      // Send current buffer contents
      if (serialBuffer.length() > 0) {
        sseClient.println("data: " + serialBuffer);
        sseClient.println();
        sseClient.flush();
      }

      return; // Don't close this connection
    } else if (request.indexOf("GET /clear-console") >= 0) {
      // Clear console request
      serialBuffer = "";
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println();
      client.println("Console cleared");
      client.stop();
      return;
    }
  }
}

// Function to handle web server requests
void handleWebServer() {
  // Handle SSE first
  handleSSE();

  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.readStringUntil('\n'); // consume \n

    // Skip SSE and clear-console requests
    if (request.indexOf("/events") >= 0 || request.indexOf("/clear-console") >= 0) {
      return;
    }

    // Read headers until blank line
    String line;
    do {
      line = client.readStringUntil('\r');
      client.readStringUntil('\n');
    } while (line.length() > 0);

    bool credentialsUpdated = false;
    bool scanRequested = false;
    if (request.indexOf("POST") >= 0) {
      // Read body
      String body = "";
      while (client.available()) {
        body += (char)client.read();
      }

      // Check if scan was requested
      if (body.indexOf("scan=1") >= 0) {
        scanRequested = true;
        scanNetworks();
      } else {
        // Parse form data using improved parser
        String ssidValue = "";
        String passValue = "";

        SERIAL_PRINTLN("Raw POST body: " + body);

        if (parseFormData(body, ssidValue, passValue)) {
          SERIAL_PRINTLN("Parsed SSID: '" + ssidValue + "'");
          SERIAL_PRINTLN("Parsed Password: '" + passValue + "'");

          // Copy to global variables
          ssidValue.toCharArray(ssid, sizeof(ssid));
          passValue.toCharArray(password, sizeof(password));

          // Save to preferences
          preferences.putString("ssid", ssid);
          preferences.putString("password", password);

          SERIAL_PRINTLN("WiFi credentials updated and saved");
          SERIAL_PRINTLN("New SSID: " + String(ssid));
          SERIAL_PRINTLN("New Password length: " + String(strlen(password)));

          credentialsUpdated = true;
        } else {
          SERIAL_PRINTLN("Failed to parse form data");
          SERIAL_PRINTLN("SSID value: '" + ssidValue + "'");
          SERIAL_PRINTLN("Password value: '" + passValue + "'");
        }
      }
    }

    // Send HTML response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("");
    client.println(generateHTML(credentialsUpdated, scanRequested));
    client.stop();

    // Now attempt reconnection after sending response
    if (credentialsUpdated) {
      SERIAL_PRINTLN("Attempting to reconnect to new WiFi...");
      SERIAL_PRINTLN("Disconnecting from current WiFi...");
      WiFi.disconnect(true);
      delay(1000);

      SERIAL_PRINTLN("Switching to STA mode...");
      WiFi.mode(WIFI_STA);
      delay(500);

      SERIAL_PRINTLN("Connecting to: " + String(ssid));
      WiFi.begin(ssid, password);

      // Wait for connection with timeout
      int attempts = 0;
      const int maxAttempts = 20;
      while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        attempts++;
        SERIAL_PRINT(".");
      }
      SERIAL_PRINTLN();

      if (WiFi.status() == WL_CONNECTED) {
        SERIAL_PRINTLN("Reconnected successfully!");
        SERIAL_PRINT("IP Address: ");
        SERIAL_PRINTLN(WiFi.localIP());
        // AP mode disabled, web server stops
      } else {
        SERIAL_PRINTLN("Reconnection failed, switching back to AP mode");
        SERIAL_PRINTLN("WiFi status: " + String(WiFi.status()));
        WiFi.mode(WIFI_AP);
        delay(500);
        WiFi.softAP(ap_ssid, ap_password);
        server.begin();
        SERIAL_PRINT("AP IP Address: ");
        SERIAL_PRINTLN(WiFi.softAPIP());
      }
    }
  }
}