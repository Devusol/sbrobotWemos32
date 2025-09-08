#include <Arduino.h>
#include "wifi/wifi_manager.h"
#include "control/input_controller.h"
#include "gyro/gyro.h"
#include "display/oled.h"

OLED_Display oled;
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

  initWiFi();
}

void loop()
{
  handleWebServer();
  // Main code for the robot's balancing loop

  GyroData gyro = readGyro();
  AccelData accel = readAccel();
  // Serial.println("Gyro: X=-0.12, Y=0.05, Z=0.98");
  Serial.printf("Gyro: X=%.2f, Y=%.2f, Z=%.2f\n", gyro.x, gyro.y, gyro.z);
  // Serial.printf("Accel: X=%.2f, Y=%.2f, Z=%.2f\n", accel.x, accel.y, accel.z);

  // Display gyro and accelerometer data on OLED
  oled.clearDisplay();
  oled.displayText("Gyroscope:", 0, 0, 1);
  char gyroStr[30];
  sprintf(gyroStr, "X: %.2f", gyro.x);
  oled.displayText(gyroStr, 0, 10, 1);
  sprintf(gyroStr, "Y: %.2f", gyro.y);
  oled.displayText(gyroStr, 0, 20, 1);
  sprintf(gyroStr, "Z: %.2f", gyro.z);
  oled.displayText(gyroStr, 0, 30, 1);

  oled.displayText("Accelerometer:", 0, 40, 1);
  char accelStr[30];
  sprintf(accelStr, "X: %.2f", accel.x);
  oled.displayText(accelStr, 0, 50, 1);
  sprintf(accelStr, "Y: %.2f", accel.y);
  oled.displayText(accelStr, 60, 50, 1);
  sprintf(accelStr, "Z: %.2f", accel.z);
  oled.displayText(accelStr, 0, 60, 1);

  oled.updateDisplay();

  delay(100);
}