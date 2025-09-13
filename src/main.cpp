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
  Orientation orientation = readOrientation(gyro, accel);
  // Calculate current angle
  float angle = calculateAngle(accel, gyro);

  // Non-blocking serial input - process each character immediately
  while (Serial.available())
  {
    char c = Serial.read();
    if (c == 'c')
    {
      stopMovement();
      calibrateGyro(gyroOffsets);
      calibrateAccel(accelOffsets);
      Serial.println("Recalibrated gyro and accelerometer.");
    } else if (c == 'i' || c == 'j' || c == 'k' || c == 'l') {
      adjustGyroOffsets(gyroOffsets, gyro, c);
    } else
    {
      adjustPIDGains(c);
    }
  }

  // Balance the robot
  balanceRobot(angle);

  // Handle web server requests
  // handleWebServer();

  // Display gyro and accelerometer data on OLED using combined function
  // oled.displaySensorData(gyro, accel);

  // Print orientation less frequently to reduce serial lag
  // static int printCounter = 0;
  // printCounter++;
  // if (printCounter >= 20) {  // Print every 20 loops (~100ms at 5ms loop time)
  //   Serial.printf("Orientation - Pitch: %.2f, Roll: %.2f, Yaw: %.2f\n", orientation.pitch, orientation.roll, orientation.yaw);
  //   printCounter = 0;
  // }

  // Small delay for stability
  delay(5); // Reduced from 10ms for faster response
}