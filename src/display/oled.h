#ifndef OLED_H
#define OLED_H

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "../gyro/gyro.h"

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED reset pin (-1 if sharing Arduino reset pin)
#define OLED_RESET -1

class OLED_Display {
private:
    Adafruit_SSD1306 display;
    
public:
    OLED_Display();
    bool begin();
    void clearDisplay();
    void setBrightness(uint8_t brightness);  // 0-255, where 255 is maximum
    void displayWiFiStatus(bool isConnected, const String& ipAddress = "", bool isAPMode = false);
    void displayText(const String& text, int x = 0, int y = 0, int size = 1);
    void updateDisplay();
    void displayGyroData(const GyroData& data);
    void displayAccelData(const AccelData& data);
    void displaySensorData(const GyroData& gyro, const AccelData& accel);
};

#endif