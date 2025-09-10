#include "input_controller.h"

// Onboard LED pin for testing (GPIO 2 on most ESP32 boards)
#define LED_PIN 2


#define MOTOR_LEFT_REV 12
#define MOTOR_LEFT_PWM 32
#define MOTOR_LEFT_FWD 25
#define MOTOR_RIGHT_PWM 27

#define MOTOR_RIGHT_REV 33
#define MOTOR_RIGHT_FWD 14


// LEDC PWM configuration
const int LEDC_CHANNEL_LED = 0;
const int LEDC_CHANNEL_LEFT = 1;
const int LEDC_CHANNEL_RIGHT = 2;
const int LEDC_FREQ = 5000;
const int LEDC_RESOLUTION = 8; // 0-255

// Current speed (0-100)
static int currentSpeed = 0;

// Initialize the controller
void initController() {
    // Setup onboard LED for PWM control
    pinMode(LED_PIN, OUTPUT);

    // Initialize LEDC for PWM
    ledcSetup(LEDC_CHANNEL_LED, LEDC_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(LED_PIN, LEDC_CHANNEL_LED);

    // Setup motor pins
    pinMode(MOTOR_LEFT_FWD, OUTPUT);
    pinMode(MOTOR_LEFT_REV, OUTPUT);
    pinMode(MOTOR_RIGHT_FWD, OUTPUT);
    pinMode(MOTOR_RIGHT_REV, OUTPUT);

    ledcSetup(LEDC_CHANNEL_LEFT, LEDC_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(MOTOR_LEFT_PWM, LEDC_CHANNEL_LEFT);

    ledcSetup(LEDC_CHANNEL_RIGHT, LEDC_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(MOTOR_RIGHT_PWM, LEDC_CHANNEL_RIGHT);

    Serial.println("Controller initialized - LED PWM ready for testing");
}

// Handle robot commands (called from HTTP POST handler)
void handleRobotCommand(String command, String value) {
    Serial.print("Handling robot command: ");
    Serial.print(command);
    if (value.length() > 0) {
        Serial.print(" with value: ");
        Serial.println(value);
    } else {
        Serial.println();
    }

    if (command == "speed") {
        int speedValue = value.toInt();
        setSpeed(speedValue);
        Serial.println("Speed command processed successfully");
    } else if (command == "forward") {
        moveForward();
        Serial.println("Forward command processed successfully");
    } else if (command == "backward") {
        moveBackward();
        Serial.println("Backward command processed successfully");
    } else if (command == "left") {
        turnLeft();
        Serial.println("Left turn command processed successfully");
    } else if (command == "right") {
        turnRight();
        Serial.println("Right turn command processed successfully");
    } else if (command == "stop") {
        stopMovement();
        Serial.println("Stop command processed successfully");
    } else {
        Serial.println("Unknown command: " + command);
    }
}

// Set speed (0-100) - controls LED brightness for testing
void setSpeed(int speed) {
    currentSpeed = constrain(speed, 0, 100);
    int pwmValue = map(currentSpeed, 0, 100, 0, 255);
    ledcWrite(LEDC_CHANNEL_LED, pwmValue);

    Serial.print("Speed set to: ");
    Serial.print(currentSpeed);
    Serial.print("% (PWM: ");
    Serial.print(pwmValue);
    Serial.println(")");
}

void moveForward() {
    Serial.println("Moving forward at speed: " + String(currentSpeed) + "%");
    int pwmValue = map(currentSpeed, 0, 100, 0, 255);

    // Set direction: forward
    digitalWrite(MOTOR_LEFT_FWD, HIGH);
    digitalWrite(MOTOR_LEFT_REV, LOW);
    digitalWrite(MOTOR_RIGHT_FWD, HIGH);
    digitalWrite(MOTOR_RIGHT_REV, LOW);

    // Set speed
    ledcWrite(LEDC_CHANNEL_LEFT, pwmValue);
    ledcWrite(LEDC_CHANNEL_RIGHT, pwmValue);
}

void moveBackward() {
    Serial.println("Moving backward at speed: " + String(currentSpeed) + "%");
    int pwmValue = map(currentSpeed, 0, 100, 0, 255);

    // Set direction: backward
    digitalWrite(MOTOR_LEFT_FWD, LOW);
    digitalWrite(MOTOR_LEFT_REV, HIGH);
    digitalWrite(MOTOR_RIGHT_FWD, LOW);
    digitalWrite(MOTOR_RIGHT_REV, HIGH);

    // Set speed
    ledcWrite(LEDC_CHANNEL_LEFT, pwmValue);
    ledcWrite(LEDC_CHANNEL_RIGHT, pwmValue);
}

void turnLeft() {
    Serial.println("Turning left at speed: " + String(currentSpeed) + "%");
    int pwmValue = map(currentSpeed, 0, 100, 0, 255);

    // Left motor backward, right motor forward
    digitalWrite(MOTOR_LEFT_FWD, LOW);
    digitalWrite(MOTOR_LEFT_REV, HIGH);
    digitalWrite(MOTOR_RIGHT_FWD, HIGH);
    digitalWrite(MOTOR_RIGHT_REV, LOW);

    // Set speed
    ledcWrite(LEDC_CHANNEL_LEFT, pwmValue);
    ledcWrite(LEDC_CHANNEL_RIGHT, pwmValue);
}

void turnRight() {
    Serial.println("Turning right at speed: " + String(currentSpeed) + "%");
    int pwmValue = map(currentSpeed, 0, 100, 0, 255);

    // Left motor forward, right motor backward
    digitalWrite(MOTOR_LEFT_FWD, HIGH);
    digitalWrite(MOTOR_LEFT_REV, LOW);
    digitalWrite(MOTOR_RIGHT_FWD, LOW);
    digitalWrite(MOTOR_RIGHT_REV, HIGH);

    // Set speed
    ledcWrite(LEDC_CHANNEL_LEFT, pwmValue);
    ledcWrite(LEDC_CHANNEL_RIGHT, pwmValue);
}

void stopMovement() {
    Serial.println("Stopping movement");

    // Stop motors
    digitalWrite(MOTOR_LEFT_FWD, LOW);
    digitalWrite(MOTOR_LEFT_REV, LOW);
    digitalWrite(MOTOR_RIGHT_FWD, LOW);
    digitalWrite(MOTOR_RIGHT_REV, LOW);

    ledcWrite(LEDC_CHANNEL_LEFT, 0);
    ledcWrite(LEDC_CHANNEL_RIGHT, 0);
}

