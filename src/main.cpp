#include <Arduino.h>
#include "wifi/wifi_manager.h"
#include "control/input_controller.h"
#include "gyro/gyro.h"
#include "display/oled.h"
#include "self_balancing/balance.h"

// Function prototype for sending angle data
void sendAngleData(float angle, float target);

// OLED_Display oled;

// Counter for periodic angle data sending
static int angleSendCounter = 0;

void setup()
{
  Serial.begin(115200);
 
  // if (!oled.begin())
  // {
  //   Serial.println("OLED initialization failed");
  // }
  // else
  // {
  //   Serial.println("OLED initialized successfully");
  //   oled.setBrightness(255);
  //   oled.clearDisplay();
  //   oled.displayText("Robot Initializing...", 0, 0, 1);
  //   oled.updateDisplay();
  // }

  // Initialize the robot controller
  initController();
  initGyro();
  calibrateAll();

  initBalance();
  setSpeed(60); // Set initial speed to 60%

  initWiFi();
}

void loop()
{
  handleKeyboardInputs();

  // Balance the robot
  balanceRobot();

  // // Send angle data to WebSocket clients periodically (every 20 loops ~100ms)
  // angleSendCounter++;
  // if (angleSendCounter >= 10) {
  //   sendAngleData();
  //   angleSendCounter = 0;
  // }



  // Display gyro and accelerometer data on OLED using combined function
  // oled.displaySensorData(gyro, accel);

   // Small delay for stability
  delay(5); // Reduced from 10ms for faster response
}