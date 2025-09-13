#include "gyro.h"

// Initialize the gyroscope
void initGyro() {
    Wire.begin();
    Wire.beginTransmission(GYRO_I2C_ADDRESS);
    Wire.write(0x6B);  // Power management register
    Wire.write(0);      // Wake up the gyro
    Wire.endTransmission();
    Serial.println("Gyroscope initialized");
}

// Calibrate gyroscope by averaging readings when stationary
void calibrateGyro(GyroOffsets &offsets) {
    const int numSamples = 100;
    float sumX = 0, sumY = 0, sumZ = 0;

    Serial.println("Calibrating gyroscope... Keep the device stationary.");

    for (int i = 0; i < numSamples; i++) {
        Wire.beginTransmission(GYRO_I2C_ADDRESS);
        Wire.write(0x43);
        Wire.endTransmission();
        Wire.requestFrom(GYRO_I2C_ADDRESS, 6);
        if (Wire.available() == 6) {
            int16_t rawX = Wire.read() << 8 | Wire.read();
            int16_t rawY = Wire.read() << 8 | Wire.read();
            int16_t rawZ = Wire.read() << 8 | Wire.read();
            sumX += rawX;
            sumY += rawY;
            sumZ += rawZ;
        }
        delay(10);
    }

    offsets.x = sumX / numSamples;
    offsets.y = sumY / numSamples;
    offsets.z = sumZ / numSamples;

    Serial.printf("Gyro offsets: X=%.2f, Y=%.2f, Z=%.2f\n", offsets.x, offsets.y, offsets.z);
}

// Calibrate accelerometer by averaging readings when flat
void calibrateAccel(AccelOffsets &offsets) {
    const int numSamples = 100;
    float sumX = 0, sumY = 0, sumZ = 0;

    Serial.println("Calibrating accelerometer... Keep the device flat and stationary.");

    for (int i = 0; i < numSamples; i++) {
        Wire.beginTransmission(GYRO_I2C_ADDRESS);
        Wire.write(0x3B);
        Wire.endTransmission();
        Wire.requestFrom(GYRO_I2C_ADDRESS, 6);
        if (Wire.available() == 6) {
            int16_t rawX = Wire.read() << 8 | Wire.read();
            int16_t rawY = Wire.read() << 8 | Wire.read();
            int16_t rawZ = Wire.read() << 8 | Wire.read();
            sumX += rawX;
            sumY += rawY;
            sumZ += rawZ;
        }
        delay(10);
    }

    // For accel, when flat (X down, Z forward), X should be 1g, Z should be 0
    offsets.x = sumX / numSamples - 16384;  // Subtract 1g from X
    offsets.y = sumY / numSamples;
    offsets.z = sumZ / numSamples;

    Serial.printf("Accel offsets: X=%.2f, Y=%.2f, Z=%.2f\n", offsets.x, offsets.y, offsets.z);
}

// Read gyroscope data with calibration
GyroData readGyro(const GyroOffsets &offsets) {
    GyroData data;
    Wire.beginTransmission(GYRO_I2C_ADDRESS);
    Wire.write(0x43);  // Starting register for gyro data
    Wire.endTransmission();
    Wire.requestFrom(GYRO_I2C_ADDRESS, 6);
    if (Wire.available() == 6) {
        int16_t rawX = (Wire.read() << 8 | Wire.read()) - offsets.x;
        int16_t rawY = (Wire.read() << 8 | Wire.read()) - offsets.y;
        int16_t rawZ = (Wire.read() << 8 | Wire.read()) - offsets.z;
        data.x = rawX / 131.0;  // Convert to degrees/sec (for 250 deg/s range)
        data.y = rawY / 131.0;
        data.z = rawZ / 131.0;
    }
    return data;
}

// Read accelerometer data with calibration
AccelData readAccel(const AccelOffsets &offsets) {
    AccelData data;
    Wire.beginTransmission(GYRO_I2C_ADDRESS);
    Wire.write(0x3B);  // Starting register for accel data
    Wire.endTransmission();
    Wire.requestFrom(GYRO_I2C_ADDRESS, 6);
    if (Wire.available() == 6) {
        int16_t rawX = (Wire.read() << 8 | Wire.read()) - offsets.x;
        int16_t rawY = (Wire.read() << 8 | Wire.read()) - offsets.y;
        int16_t rawZ = (Wire.read() << 8 | Wire.read()) - offsets.z;
        data.x = rawX / 16384.0;  // Convert to g (for 2g range)
        data.y = rawY / 16384.0;
        data.z = rawZ / 16384.0;
    }
    return data;
}
