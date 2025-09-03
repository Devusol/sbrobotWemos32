#include "input_controller.h"

// Onboard LED pin for testing (GPIO 2 on most ESP32 boards)
#define LED_PIN 2

// LEDC PWM configuration
const int LEDC_CHANNEL = 0;
const int LEDC_FREQ = 5000;
const int LEDC_RESOLUTION = 8; // 0-255

// Current speed (0-100)
static int currentSpeed = 0;

// Initialize the controller
void initController() {
    // Setup onboard LED for PWM control
    pinMode(LED_PIN, OUTPUT);

    // Initialize LEDC for PWM
    ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(LED_PIN, LEDC_CHANNEL);

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
    ledcWrite(LEDC_CHANNEL, pwmValue);

    Serial.print("Speed set to: ");
    Serial.print(currentSpeed);
    Serial.print("% (PWM: ");
    Serial.print(pwmValue);
    Serial.println(")");
}

// Movement commands (currently just logging for testing)
void moveForward() {
    Serial.println("Moving forward at speed: " + String(currentSpeed) + "%");
    // Add your forward movement logic here
    // Example: setMotorSpeed(MOTOR_LEFT, currentSpeed);
    //         setMotorSpeed(MOTOR_RIGHT, currentSpeed);
}

void moveBackward() {
    Serial.println("Moving backward at speed: " + String(currentSpeed) + "%");
    // Add your backward movement logic here
    // Example: setMotorSpeed(MOTOR_LEFT, -currentSpeed);
    //         setMotorSpeed(MOTOR_RIGHT, -currentSpeed);
}

void turnLeft() {
    Serial.println("Turning left at speed: " + String(currentSpeed) + "%");
    // Add your left turn logic here
    // Example: setMotorSpeed(MOTOR_LEFT, -currentSpeed);
    //         setMotorSpeed(MOTOR_RIGHT, currentSpeed);
}

void turnRight() {
    Serial.println("Turning right at speed: " + String(currentSpeed) + "%");
    // Add your right turn logic here
    // Example: setMotorSpeed(MOTOR_LEFT, currentSpeed);
    //         setMotorSpeed(MOTOR_RIGHT, -currentSpeed);
}

void stopMovement() {
    Serial.println("Stopping movement");
    // Add your stop logic here
    // Example: setMotorSpeed(MOTOR_LEFT, 0);
    //         setMotorSpeed(MOTOR_RIGHT, 0);
}
