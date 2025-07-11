#include <HUSKYLENS.h>
#include <ESP32Servo.h>
#include "SoftwareSerial.h"

SoftwareSerial mySerial(27, 32); // RX, TX
HUSKYLENS huskylens;
Servo cameraServo;
Servo wheelServo;
const int cameraServoPin = 23;
const int wheelServoPin = 26;
const int ledPin = 2;
const float Kp_camera = 0.1;   // Camera proportional gain
const int screenCenterX = 160;
const int neutralAngle = 90;
const int maxAngleStep = 3;    // Max 3° per 50ms
const unsigned long motorDelay = 500; // Delay motor start after confirmation
const int errorDeadband = 3;   // Stop steering if |error| < 3px
const int cameraErrorDeadband = 5; // Deadband for camera servo to prevent fluctuation (±5 pixels)
const int errorBufferSize = 5; // Moving average over 5 samples
const float dt = 0.05;         // Loop time (50ms)
const int squareTolerance = 3; // Width and height within ±3 pixels
const int minDetections = 5;   // Require 5 consecutive detections
const int xCenterTolerance = 20; // ±20 pixels for consistent xCenter
const int targetColorID = 1;   // ID of the yellow ball (adjust based on your HuskyLens setup)

int motor1Pin1 = 18;
int motor1Pin2 = 19;

int count_angle = neutralAngle; // Start at neutral
bool searchForward = true; // Oscillation direction
unsigned long previousSearchMillis = 0;
const long searchInterval = 200; // 200ms for slower movement (10°/s)
const int searchStep = 2; // 2° steps for slower scanning
bool locked = false; // Locking state
int lastCameraAngle = neutralAngle; // Last known angle
int lastXCenter = screenCenterX; // Last known xCenter
unsigned long lastDetectionMillis = 0; // Time of last detection
const long searchTimeout = 500; // Search for 500ms after losing color
const int searchRange = 45; // Search ±45° from last angle
static int currentCameraAngle = neutralAngle;
static int currentWheelAngle = neutralAngle;
static bool motorStarted = false; // Track motor state
static unsigned long detectionStartMillis = 0; // Motor delay timer
static int errorBuffer[errorBufferSize] = {0}; // Moving average buffer
static int errorBufferIndex = 0;
static int detectionCount = 0; // Count consecutive detections
static int lastDetectedID = 0; // Last detected block ID
static int detectionXCenters[minDetections] = {0}; // Buffer for xCenter values
static int detectionIndex = 0; // Index for xCenter buffer
static bool hasMovedForwardAfterDetection = false; // Flag to track forward movement after detection
static int incorrectDetectionCount = 0; // Count consecutive incorrect detections
static int lastIncorrectID = 0; // Track the last incorrect ID detected
static unsigned long lastIncorrectDetectionMillis = 0; // Timestamp of last incorrect detection
static const long incorrectDetectionCooldown = 1000; // 1 second cooldown for incorrect detections
static const int maxIncorrectDetections = 3; // Max consecutive incorrect detections before moving on

void moveForward() {
  if (!motorStarted) {
    Serial.println(F("Moving Forward"));
    analogWrite(motor1Pin1, 100);
    analogWrite(motor1Pin2, 0);
    motorStarted = true;
    hasMovedForwardAfterDetection = true; // Set flag after moving forward
    Serial.println(F("hasMovedForwardAfterDetection set to true"));
  }
}

void moveBackward() {
  if (!motorStarted) {
    Serial.println(F("Moving Backward"));
    analogWrite(motor1Pin1, 0);
    analogWrite(motor1Pin2, 100);
    motorStarted = true;
  }
}

void stopMove() {
  if (motorStarted) {
    Serial.println(F("Stopping Motor"));
    analogWrite(motor1Pin1, 0);
    analogWrite(motor1Pin2, 0);
    motorStarted = false;
  }
}

void updateSearchAngle() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousSearchMillis >= searchInterval) {
    previousSearchMillis = currentMillis;
    if (searchForward) {
      count_angle += searchStep;
      if (count_angle >= 170) {
        count_angle = 170;
        searchForward = false;
      }
    } else {
      count_angle -= searchStep;
      if (count_angle <= 10) {
        count_angle = 10;
        searchForward = true;
      }
    }
    Serial.print(F("Search angle: "));
    Serial.println(count_angle);
    cameraServo.write(count_angle);
    currentCameraAngle = count_angle;
  }
}

float getFilteredError() {
  int sum = 0;
  for (int i = 0; i < errorBufferSize; i++) {
    sum += errorBuffer[i];
  }
  return (float)sum / errorBufferSize;
}

float getAveragedDetectionError() {
  int sum = 0;
  for (int i = 0; i < minDetections; i++) {
    sum += detectionXCenters[i] - screenCenterX;
  }
  return (float)sum / minDetections;
}

void printResult(HUSKYLENSResult result) {
  if (result.command == COMMAND_RETURN_BLOCK) {
    if (result.ID == targetColorID && abs(result.width - result.height) <= squareTolerance) {
      Serial.print(F("Correct Yellow Square Block: xCenter="));
      Serial.print(result.xCenter);
      Serial.print(F(", yCenter="));
      Serial.print(result.yCenter);
      Serial.print(F(", width="));
      Serial.print(result.width);
      Serial.print(F(", height="));
      Serial.print(result.height);
      Serial.print(F(", ID="));
      Serial.println(result.ID);
    } else {
      Serial.print(F("Incorrect or non-square block: xCenter="));
      Serial.print(result.xCenter);
      Serial.print(F(", yCenter="));
      Serial.print(result.yCenter);
      Serial.print(F(", width="));
      Serial.print(result.width);
      Serial.print(F(", height="));
      Serial.print(result.height);
      Serial.print(F(", ID="));
      Serial.println(result.ID);
    }
  }
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  stopMove();

  // Initialize servos
  cameraServo.attach(cameraServoPin);
  wheelServo.attach(wheelServoPin);
  cameraServo.write(neutralAngle);
  wheelServo.write(neutralAngle);
  
  // Initialize LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Initialize HuskyLens
  while (!huskylens.begin(mySerial)) {
    Serial.println(F("HuskyLens begin failed!"));
    Serial.println(F("1.Please recheck the \"Protocol Type\" in HUSKYLENS (General Settings>>Protocol Type>>Serial 9600)"));
    Serial.println(F("2.Please recheck the connection."));
    delay(100);
  }
  Serial.println(F("HuskyLens initialized successfully"));
}

void loop() {
  if (!huskylens.request()) {
    Serial.println(F("Failed to request data from HuskyLens, check connection!"));
    if (locked && (millis() - lastDetectionMillis < searchTimeout)) {
      int searchAngle = lastCameraAngle + (lastXCenter > screenCenterX ? -searchRange : searchRange);
      searchAngle = constrain(searchAngle, 10, 170);
      cameraServo.write(searchAngle);
      currentCameraAngle = searchAngle;
      Serial.print(F("Targeted search at angle: "));
      Serial.println(searchAngle);
    } else {
      locked = false;
      detectionCount = 0;
      lastDetectedID = 0;
      detectionIndex = 0;
      updateSearchAngle();
      wheelServo.write(neutralAngle);
      currentWheelAngle = neutralAngle;
      stopMove();
      digitalWrite(ledPin, LOW);
    }
  } else if (!huskylens.isLearned()) {
    Serial.println(F("No colors learned. Press learn button on HuskyLens to learn a color!"));
    locked = false;
    detectionCount = 0;
    lastDetectedID = 0;
    detectionIndex = 0;
    updateSearchAngle();
    wheelServo.write(neutralAngle);
    currentWheelAngle = neutralAngle;
    stopMove();
    digitalWrite(ledPin, LOW);
  } else if (!huskylens.available()) {
    Serial.println(F("No color detected on screen!"));
    if (locked && (millis() - lastDetectionMillis < searchTimeout)) {
      int searchAngle = lastCameraAngle + (lastXCenter > screenCenterX ? -searchRange : searchRange);
      searchAngle = constrain(searchAngle, 10, 170);
      cameraServo.write(searchAngle);
      currentCameraAngle = searchAngle;
      Serial.print(F("Targeted search at angle: "));
      Serial.println(searchAngle);
    } else {
      locked = false;
      detectionCount = 0;
      lastDetectedID = 0;
      detectionIndex = 0;
      updateSearchAngle();
      wheelServo.write(neutralAngle);
      currentWheelAngle = neutralAngle;
      
      // Move backward only if we previously moved forward
      if (hasMovedForwardAfterDetection) {
        moveBackward();
        delay(1000); // Move backward for 1 second
        stopMove();
        hasMovedForwardAfterDetection = false;
        Serial.println(F("Moved backward after losing color detection"));
        delay(500); // Add delay to stabilize before resuming scan
      }
      
      digitalWrite(ledPin, LOW);
    }
  } else {
    Serial.println(F("###########"));
    int largestSquareWidth = 0;
    int bestXCenter = screenCenterX;
    int bestYCenter = 0;
    int bestHeight = 0;
    int bestID = 0;
    float smallestError = 1000.0;
    bool foundCorrectSquare = false;

    // Process all available blocks
    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();
      printResult(result);
      
      // Check if the detected block matches the target color ID and is a square
      if (result.command == COMMAND_RETURN_BLOCK && 
          result.ID == targetColorID && 
          abs(result.width - result.height) <= squareTolerance) {
        // Correct color and square block detected
        int error = abs(result.xCenter - screenCenterX);
        if (result.width > largestSquareWidth || (result.width == largestSquareWidth && error < smallestError)) {
          largestSquareWidth = result.width;
          bestXCenter = result.xCenter;
          bestYCenter = result.yCenter;
          bestHeight = result.height;
          bestID = result.ID;
          smallestError = error;
          foundCorrectSquare = true;
          incorrectDetectionCount = 0; // Reset incorrect detection counter
        }
      } else if (result.command == COMMAND_RETURN_BLOCK) {
        // Incorrect color or non-square block detected
        if (result.ID == lastIncorrectID && 
            abs(result.xCenter - lastXCenter) <= xCenterTolerance && 
            (millis() - lastIncorrectDetectionMillis < incorrectDetectionCooldown)) {
          incorrectDetectionCount++;
          Serial.print(F("Repetitive incorrect detection, count: "));
          Serial.println(incorrectDetectionCount);
        } else {
          lastIncorrectID = result.ID;
          lastIncorrectDetectionMillis = millis();
          incorrectDetectionCount = 1;
          Serial.print(F("New incorrect detection, starting count: "));
          Serial.println(incorrectDetectionCount);
        }

        // If we've seen this incorrect block too many times, force a scan
        if (incorrectDetectionCount >= maxIncorrectDetections) {
          Serial.println(F("Too many incorrect detections, forcing scan to move on..."));
          updateSearchAngle();
          incorrectDetectionCount = 0; // Reset counter after forcing a scan
        }
        continue;
      }
    }

    if (foundCorrectSquare) {
      // Check for consistent detection of the correct color
      if (bestID == lastDetectedID && abs(bestXCenter - lastXCenter) <= xCenterTolerance) {
        detectionCount++;
        detectionXCenters[detectionIndex] = bestXCenter;
        detectionIndex = (detectionIndex + 1) % minDetections;
      } else {
        detectionCount = 1;
        lastDetectedID = bestID;
        for (int i = 0; i < minDetections; i++) {
          detectionXCenters[i] = bestXCenter;
        }
        detectionIndex = 1;
      }
      
      locked = true;
      lastDetectionMillis = millis();
      lastXCenter = bestXCenter;
      
      if (detectionStartMillis == 0) {
        detectionStartMillis = millis();
      }
      
      Serial.print(F("Selected Correct Yellow Square Block: xCenter="));
      Serial.print(bestXCenter);
      Serial.print(F(", yCenter="));
      Serial.print(bestYCenter);
      Serial.print(F(", width="));
      Serial.print(largestSquareWidth);
      Serial.print(F(", height="));
      Serial.print(bestHeight);
      Serial.print(F(", ID="));
      Serial.print(bestID);
      Serial.print(F(", Detection Count: "));
      Serial.println(detectionCount);
      
      // Calculate error (pixels from center) for camera servo
      int error = bestXCenter - screenCenterX;
      errorBuffer[errorBufferIndex] = error;
      errorBufferIndex = (errorBufferIndex + 1) % errorBufferSize;
      float filteredError = getFilteredError();
      
      // Camera servo: Apply deadband to prevent fluctuation
      if (abs(filteredError) > cameraErrorDeadband) {
        float cameraAngleChange = Kp_camera * filteredError;
        int targetCameraAngle = constrain(currentCameraAngle - cameraAngleChange, 10, 170);
        int cameraStep = constrain(targetCameraAngle - currentCameraAngle, -maxAngleStep, maxAngleStep);
        currentCameraAngle += cameraStep;
        cameraServo.write(currentCameraAngle);
        lastCameraAngle = currentCameraAngle;
        Serial.print(F("Camera servo adjusted, error: "));
        Serial.println(filteredError);
      } else {
        Serial.println(F("Camera servo: Centered, no adjustment needed"));
      }
      
      // Motor and wheel servo: Activate only after minDetections
      if (detectionCount >= minDetections && millis() - detectionStartMillis >= motorDelay) {
        // Map camera angle to wheel angle (inverse relationship)
        // Camera: 10° (right) to 170° (left) -> Wheel: 120° (left) to 40° (right)
        int targetWheelAngle = map(currentCameraAngle, 10, 170, 120, 40);
        int wheelStep = constrain(targetWheelAngle - currentWheelAngle, -maxAngleStep, maxAngleStep);
        currentWheelAngle += wheelStep;
        wheelServo.write(currentWheelAngle);
        Serial.print(F("Wheel servo adjusted, angle: "));
        Serial.println(currentWheelAngle);
        
        digitalWrite(ledPin, HIGH);
        Serial.print(F("Confirmed detection! Camera angle: "));
        Serial.print(currentCameraAngle);
        Serial.print(F(" deg, Wheel angle: "));
        Serial.print(currentWheelAngle);
        Serial.println(F(" deg"));
        
        moveForward();
      } else {
        Serial.println(F("Waiting for confirmed detection..."));
        stopMove();
        wheelServo.write(neutralAngle);
        currentWheelAngle = neutralAngle;
        digitalWrite(ledPin, LOW);
      }
    } else {
      Serial.println(F("No correct yellow square detected!"));
      detectionStartMillis = 0;
      detectionCount = 0;
      lastDetectedID = 0;
      detectionIndex = 0;
      if (locked && (millis() - lastDetectionMillis < searchTimeout)) {
        int searchAngle = lastCameraAngle + (lastXCenter > screenCenterX ? -searchRange : searchRange);
        searchAngle = constrain(searchAngle, 10, 170);
        cameraServo.write(searchAngle);
        currentCameraAngle = searchAngle;
        Serial.print(F("Targeted search at angle: "));
        Serial.println(searchAngle);
      } else {
        locked = false;
        updateSearchAngle();
        wheelServo.write(neutralAngle);
        currentWheelAngle = neutralAngle;
        
        // Move backward only if we previously moved forward
        if (hasMovedForwardAfterDetection) {
          moveBackward();
          delay(1000);
          stopMove();
          hasMovedForwardAfterDetection = false;
          Serial.println(F("Moved backward after losing color detection"));
          delay(500); // Add delay to stabilize before resuming scan
        }
        
        digitalWrite(ledPin, LOW);
      }
    }
  }
  delay(50);
}