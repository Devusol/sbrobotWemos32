#include <Arduino.h>
#include "wifi/wifi_manager.h"
#include "control/input_controller.h"
#include "gyro/gyro.h"
#include "display/oled.h"
#include "self_balancing/balance.h"

// OLED_Display oled;

float targetAngle = 87.0;

// Deadband for error to reduce noise
float deadBand = 0.0; // degrees

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

  // calibrateGyro(gyroOffsets);
  // calibrateAccel(accelOffsets);
  initBalance();
  setSpeed(60); // Set initial speed to 60%

  initWiFi();
}

void loop()
{
  // handleWebServer();
  // Main code for the robot's balancing loop
  // GyroData gyro = readGyro(gyroOffsets);
  // AccelData accel = readAccel(accelOffsets);
  // Orientation orientation = readOrientation(gyro, accel);
  // Calculate current angle
  // float angle = calculateAngle(accel, gyro);
  // Serial.printf("Angle: %.2f\n", angle);
  // Non-blocking serial input - process each character immediately
  while (Serial.available())
  {
    char key = Serial.read();
    if (key == 'c')
    {
      stopMovement();
      delay(1000); // Small delay to ensure stop command is processed
      calibrateAll();
      targetAngle = 87.0;
      Serial.println("Recalibrated gyro and accelerometer.");
    }
    else if (key == 'v')
    {
      targetAngle += 0.1; // Increase target angle by 0.1 degree
      Serial.println("Target angle increased to " + String(targetAngle) + " degrees.");
    }
    else if (key == 'b')
    {
      targetAngle -= 0.1; // Decrease target angle by 0.1 degree
      Serial.println("Target angle decreased to " + String(targetAngle) + " degrees.");
    }
    else if (key == 't')
    {
      deadBand += 1; // Increase deadband by 1 degree
      Serial.println("Deadband increased to " + String(deadBand) + " degrees.");
    }
    else if (key == 'g')
    {
      deadBand -= 1; // Decrease deadband by 1 degree
      if (deadBand < 0) deadBand = 0; // Prevent negative deadband
      Serial.println("Deadband decreased to " + String(deadBand) + " degrees.");
    }
    else
    {
      adjustPIDGains(key);
    }
  }

  // Balance the robot
  balanceRobot(targetAngle, deadBand);

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