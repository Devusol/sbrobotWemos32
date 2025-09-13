#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include <Arduino.h>

// Control commands
void initController();
void setSpeed(int speed);  // 0-100
void moveForward();
void moveBackward();
void motorTest();
void turnLeft();
void turnRight();
void stopMovement();
void setMotorSpeeds(int leftSpeed, int rightSpeed);  // -100 to 100

// HTTP command handling
void handleRobotCommand(String command, String value);

#endif
