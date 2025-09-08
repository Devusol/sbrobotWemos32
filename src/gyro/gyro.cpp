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

// Read gyroscope data
GyroData readGyro() {
    GyroData data;
    Wire.beginTransmission(GYRO_I2C_ADDRESS);
    Wire.write(0x43);  // Starting register for gyro data
    Wire.endTransmission();
    Wire.requestFrom(GYRO_I2C_ADDRESS, 6);
    if (Wire.available() == 6) {
        int16_t rawX = Wire.read() << 8 | Wire.read();
        int16_t rawY = Wire.read() << 8 | Wire.read();
        int16_t rawZ = Wire.read() << 8 | Wire.read();
        data.x = rawX / 131.0;  // Convert to degrees/sec (for 250 deg/s range)
        data.y = rawY / 131.0;
        data.z = rawZ / 131.0;
    }
    return data;
}

// Read accelerometer data
AccelData readAccel() {
    AccelData data;
    Wire.beginTransmission(GYRO_I2C_ADDRESS);
    Wire.write(0x3B);  // Starting register for accel data
    Wire.endTransmission();
    Wire.requestFrom(GYRO_I2C_ADDRESS, 6);
    if (Wire.available() == 6) {
        int16_t rawX = Wire.read() << 8 | Wire.read();
        int16_t rawY = Wire.read() << 8 | Wire.read();
        int16_t rawZ = Wire.read() << 8 | Wire.read();
        data.x = rawX / 16384.0;  // Convert to g (for 2g range)
        data.y = rawY / 16384.0;
        data.z = rawZ / 16384.0;
    }
    return data;
}
