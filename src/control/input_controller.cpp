#include "input_controller.h"
#include <WebSocketsServer.h>

// WebSocket server pointer
WebSocketsServer* webSocketPtr = nullptr;

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

// Set WebSocket server pointer
void setWebSocketServer(WebSocketsServer* ws) {
    webSocketPtr = ws;
}

// Broadcast status to all WebSocket clients
void broadcastStatus(const char* status) {
    if (webSocketPtr) {
        String message = "{\"status\":\"";
        message += status;
        message += "\",\"speed\":";
        message += String(currentSpeed);
        message += "}";
        webSocketPtr->broadcastTXT(message);
    }
}

// Handle WebSocket commands
void handleWebSocketCommand(uint8_t clientNum, String command, String value) {
    Serial.print("Handling WebSocket command: ");
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

        // Send acknowledgment
        if (webSocketPtr) {
            String response = "{\"command\":\"speed\",\"value\":";
            response += String(speedValue);
            response += ",\"acknowledged\":true}";
            webSocketPtr->sendTXT(clientNum, response);
        }
    } else if (command == "forward") {
        moveForward();
        broadcastStatus("moving_forward");
    } else if (command == "backward") {
        moveBackward();
        broadcastStatus("moving_backward");
    } else if (command == "left") {
        turnLeft();
        broadcastStatus("turning_left");
    } else if (command == "right") {
        turnRight();
        broadcastStatus("turning_right");
    } else if (command == "stop") {
        stopMovement();
        broadcastStatus("stopped");
    } else if (command == "ping") {
        if (webSocketPtr) {
            webSocketPtr->sendTXT(clientNum, "{\"response\":\"pong\"}");
        }
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
