#include "gyro.h"
GyroOffsets gyroOffsets;
AccelOffsets accelOffsets;

// Global variables for complementary filter
float currentAngle = 0.0;
unsigned long lastAngleTime = 0;

// Initialize the gyroscope
void initGyro()
{
    Wire.begin();
    Wire.beginTransmission(GYRO_I2C_ADDRESS);
    Wire.write(0x6B); // Power management register
    Wire.write(0);    // Wake up the gyro
    Wire.endTransmission();

    // Set accelerometer range to 2g
    Wire.beginTransmission(GYRO_I2C_ADDRESS);
    Wire.write(0x1C); // Accel config register
    Wire.write(0);    // 2g range
    Wire.endTransmission();

    Serial.println("Gyroscope initialized");
}

void calibrateAll()
{

    calibrateGyro(gyroOffsets);
    calibrateAccel(accelOffsets);
}

// Calibrate gyroscope by averaging readings when stationary
void calibrateGyro(GyroOffsets &offsets)
{
    const int numSamples = 100;
    float sumX = 0, sumY = 0, sumZ = 0;

    Serial.println("Calibrating gyroscope... Keep the device stationary.");

    for (int i = 0; i < numSamples; i++)
    {
        Wire.beginTransmission(GYRO_I2C_ADDRESS);
        Wire.write(0x43);
        Wire.endTransmission();
        Wire.requestFrom(GYRO_I2C_ADDRESS, 6);
        if (Wire.available() == 6)
        {
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
void calibrateAccel(AccelOffsets &offsets)
{
    Serial.println("Setting accelerometer offsets to 0.");
    offsets.x = 0;
    offsets.y = 0;
    offsets.z = 0;
    Serial.printf("Accel offsets: X=%.2f, Y=%.2f, Z=%.2f\n", offsets.x, offsets.y, offsets.z);
}

// Read gyroscope data with calibration
GyroData readGyro(const GyroOffsets &offsets)
{
    GyroData data;
    Wire.beginTransmission(GYRO_I2C_ADDRESS);
    Wire.write(0x43); // Starting register for gyro data
    Wire.endTransmission();
    Wire.requestFrom(GYRO_I2C_ADDRESS, 6);
    if (Wire.available() == 6)
    {
        int16_t rawX = (Wire.read() << 8 | Wire.read()) - offsets.x;
        int16_t rawY = (Wire.read() << 8 | Wire.read()) - offsets.y;
        int16_t rawZ = (Wire.read() << 8 | Wire.read()) - offsets.z;
        data.x = rawX / 131.0; // Convert to degrees/sec (for 250 deg/s range)
        data.y = rawY / 131.0;
        data.z = rawZ / 131.0;
    }
    return data;
}

// Read accelerometer data with calibration
AccelData readAccel(const AccelOffsets &offsets)
{
    AccelData data;
    Wire.beginTransmission(GYRO_I2C_ADDRESS);
    Wire.write(0x3B); // Starting register for accel data
    Wire.endTransmission();
    Wire.requestFrom(GYRO_I2C_ADDRESS, 6);
    if (Wire.available() == 6)
    {
        int16_t rawX = (Wire.read() << 8 | Wire.read()) - offsets.x;
        int16_t rawY = (Wire.read() << 8 | Wire.read()) - offsets.y;
        int16_t rawZ = (Wire.read() << 8 | Wire.read()) - offsets.z;
        data.x = rawX / 16384.0; // Convert to g (for 2g range)
        data.y = rawY / 16384.0;
        data.z = rawZ / 16384.0;
    }
    return data;
}

Orientation readOrientation(const GyroData &gyro, const AccelData &accel)
{
    Orientation ori;
    // Simple complementary filter for orientation estimation
    static float pitch = 0.0, roll = 0.0, yaw = 0.0;
    static unsigned long lastTime = millis();
    unsigned long currentTime = millis();
    float dt = (currentTime - lastTime) / 1000.0; // Convert to seconds
    lastTime = currentTime;

    // Integrate gyro data (adjusted for X-down, Y-right, Z-forward orientation)
    // Pitch is rotation around Y-axis, roll around X-axis
    pitch += -gyro.y * dt;
    roll += gyro.x * dt;
    yaw += gyro.z * dt;

    // Calculate accelerometer angles for this orientation
    // Gravity along +X, so up is -X
    // Pitch (around Y): atan2(Z, X)
    // Roll (around X): atan2(Y, X) - but since gravity along X, roll accel measurement is limited
    float accelPitch = atan2(-accel.x, accel.z) * 180.0 / PI;
    float accelRoll = atan2(accel.y, accel.z) * 180.0 / PI;

    // Complementary filter to combine gyro and accel
    const float alpha = 0.95; // Reduced for faster response
    pitch = alpha * pitch + (1 - alpha) * accelPitch;
    roll = alpha * roll + (1 - alpha) * accelRoll;

    ori.pitch = pitch;
    ori.roll = roll;
    ori.yaw = yaw; // Yaw is only from gyro integration

    return ori;
}

void adjustGyroOffsets(GyroOffsets &offsets, const GyroData &drift, char ijkl)
{
    if (ijkl == 'i')
    {
        offsets.x += 1.0; // Adjust pitch offset
    }
    else if (ijkl == 'k')
    {
        offsets.x -= 1.0;
    }
    else if (ijkl == 'j')
    {
        offsets.y += 1.0; // Adjust roll offset
    }
    else if (ijkl == 'l')
    {
        offsets.y -= 1.0;
    }
    else
    {
        Serial.println("Invalid input for gyro offset adjustment. Use 'i', 'j', 'k', or 'l'.");
        return;
    }
    // Simple adjustment by averaging current offsets with drift
    offsets.x = (offsets.x + drift.x) / 2.0;
    offsets.y = (offsets.y + drift.y) / 2.0;
    offsets.z = (offsets.z + drift.z) / 2.0;

    Serial.printf("Adjusted Gyro Offsets: X=%.2f, Y=%.2f, Z=%.2f\n", offsets.x, offsets.y, offsets.z);
}

// Calculate angle using complementary filter
// float calculateAngle(AccelData accel, GyroData gyro)
float calculateAngle()
{
    AccelData accel = readAccel(accelOffsets);
    GyroData gyro = readGyro(gyroOffsets);
    unsigned long currentTime = millis();
    float dt = (currentTime - lastAngleTime) / 1000.0; // Convert to seconds
    lastAngleTime = currentTime;

    // Calculate angle from accelerometer (pitch angle)
    // Orientation: X down, Y right, Z forward
    // Pitch angle around Y-axis: atan2(Z, X)
    float accelAngle = atan2(-accel.x, accel.z) * 180.0 / PI;

    // Integrate gyro rate to get angle change
    float gyroRate = -gyro.y; // Y-axis for pitch rate
    float gyroAngleChange = gyroRate * dt;

    // Complementary filter
    float alpha = 0.8; // Reduced alpha for faster response to accelerometer
    currentAngle = alpha * (currentAngle + gyroAngleChange) + (1 - alpha) * accelAngle;

    // Normalize angle to 0-360 degrees
    currentAngle = fmod(currentAngle + 360.0, 360.0);
    
    return currentAngle;
}