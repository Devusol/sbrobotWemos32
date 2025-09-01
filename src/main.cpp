
#include <Arduino.h>
#include "wifi/wifi_manager.h"

void setup() {
  Serial.begin(115200);
  initWiFi();
}

void loop() {
  handleWebServer();
  // Main code for the robot's balancing loop
}