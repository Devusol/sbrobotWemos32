#ifndef GYRO_H
#define GYRO_H

#include <Wire.h>
#include <Arduino.h>

// Gyroscope I2C address
#define GYRO_I2C_ADDRESS 0x68

// Gyroscope data structure
struct GyroData {
    float x;
    float y;
    float z;
};

// Accelerometer data structure
struct AccelData {
    float x;
    float y;
    float z;
};

// Offset structures
struct GyroOffsets {
    float x, y, z;
};

struct AccelOffsets {
    float x, y, z;
};

struct Orientation {
    float pitch; // Rotation around X-axis
    float roll;  // Rotation around Y-axis
    float yaw;   // Rotation around Z-axis
};

// Function declarations
void initGyro();
void calibrateGyro(GyroOffsets &offsets);
void calibrateAccel(AccelOffsets &offsets);
GyroData readGyro(const GyroOffsets &offsets);
AccelData readAccel(const AccelOffsets &offsets);
Orientation readOrientation(const GyroData &gyro, const AccelData &accel);
void adjustGyroOffsets(GyroOffsets &offsets, const GyroData &drift, char ijkl);

#endif