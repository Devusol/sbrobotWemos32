#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include <Arduino.h>
#include "control/input_controller.h"
#include "self_balancing/balance.h"

struct ControlParams {
    float targetAngle;
    float deadBand;
};

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
ControlParams handleTargetAngle(float targetDelta, float deadbandDelta); // Adjust target angle by delta
void handleKeyboardInputs();

// HTTP command handling
void handleRobotCommand(String command, String value);

#endif
