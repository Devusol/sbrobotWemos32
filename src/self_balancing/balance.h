#ifndef BALANCE_H
#define BALANCE_H

#include <Arduino.h>
#include "gyro/gyro.h"
#include "wifi/wifi_manager.h"
#include "auto_tune.h"

// PID controller structure
struct PIDController
{
    float kp; // Proportional gain
    float ki; // Integral gain
    float kd; // Derivative gain
    float integral;
    float previousError;
    unsigned long lastTime;
    int baseSpeed;
};

// Global variables
extern GyroOffsets gyroOffsets;
extern AccelOffsets accelOffsets;

// Function declarations
void initBalance();
float updatePID(PIDController &pid, float error, float deadBand);
void balanceRobot();
void adjustPIDGainsFromSerial(char input);

// Auto-tuning integration functions
void handleAutoTune();
void startAutoTuneFromSerial();
void stopAutoTuneFromSerial();
void applyAutoTuneFromSerial();

// Global PID controller access
extern PIDController balancePID;

// Function to send angle data via WebSocket
extern void sendAngleData(float angle, float target);

#endif
