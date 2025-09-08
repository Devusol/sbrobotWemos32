#include "oled.h"
#include <Wire.h>
#include <WiFi.h>

OLED_Display::OLED_Display() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
}

bool OLED_Display::begin() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        return false;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    
    // Set default brightness to maximum
    setBrightness(255);
    
    return true;
}

void OLED_Display::setBrightness(uint8_t brightness) {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(brightness);
}

void OLED_Display::clearDisplay() {
    display.clearDisplay();
}

void OLED_Display::displayWiFiStatus(bool isConnected, const String& ipAddress, bool isAPMode) {
    display.clearDisplay();
    display.setCursor(0, 0);
    
    if (isAPMode) {
        display.setTextSize(2);
        display.println("AP MODE");
        display.setTextSize(1);
        display.println("");
        display.println("SSID: Robot_AP");
        display.println("Pass: robot_password");
        display.println("");
        display.print("IP: ");
        display.println(WiFi.softAPIP().toString());
    } else if (isConnected) {
        display.setTextSize(2);
        display.println("WiFi");
        display.println("CONNECTED");
        display.setTextSize(1);
        display.println("");
        display.print("IP: ");
        display.println(ipAddress);
        display.print("Signal: ");
        display.print(WiFi.RSSI());
        display.println(" dBm");
    } else {
        display.setTextSize(2);
        display.println("WiFi");
        display.println("DISCONNECTED");
        display.setTextSize(1);
        display.println("");
        display.println("Attempting to connect...");
        display.println("Check credentials");
    }
    
    display.display();
}

void OLED_Display::displayText(const String& text, int x, int y, int size) {
    display.setCursor(x, y);
    display.setTextSize(size);
    display.println(text);
}

void OLED_Display::updateDisplay() {
    display.display();
}