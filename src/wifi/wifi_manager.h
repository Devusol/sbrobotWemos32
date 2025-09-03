#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <LittleFS.h>

// Serial capturing functionality
extern String serialBuffer;
extern const int MAX_SERIAL_BUFFER;
extern WiFiClient sseClient;
extern bool sseConnected;

void addToSerialBuffer(String message);

// Macros to replace Serial calls with capturing versions
#define SERIAL_PRINT(x) { Serial.print(x); addToSerialBuffer(String(x)); }
#define SERIAL_PRINTLN(x) { Serial.println(x); addToSerialBuffer(String(x)); }
#define SERIAL_PRINTLN_VOID() { Serial.println(); addToSerialBuffer(""); }

// Helper macros for complex expressions
#define SERIAL_PRINTLN_STR(x) SERIAL_PRINTLN(String(x))

extern char ssid[32];
extern char password[64];
extern const char* ap_ssid;
extern const char* ap_password;
extern Preferences preferences;
extern WiFiServer server;
extern String scannedNetworks;

void initWiFi();
void switchToAPMode();
void onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void handleWebServer();
void scanNetworks();

#endif
