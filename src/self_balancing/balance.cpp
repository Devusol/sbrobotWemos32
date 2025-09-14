#in// PID controller for balancing
PIDController balancePID =     Serial.printf("Accel X: %.2f, Y: %.2f, Z: %.2f\n", accel.x, accel.y, accel.z);

    float error = angle - targetAngle; // Positive when tilted forward

    // Apply deadband to reduce noise
    if (abs(error) < DEAD_BAND) {
        error = 0;
    }

    // Update PID, 5.0, 5.0, 0.0, 0.0, 0, 0};

// Target angle (upright)
const float TARGET_ANGLE = -90.0;

// Deadband for error to reduce noise
const float DEAD_BAND = 1.0; // degrees "balance.h"
#include "control/input_controller.h"

// PID controller for balancing
PIDController balancePID = {0.0, 0.0, 0.0, 0.0, 0.0, 0, 0};

// Target angle (upright)
const float TARGET_ANGLE = -90.0;

// Initialize balancing
void initBalance()
{
    balancePID.lastTime = millis();

    // Initialize currentAngle to the initial accelerometer angle
    AccelData initialAccel = readAccel(accelOffsets);
    currentAngle = atan2(initialAccel.z, initialAccel.x) * 180.0 / PI;
    lastAngleTime = millis();
}

// Update PID controller
float updatePID(PIDController &pid, float error)
{
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
void balanceRobot(float targetAngle)
{

    GyroData gyro = readGyro(gyroOffsets);
    AccelData accel = readAccel(accelOffsets);
    float angle = calculateAngle(accel, gyro);
    // Serial.printf("Current Angle: %.2f, Target Angle: %.2f\n", angle, targetAngle);

    // Serial.printf("Accel X: %.2f, Y: %.2f, Z: %.2f\n", accel.x, accel.y, accel.z);

    float error = angle - targetAngle; // Positive when tilted forward

    // Update PID
    float pidOutput = updatePID(balancePID, error);

    // Convert PID output to motor speeds
    // Base speed provides steady-state balancing torque
    int leftSpeed = balancePID.baseSpeed + pidOutput;
    int rightSpeed = balancePID.baseSpeed + pidOutput;

    // Set motor speeds
    setMotorSpeeds(leftSpeed, rightSpeed);
}

void adjustPIDGains(char qawsedrf)
{
    // Serial.println("PID gains: Kp=" + String(balancePID.kp, 3) + ", Ki=" + String(balancePID.ki, 3) + ", Kd=" + String(balancePID.kd, 3) + ", BaseSpeed=" + String(balancePID.baseSpeed));

    float kp = balancePID.kp, ki = balancePID.ki, kd = balancePID.kd, baseSpeed = balancePID.baseSpeed;
    if (qawsedrf == 'w')
    {
        kp += 0.1; // Increase Kp
    }
    else if (qawsedrf == 's')
    {
        kp -= 0.1; // Decrease Kp
    }
    else if (qawsedrf == 'e')
    {
        ki += 0.01; // Increase Ki
    }
    else if (qawsedrf == 'd')
    {
        ki -= 0.01; // Decrease Ki
    }
    else if (qawsedrf == 'r')
    {
        kd += 0.01; // Increase Kd
    }
    else if (qawsedrf == 'f')
    {
        kd -= 0.01; // Decrease Kd
    }
    else if (qawsedrf == 'q')
    {
        baseSpeed += 5; // Increase base speed
    }
    else if (qawsedrf == 'a')
    {
        baseSpeed -= 5; // Decrease base speed
        if (baseSpeed < 0)
            baseSpeed = 0; // Prevent negative speed
    }
    else
    {
        Serial.println("Invalid input for PID gain adjustment. Use 'w', 's', 'e', 'd', 'r', or 'f'.");
        return;
    }

    balancePID.kp = kp;
    balancePID.ki = ki;
    balancePID.kd = kd;
    balancePID.baseSpeed = baseSpeed;

    Serial.println("Adjusted PID gains: Kp=" + String(balancePID.kp, 3) + ", Ki=" + String(balancePID.ki, 3) + ", Kd=" + String(balancePID.kd, 3) + ", BaseSpeed=" + String(balancePID.baseSpeed));
}