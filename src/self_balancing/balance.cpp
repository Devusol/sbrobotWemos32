#include "balance.h"
#include "control/input_controller.h"

// PID controller for balancing
PIDController balancePID = {2.0, 0.0, 0.1, 0.0, 0.0, 0};  // Tune these values

// Complementary filter variables
float currentAngle = 0.0;
unsigned long lastAngleTime = 0;

// Target angle (upright)
const float TARGET_ANGLE = 0.0;

// Initialize balancing
void initBalance() {
    balancePID.lastTime = millis();
    lastAngleTime = millis();
}

// Calculate angle using complementary filter
float calculateAngle(AccelData accel, GyroData gyro) {
    unsigned long currentTime = millis();
    float dt = (currentTime - lastAngleTime) / 1000.0;  // Convert to seconds
    lastAngleTime = currentTime;

    // Calculate angle from accelerometer (pitch angle)
    // Orientation: X down, Y right, Z forward
    // Pitch angle around Y-axis: atan2(Z, X)
    float accelAngle = atan2(accel.z, accel.x) * 180.0 / PI;

    // Integrate gyro rate to get angle change
    float gyroRate = gyro.y;  // Y-axis for pitch rate
    float gyroAngleChange = gyroRate * dt;

    // Complementary filter
    float alpha = 0.98;
    currentAngle = alpha * (currentAngle + gyroAngleChange) + (1 - alpha) * accelAngle;

    return currentAngle;
}

// Update PID controller
float updatePID(PIDController &pid, float error) {
    unsigned long currentTime = millis();
    float dt = (currentTime - pid.lastTime) / 1000.0;
    pid.lastTime = currentTime;

    // Proportional
    float pTerm = pid.kp * error;

    // Integral
    pid.integral += error * dt;
    // Limit integral to prevent windup
    pid.integral = constrain(pid.integral, -100, 100);
    float iTerm = pid.ki * pid.integral;

    // Derivative
    float derivative = (error - pid.previousError) / dt;
    float dTerm = pid.kd * derivative;
    pid.previousError = error;

    // Total output
    float output = pTerm + iTerm + dTerm;

    return output;
}

// Balance the robot
void balanceRobot(float angle) {
    float error = angle - TARGET_ANGLE;  // Positive when tilted forward

    // Update PID
    float pidOutput = updatePID(balancePID, error);

    // Convert PID output to motor speeds
    // Positive pidOutput means tilted forward, increase forward speed
    int baseSpeed = 50;  // Base speed for balancing
    int leftSpeed = baseSpeed + pidOutput;
    int rightSpeed = baseSpeed + pidOutput;

    // Set motor speeds
    setMotorSpeeds(leftSpeed, rightSpeed);
}
