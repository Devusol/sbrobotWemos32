#include "wifi_manager.h"

// WiFi credentials
char ssid[32] = "Slow_Network";
char password[64] = "W1F1_-Pa55!";

// AP Mode credentials
const char* ap_ssid = "Robot_AP";
const char* ap_password = "robot_password";

Preferences preferences;
// WiFiServer server(80);

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
  preferences.end();
  
  if (!ssid || !password) {
    //fall back to strings defined above
    strlcpy(ssid, "Slow_Network", sizeof(ssid));
    strlcpy(password, "W1F1_-Pa55!", sizeof(password));
  }


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
    SERIAL_PRINTLN(WiFi.localIP());
    initWebServerWithWebSocket();  // Initialize web server with WebSocket
  } else {
    SERIAL_PRINTLN("\nFailed to connect, switching to AP mode.");
    switchToAPMode();
  }
}

void switchToAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress apIP = WiFi.softAPIP();
  SERIAL_PRINT("AP IP Address: ");
  SERIAL_PRINTLN(apIP.toString());
  initWebServerWithWebSocket();  // Initialize web server with WebSocket
  SERIAL_PRINTLN("HTTP server started on port 80");
  
}

void onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  SERIAL_PRINTLN("Wi-Fi disconnected!");
  switchToAPMode();
}
