#include "wifi_manager.h"
#include "control/input_controller.h"

// Function prototypes
void parseWebSocketCommand(uint8_t clientNum, String jsonMessage);

// WiFi credentials
char ssid[32] = "YOUR_WIFI_SSID";
char password[64] = "YOUR_WIFI_PASSWORD";

// AP Mode credentials
const char* ap_ssid = "Robot_AP";
const char* ap_password = "robot_password";

Preferences preferences;
WiFiServer server(80);
WebSocketsServer webSocket(81); // WebSocket server on port 81

void initWiFi() {
  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    SERIAL_PRINTLN("LittleFS Mount Failed");
    return;
  }
  SERIAL_PRINTLN("LittleFS mounted successfully");

  preferences.begin("wifi", false);
  preferences.getString("ssid", ssid, sizeof(ssid));
  preferences.getString("password", password, sizeof(password));
  WiFi.onEvent(onWiFiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  SERIAL_PRINT("Connecting to WiFi ..");

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    SERIAL_PRINT('.');
    delay(500);
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    SERIAL_PRINTLN("\nConnected to WiFi!");
    SERIAL_PRINT("IP Address: ");
    SERIAL_PRINTLN(WiFi.localIP().toString());
    server.begin();  // Start HTTP server in STA mode
    
    // Start WebSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    setWebSocketServer(&webSocket);  // Set WebSocket pointer for controller
    SERIAL_PRINTLN("WebSocket server started on port 81");
  } else {
    SERIAL_PRINTLN("\nFailed to connect, switching to AP mode.");
    switchToAPMode();
  }
}

void switchToAPMode() {
  SERIAL_PRINTLN("Switching to AP mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  SERIAL_PRINT("AP IP Address: ");
  SERIAL_PRINTLN(WiFi.softAPIP().toString());
  server.begin();
  
  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  setWebSocketServer(&webSocket);  // Set WebSocket pointer for controller
  SERIAL_PRINTLN("WebSocket server started on port 81");
}

void onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  SERIAL_PRINTLN("Wi-Fi disconnected!");
  switchToAPMode();
}

void handleWebServer();

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      SERIAL_PRINTLN("WebSocket client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      SERIAL_PRINTLN("WebSocket client " + String(num) + " connected from " + ip.toString());
      
      // Send welcome message
      webSocket.sendTXT(num, "Connected to Robot Control WebSocket");
    }
      break;
    case WStype_TEXT: {
      String message = String((char*)payload);
      SERIAL_PRINTLN("WebSocket message from client " + String(num) + ": " + message);
      
      // Parse JSON message and handle commands
      if (message.startsWith("{")) {
        // Handle JSON commands
        parseWebSocketCommand(num, message);
      } else {
        // Echo back for testing
        webSocket.sendTXT(num, "Echo: " + message);
      }
    }
      break;
    case WStype_BIN:
      SERIAL_PRINTLN("WebSocket binary message received");
      break;
    case WStype_ERROR:
      SERIAL_PRINTLN("WebSocket error");
      break;
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      SERIAL_PRINTLN("WebSocket fragment received");
      break;
  }
}

// Parse WebSocket commands
void parseWebSocketCommand(uint8_t clientNum, String jsonMessage) {
  // Parse JSON command
  String command = "";
  String value = "";

  int cmdStart = jsonMessage.indexOf("\"command\":\"") + 11;
  int cmdEnd = jsonMessage.indexOf("\"", cmdStart);
  if (cmdStart > 10 && cmdEnd > cmdStart) {
    command = jsonMessage.substring(cmdStart, cmdEnd);
  }

  int valStart = jsonMessage.indexOf("\"value\":\"") + 9;
  int valEnd = jsonMessage.indexOf("\"", valStart);
  if (valStart > 8 && valEnd > valStart) {
    value = jsonMessage.substring(valStart, valEnd);
  }

  SERIAL_PRINTLN("WebSocket command: " + command + (value.length() > 0 ? " " + value : ""));

  // Handle commands through the controller
  handleWebSocketCommand(clientNum, command, value);
}
