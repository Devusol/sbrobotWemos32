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
  // Serial.printf("Gyro: X=%.2f, Y=%.2f, Z=%.2f\n", gyro.x, gyro.y, gyro.z);
  // Serial.printf("Accel: X=%.2f, Y=%.2f, Z=%.2f\n", accel.x, accel.y, accel.z);

  // Display gyro and accelerometer data on OLED using combined function
  oled.displaySensorData(gyro, accel);

  delay(100);
}