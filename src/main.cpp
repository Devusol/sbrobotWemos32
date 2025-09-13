#include <Arduino.h>
#include "wifi/wifi_manager.h"
#include "control/input_controller.h"
#include "gyro/gyro.h"
#include "display/oled.h"
#include "self_balancing/balance.h"

OLED_Display oled;
GyroOffsets gyroOffsets;
AccelOffsets accelOffsets;

void setup()
{
  Serial.begin(115200);
  // Initialize OLED display
  if (!oled.begin())
  {
    Serial.println("OLED initialization failed");
  }
  else
  {
    Serial.println("OLED initialized successfully");
    // Set brightness to a higher level (255 out of 255 for maximum visibility)
    oled.setBrightness(255);
    oled.clearDisplay();
    oled.displayText("Robot Initializing...", 0, 0, 1);
    oled.updateDisplay();
  }

  // Initialize the robot controller
  initController();
  initGyro();
  calibrateGyro(gyroOffsets);
  calibrateAccel(accelOffsets);
  initBalance();
  setSpeed(60); // Set initial speed to 60%

  // initWiFi();
}

void loop()
{
  // handleWebServer();
  // Main code for the robot's balancing loop
  GyroData gyro = readGyro(gyroOffsets);
  AccelData accel = readAccel(accelOffsets);

  // Calculate current angle
  float angle = calculateAngle(accel, gyro);

  // Balance the robot
  balanceRobot(angle);
  // motorTest();

  // Display gyro and accelerometer data on OLED using combined function
  // oled.displaySensorData(gyro, accel);

  // Small delay for stability
  delay(10);
}