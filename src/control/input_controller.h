#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include <Arduino.h>

// Forward declaration for WebSocket server
class WebSocketsServer;

extern WebSocketsServer* webSocketPtr;

// Control commands
void initController();
void setSpeed(int speed);  // 0-100
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopMovement();

// WebSocket integration
void setWebSocketServer(WebSocketsServer* ws);
void broadcastStatus(const char* status);
void handleWebSocketCommand(uint8_t clientNum, String command, String value);

#endif
