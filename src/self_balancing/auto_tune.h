#ifndef AUTO_TUNE_H
#define AUTO_TUNE_H

#include <Arduino.h>

// Auto-tuning states
enum AutoTuneState {
    AT_IDLE,
    AT_RELAY_TEST,
    AT_CALCULATING,
    AT_COMPLETE,
    AT_FAILED
};

// Auto-tuning parameters structure
struct AutoTuneParams {
    float kpStart;           // Starting Kp value
    float kpIncrement;       // Kp increment per step
    float maxKp;            // Maximum Kp to test
    float oscillationThreshold; // Minimum oscillation amplitude to detect
    unsigned long settleTime;    // Time to wait for settling (ms)
    unsigned long testDuration;  // Duration of each test (ms)
};

// Auto-tuning results structure
struct AutoTuneResults {
    float ultimateGain;     // Ku - ultimate gain
    float oscillationPeriod; // Tu - oscillation period in seconds
    float kp;              // Calculated Kp
    float ki;              // Calculated Ki
    float kd;              // Calculated Kd
    bool valid;            // Whether results are valid
};

// Global auto-tuning state
extern AutoTuneState autoTuneState;
extern AutoTuneParams autoTuneParams;
extern AutoTuneResults autoTuneResults;

// Function declarations
void initAutoTune();
void startAutoTune();
void stopAutoTune();
void updateAutoTune();
bool isAutoTuneActive();
void applyAutoTuneResults();
void resetAutoTuneResults();

#endif