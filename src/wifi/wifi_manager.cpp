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
    // server.begin();  // Start HTTP server in STA mode
    // SERIAL_PRINTLN("HTTP server started on port 80");
  } else {
    SERIAL_PRINTLN("\nFailed to connect, switching to AP mode.");
    switchToAPMode();
  }
}

void switchToAPMode() {
  SERIAL_PRINTLN("Switching to AP mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress apIP = WiFi.softAPIP();
  SERIAL_PRINT("AP IP Address: ");
  SERIAL_PRINTLN(apIP.toString());
  server.begin();
  SERIAL_PRINTLN("HTTP server started on port 80");
  
  // Add some debugging for AP mode
  SERIAL_PRINTLN("AP mode initialized. Available for connections.");
}

void onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  SERIAL_PRINTLN("Wi-Fi disconnected!");
  switchToAPMode();
}

void handleWebServer();
