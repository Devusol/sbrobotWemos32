
#include <Arduino.h>
#include "wifi/wifi_manager.h"
#include "control/input_controller.h"

void setup() {
  Serial.begin(115200);

  // Initialize the robot controller
  initController();

  initWiFi();
}

void loop() {
  handleWebServer();
  webSocket.loop();  // Handle WebSocket connections
  // Main code for the robot's balancing loop
}