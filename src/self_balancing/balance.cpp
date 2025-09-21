#include "balance.h"
#include "control/input_controller.h"

// PID controller for balancing
PIDController balancePID = {5.0, 0.0, 0.0, 0.0, 0.0, 0, 0};

// Initialize balancing
void initBalance()
{
    balancePID.lastTime = millis();

    // Initialize currentAngle to the initial accelerometer angle
    AccelData initialAccel = readAccel(accelOffsets);
    currentAngle = atan2(-initialAccel.x, initialAccel.z) * 180.0 / PI;
    currentAngle = fmod(currentAngle + 360.0, 360.0);
    lastAngleTime = millis();
}

// Update PID controller
float updatePID(PIDController &pid, float error, float deadBand)
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

    // Reset integral when error is small to prevent buildup
    if (abs(error) < deadBand)
    {
        pid.integral = 0;
        iTerm = 0;
    }

    // Derivative
    float derivative = (error - pid.previousError) / dt;
    // Low-pass filter the derivative to reduce noise amplification
    static float filteredDerivative = 0.0;
    filteredDerivative = 0.9 * filteredDerivative + 0.1 * derivative;
    derivative = filteredDerivative;
    float dTerm = pid.kd * derivative;
    pid.previousError = error;

    // Total output
    float output = pTerm + iTerm + dTerm;

    return output;
}

// Balance the robot
void balanceRobot(float targetAngle, float deadBand)
{

    float angle = calculateAngle();

    // angle = round(angle); // Round to nearest whole degree to reduce noise
    Serial.printf("Kp: %.3f, Ki: %.3f, Kd: %.3f\n", balancePID.kp, balancePID.ki, balancePID.kd);

    // Serial.printf("Accel X: %.2f, Y: %.2f, Z: %.2f\n", accel.x, accel.y, accel.z);

    float error = angle - targetAngle; // Positive when tilted forward
    // Serial.printf("Angle: %.2f, Error: %.2f\n", angle, error);
    // Serial.printf("Angle:%.2f\n, Target:%.2f\n, Error:%.2f\n", angle, targetAngle, error);
    // Serial.print(">Angle:");
    // Serial.println(angle);
    // Serial.print(">Target:");
    // Serial.println(targetAngle);
    // Serial.print(">Error:");
    // Serial.println(error);

    // Apply deadband to reduce noise
    if (abs(error) < deadBand)
    {
        error = 0;
    }

    // Update PID
    float pidOutput = updatePID(balancePID, error, deadBand);

    // Low-pass filter the PID output to reduce jitter
    static float filteredPidOutput = 0.0;
    filteredPidOutput = 0.9 * filteredPidOutput + 0.1 * pidOutput;
    pidOutput = filteredPidOutput;

    // Constrain PID output to prevent excessive speeds
    pidOutput = constrain(pidOutput, -100, 100);

    // Convert PID output to motor speeds
    // Base speed provides steady-state balancing torque
    int leftSpeed = balancePID.baseSpeed + pidOutput;
    int rightSpeed = balancePID.baseSpeed + pidOutput;

    // Stop motors if angle is too extreme (fallen over)
    if (angle > 160.0)
    {
        // Serial.println("Stop fell backward");
        stopMovement();
        // delay(1000); // Small delay to ensure stop command is processed
        leftSpeed = 0;
        rightSpeed = 0;
    }
    if (angle < 20.0)
    {
        // Serial.println("Stop fell forward");
        stopMovement();
        // delay(1000); // Small delay to ensure stop command is processed
        leftSpeed = 0;
        rightSpeed = 0;
    }
    // Set motor speeds
    setMotorSpeeds(leftSpeed, rightSpeed);
}

void adjustPIDGainsFromSerial(char qawsedrf)
{
    Serial.println("PID gains: Kp=" + String(balancePID.kp, 3) + ", Ki=" + String(balancePID.ki, 3) + ", Kd=" + String(balancePID.kd, 3) + ", BaseSpeed=" + String(balancePID.baseSpeed));

    float kp = balancePID.kp, ki = balancePID.ki, kd = balancePID.kd, baseSpeed = balancePID.baseSpeed;
    if (qawsedrf == 'w')
    {
        kp += 0.1; // Increase Kp
    }
    else if (qawsedrf == 's')
    {
        kp -= 0.1; // Decrease Kp
        if (kp < 0)
            kp = 0; // Prevent negative Kp
    }
    else if (qawsedrf == 'e')
    {
        ki += 0.001; // Increase Ki
    }
    else if (qawsedrf == 'd')
    {
        ki -= 0.001; // Decrease Ki
        if (ki < 0)
            ki = 0; // Prevent negative Ki
    }
    else if (qawsedrf == 'r')
    {
        kd += 0.001; // Increase Kd
    }
    else if (qawsedrf == 'f')
    {
        kd -= 0.001; // Decrease Kd
        if (kd < 0)
            kd = 0; // Prevent negative Kd
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