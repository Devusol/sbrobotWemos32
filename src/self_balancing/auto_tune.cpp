#include "auto_tune.h"
#include "balance.h"
#include <Arduino.h>

// Global variables
AutoTuneState autoTuneState = AT_IDLE;
AutoTuneParams autoTuneParams = {1.0, 0.5, 20.0, 2.0, 2000, 10000};
AutoTuneResults autoTuneResults = {0.0, 0.0, 0.0, 0.0, 0.0, false};

// Internal variables for auto-tuning
static float originalKp, originalKi, originalKd;
static unsigned long tuneStartTime;
static unsigned long lastOscillationTime;
static float maxAngle, minAngle;
static int oscillationCount;
static bool increasingKp;
static float currentTestKp;

// Initialize auto-tuning
void initAutoTune() {
    autoTuneState = AT_IDLE;
    resetAutoTuneResults();
}

// Start auto-tuning process
void startAutoTune() {
    if (autoTuneState != AT_IDLE) return;

    // Save original PID values
    originalKp = balancePID.kp;
    originalKi = balancePID.ki;
    originalKd = balancePID.kd;

    // Initialize tuning variables
    currentTestKp = autoTuneParams.kpStart;
    increasingKp = true;
    maxAngle = -1000.0;
    minAngle = 1000.0;
    oscillationCount = 0;
    lastOscillationTime = 0;

    // Set initial PID values for relay test (Ki=0, Kd=0)
    balancePID.kp = currentTestKp;
    balancePID.ki = 0.0;
    balancePID.kd = 0.0;
    balancePID.integral = 0.0;
    balancePID.previousError = 0.0;

    tuneStartTime = millis();
    autoTuneState = AT_RELAY_TEST;

    Serial.println("Auto-tuning started. Testing Kp values...");
}

// Stop auto-tuning and restore original values
void stopAutoTune() {
    if (autoTuneState == AT_IDLE) return;

    // Restore original PID values
    balancePID.kp = originalKp;
    balancePID.ki = originalKi;
    balancePID.kd = originalKd;
    balancePID.integral = 0.0;
    balancePID.previousError = 0.0;

    autoTuneState = AT_IDLE;
    Serial.println("Auto-tuning stopped. Original PID values restored.");
}

// Update auto-tuning process
void updateAutoTune() {
    if (autoTuneState != AT_RELAY_TEST) return;

    unsigned long currentTime = millis();
    float currentAngle = calculateAngle();

    // Update min/max angles
    if (currentAngle > maxAngle) maxAngle = currentAngle;
    if (currentAngle < minAngle) minAngle = currentAngle;

    // Check for oscillations
    float amplitude = maxAngle - minAngle;

    // If we have sufficient oscillation amplitude, calculate results
    if (amplitude >= autoTuneParams.oscillationThreshold && oscillationCount >= 3) {
        // Calculate ultimate gain (Ku) and period (Tu)
        autoTuneResults.ultimateGain = currentTestKp;
        autoTuneResults.oscillationPeriod = (currentTime - lastOscillationTime) / 1000.0;

        // Calculate PID gains using Ziegler-Nichols method
        autoTuneResults.kp = 0.6 * autoTuneResults.ultimateGain;
        autoTuneResults.ki = 1.2 * autoTuneResults.ultimateGain / autoTuneResults.oscillationPeriod;
        autoTuneResults.kd = 0.075 * autoTuneResults.ultimateGain * autoTuneResults.oscillationPeriod;
        autoTuneResults.valid = true;

        autoTuneState = AT_COMPLETE;

        Serial.printf("Auto-tuning complete!\n");
        Serial.printf("Ultimate Gain (Ku): %.3f\n", autoTuneResults.ultimateGain);
        Serial.printf("Oscillation Period (Tu): %.3f s\n", autoTuneResults.oscillationPeriod);
        Serial.printf("Calculated PID: Kp=%.3f, Ki=%.3f, Kd=%.3f\n",
                     autoTuneResults.kp, autoTuneResults.ki, autoTuneResults.kd);

        return;
    }

    // Check if we need to increase Kp
    if (currentTime - tuneStartTime > autoTuneParams.testDuration) {
        if (increasingKp) {
            currentTestKp += autoTuneParams.kpIncrement;
            if (currentTestKp > autoTuneParams.maxKp) {
                // Failed to find oscillations
                autoTuneState = AT_FAILED;
                Serial.println("Auto-tuning failed: Could not find oscillations within Kp range");
                return;
            }
        } else {
            // If decreasing, we might have overshot
            currentTestKp -= autoTuneParams.kpIncrement * 0.5;
            if (currentTestKp < autoTuneParams.kpStart) {
                autoTuneState = AT_FAILED;
                Serial.println("Auto-tuning failed: Could not stabilize oscillations");
                return;
            }
        }

        // Reset for new test
        balancePID.kp = currentTestKp;
        maxAngle = -1000.0;
        minAngle = 1000.0;
        oscillationCount = 0;
        tuneStartTime = currentTime;

        Serial.printf("Testing Kp: %.3f\n", currentTestKp);
    }

    // Detect zero crossings to count oscillations
    static float prevAngle = currentAngle;
    static bool wasPositive = (prevAngle > 0);

    bool isPositive = (currentAngle > 0);
    if (wasPositive != isPositive && abs(currentAngle - prevAngle) > 0.5) {
        oscillationCount++;
        lastOscillationTime = currentTime;
        wasPositive = isPositive;

        // If we have oscillations, switch to decreasing Kp to find ultimate gain
        if (oscillationCount >= 2 && increasingKp) {
            increasingKp = false;
            Serial.println("Oscillations detected, fine-tuning Kp...");
        }
    }

    prevAngle = currentAngle;
}

// Check if auto-tuning is currently active
bool isAutoTuneActive() {
    return autoTuneState != AT_IDLE;
}

// Apply the calculated PID gains
void applyAutoTuneResults() {
    if (!autoTuneResults.valid) {
        Serial.println("No valid auto-tune results to apply");
        return;
    }

    balancePID.kp = autoTuneResults.kp;
    balancePID.ki = autoTuneResults.ki;
    balancePID.kd = autoTuneResults.kd;
    balancePID.integral = 0.0;
    balancePID.previousError = 0.0;

    Serial.printf("Applied auto-tuned PID gains: Kp=%.3f, Ki=%.3f, Kd=%.3f\n",
                 balancePID.kp, balancePID.ki, balancePID.kd);
}

// Reset auto-tuning results
void resetAutoTuneResults() {
    autoTuneResults.ultimateGain = 0.0;
    autoTuneResults.oscillationPeriod = 0.0;
    autoTuneResults.kp = 0.0;
    autoTuneResults.ki = 0.0;
    autoTuneResults.kd = 0.0;
    autoTuneResults.valid = false;
}
