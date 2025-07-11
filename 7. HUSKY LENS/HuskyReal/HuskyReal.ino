#include <HUSKYLENS.h>
#include <ESP32Servo.h>
#include "SoftwareSerial.h"

SoftwareSerial mySerial(27, 32); // RX, TX
HUSKYLENS huskylens;
Servo cameraServo;

const int cameraServoPin = 23;
const int ledPin = 2;
const int buttonPin = 4;
int leftMotorPin1 = 18;
int leftMotorPin2 = 19;
int rightMotorPin1 = 21;
int rightMotorPin2 = 22;
const int speed = 175;
const int squareTolerance = 5; // Tolerance of 5 pixels for square detection
int buttonLock = 0;

int count_angle = 90;
bool searchForward = true;
unsigned long previousSearchMillis = 0;
const long searchInterval = 200;
const int searchStep = 2;
static int currentCameraAngle = 90;
const int centerAngle = 90; // Center position of the servo
const int trackingStep = 7; // Step size for servo tracking
const int xCenterTarget = 160; // HuskyLens center (320/2 for 320x240 resolution)
const int trackingDeadband = 20; // Deadband for tracking (pixels)

void moveForward() {
  Serial.println(F("Moving Forward"));
  analogWrite(leftMotorPin1, 0);
  analogWrite(leftMotorPin2, speed);
  analogWrite(rightMotorPin1, speed);
  analogWrite(rightMotorPin2, 0);
}

void moveBackward() {
    Serial.println(F("Moving Backward"));
    analogWrite(leftMotorPin1, speed);
    analogWrite(leftMotorPin2, 0);
    analogWrite(rightMotorPin1, 0);
    analogWrite(rightMotorPin2, speed);
}

void turnRight() {
  Serial.println(F("Turning Right"));
  analogWrite(leftMotorPin1, 0);
  analogWrite(leftMotorPin2, speed);
  analogWrite(rightMotorPin1, 0);
  analogWrite(rightMotorPin2, speed);
}

void turnLeft() {
  Serial.println(F("Turning Left"));
  analogWrite(leftMotorPin1, speed);
  analogWrite(leftMotorPin2, 0);
  analogWrite(rightMotorPin1, speed);
  analogWrite(rightMotorPin2, 0);
}

void stopMove() {
  Serial.println(F("Stopping Motor"));
  analogWrite(leftMotorPin1, 0);
  analogWrite(leftMotorPin2, 0);
  analogWrite(rightMotorPin1, 0);
  analogWrite(rightMotorPin2, 0);
}

void searchColor() {
  if (buttonLock == 1) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousSearchMillis >= searchInterval) {
      previousSearchMillis = currentMillis;
      if (searchForward) {
        count_angle += searchStep;
        if (count_angle >= 140) {
          count_angle = 140;
          searchForward = false;
        }
      } 
      else {
        count_angle -= searchStep;
        if (count_angle <= 70) {
          count_angle = 70;
          searchForward = true;
        }
      }
      Serial.print(F("Search angle: "));
      Serial.println(count_angle);
      cameraServo.write(count_angle);
      currentCameraAngle = count_angle;
    }
  }
}

void trackColor(int xCenter) {
  // Calculate error from center
  int error = xCenter - xCenterTarget;
  Serial.print(F("xCenter: "));
  Serial.print(xCenter);
  Serial.print(F(", Error: "));
  Serial.println(error);

  // Only adjust servo if the error is outside the deadband
      if (abs(error) > trackingDeadband) {
      if (error > 0) {
        // Block is to the right, decrease servo angle
        currentCameraAngle -= trackingStep;
      } else {
        // Block is to the left, increase servo angle
        currentCameraAngle += trackingStep;
      }
      // Constrain servo angle to safe range
      currentCameraAngle = constrain(currentCameraAngle, 70, 140);
      Serial.print(F("Tracking angle: "));
      Serial.println(currentCameraAngle);
      cameraServo.write(currentCameraAngle);
      count_angle = currentCameraAngle; // Sync search angle
    }
}

unsigned long lastDebounceTime = 0; // Last time the button state changed
const unsigned long debounceDelay = 50; // Debounce time in milliseconds
int lastButtonState = HIGH; // Previous button state
bool buttonProcessed = false; // Flag to track if press has been processed

void button() {
  int currentButtonState = digitalRead(buttonPin);

  // Check if button state has changed
  if (currentButtonState != lastButtonState) {
    lastDebounceTime = millis(); // Reset debounce timer
  }

  // Check if enough time has passed to consider the state stable
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If button is pressed (LOW for INPUT_PULLUP) and not yet processed
    if (currentButtonState == LOW && !buttonProcessed) {
      buttonLock = (buttonLock + 1) % 2; // Toggle buttonLock
      buttonProcessed = true; // Mark press as processed
      digitalWrite(ledPin, HIGH);
      delay(100); // Brief LED blink
      digitalWrite(ledPin, LOW);
      Serial.print(F("Button pressed, buttonLock: "));
      Serial.println(buttonLock);
    }
    // If button is released (HIGH for INPUT_PULLUP), reset processed flag
    if (currentButtonState == HIGH) {
      buttonProcessed = false; // Allow next press to be processed
    }
  }

  lastButtonState = currentButtonState; // Update last state
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  stopMove();

  cameraServo.attach(cameraServoPin);
  cameraServo.write(90);

  while (!huskylens.begin(mySerial)) {
    Serial.println(F("HuskyLens begin failed!"));
    Serial.println(F("1.Please recheck the \"Protocol Type\" in HUSKYLENS (General Settings>>Protocol Type>>Serial 9600)"));
    Serial.println(F("2.Please recheck the connection."));
    delay(100);
  }
  Serial.println(F("HuskyLens initialized successfully"));
}

void loop() {
  button();
  // Request data from HuskyLens
  if (!huskylens.request()) {
    Serial.println(F("Failed to request data from HuskyLens"));
    stopMove();
    digitalWrite(ledPin, LOW);
    searchColor();
    return;
  }

  // Check if any blocks are detected
  if (huskylens.available()) {
    bool validBlockDetected = false;
    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();
      // Check if the block has ID == 1 and is approximately square
      if (result.command == COMMAND_RETURN_BLOCK && result.ID == 1) {
        if (abs(result.width - result.height) <= squareTolerance) {
          Serial.println(F("Square block detected!"));
          validBlockDetected = true;
          // Track the block's position with the servo
          trackColor(result.xCenter);

          // Calculate error from center
          int error = result.xCenter - xCenterTarget;
          const int movementDeadband = 30; // Pixels, adjust as needed
          const float turnSpeedFactor = 1.5; // Proportional factor for turn speed

          // Blink LED to indicate detection
          digitalWrite(ledPin, LOW);
          delay(100);
          digitalWrite(ledPin, HIGH);

          // Proportional turning based on error
          if (abs(error) <= movementDeadband) {
            // Block is near center, move forward
            Serial.println(F("Block centered, moving forward"));
            moveForward();
          } else {
            // Calculate proportional turn speed (between 0 and 255)
            int turnSpeed = constrain(abs(error) * turnSpeedFactor, 110, speed);
            if (error > 0) {
              // Block is to the right, turn right proportionally
              Serial.print(F("Block on right, turning right with speed: "));
              Serial.println(turnSpeed);
              analogWrite(leftMotorPin1, turnSpeed);
              analogWrite(leftMotorPin2, 0);
              analogWrite(rightMotorPin1, turnSpeed);
              analogWrite(rightMotorPin2, 0);
            } else {
              // Block is to the left, turn left proportionally
              Serial.print(F("Block on left, turning left with speed: "));
              Serial.println(turnSpeed);
              analogWrite(leftMotorPin1, 0);
              analogWrite(leftMotorPin2, turnSpeed);
              analogWrite(rightMotorPin1, 0);
              analogWrite(rightMotorPin2, turnSpeed);
            }
          }
          break; // Exit loop after processing valid block
        }
      }
    }
    // If no valid block was detected
    if (!validBlockDetected) {
      Serial.println(F("No valid square block detected."));
      stopMove();
      digitalWrite(ledPin, LOW);
      searchColor();
    }
  } else {
    // No blocks detected at all
    Serial.println(F("No blocks detected."));
    stopMove();
    digitalWrite(ledPin, LOW);
    searchColor();
  }

  Serial.print("Button: "); Serial.println(buttonLock);
}