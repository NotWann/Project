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
const float Kp_steer = 0.05;   // PID proportional gain
const float Ki_steer = 0.0125; // PID integral gain
const float Kd_steer = 0.02;   // PID derivative gain
const int screenCenterX = 160;
const int neutralAngle = 90;
const int maxAngleStep = 3;    // Max 3° per 50ms
const unsigned long motorDelay = 500; // Delay motor start after confirmation
const int errorDeadband = 3;  // Stop steering if |error| < 10px
const int errorBufferSize = 5; // Moving average over 5 samples
const float dt = 0.1;         // Loop time (50ms)
const int squareTolerance = 3; // Width and height within ±3 pixels
const int minDetections = 5;   // Require 5 consecutive detections
const int xCenterTolerance = 20; // ±20 pixels for consistent xCenter

int motor1Pin1 = 18;
int motor1Pin2 = 19;

int count_angle = neutralAngle; // Start at neutral
bool searchForward = true; // Oscillation direction
unsigned long previousSearchMillis = 0;
const long searchInterval = 100; // Update every 100ms
const int searchStep = 10; // 5° steps
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
static float integral = 0; // PID integral term
static float prevError = 0; // PID previous error
static int detectionCount = 0; // Count consecutive detections
static int lastDetectedID = 0; // Last detected block ID
static int detectionXCenters[minDetections] = {0}; // Buffer for xCenter values
static int detectionIndex = 0; // Index for xCenter buffer

void moveForward() {
  if (!motorStarted) {
    Serial.println(F("Moving Forward"));
    analogWrite(motor1Pin1, 100);
    analogWrite(motor1Pin2, 0);
    motorStarted = true;
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
    Serial.println(String() + F("Search angle: ") + count_angle);
    cameraServo.write(count_angle);
    currentCameraAngle = count_angle; // Update for smooth transitions
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
  if (result.command == COMMAND_RETURN_BLOCK && result.ID >= 1 && result.ID <= 6 && abs(result.width - result.height) <= squareTolerance) {
    Serial.println(String() + F("Square Block: xCenter=") + result.xCenter + 
                  F(", yCenter=") + result.yCenter + F(", width=") + result.width + 
                  F(", height=") + result.height + F(", ID=") + result.ID);
  } else {
    Serial.println(String() + F("Non-square or invalid block: xCenter=") + result.xCenter + 
                  F(", yCenter=") + result.yCenter + F(", width=") + result.width + 
                  F(", height=") + result.height + F(", ID=") + result.ID);
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
      Serial.println(String() + F("Targeted search at angle: ") + searchAngle);
    } else {
      locked = false;
      detectionCount = 0; // Reset detection counter
      lastDetectedID = 0; // Reset detected ID
      detectionIndex = 0; // Reset detection buffer index
      updateSearchAngle();
      wheelServo.write(neutralAngle);
      currentWheelAngle = neutralAngle;
      integral = 0; // Reset PID integral
      prevError = 0; // Reset PID previous error
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
    integral = 0;
    prevError = 0;
    stopMove();
    digitalWrite(ledPin, LOW);
  } else if (!huskylens.available()) {
    Serial.println(F("No color detected on screen!"));
    if (locked && (millis() - lastDetectionMillis < searchTimeout)) {
      int searchAngle = lastCameraAngle + (lastXCenter > screenCenterX ? -searchRange : searchRange);
      searchAngle = constrain(searchAngle, 10, 170);
      cameraServo.write(searchAngle);
      currentCameraAngle = searchAngle;
      Serial.println(String() + F("Targeted search at angle: ") + searchAngle);
    } else {
      locked = false;
      detectionCount = 0;
      lastDetectedID = 0;
      detectionIndex = 0;
      updateSearchAngle();
      wheelServo.write(neutralAngle);
      currentWheelAngle = neutralAngle;
      integral = 0;
      prevError = 0;
      stopMove();
      digitalWrite(ledPin, LOW);
    }
  } else {
    Serial.println(F("###########"));
    int largestSquareWidth = 0;
    int bestXCenter = screenCenterX;
    int bestYCenter = 0;
    int bestHeight = 0;
    int bestID = 0;
    float smallestError = 1000.0; // Large initial value
    bool foundSquare = false;

    // Process all available blocks
    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();
      printResult(result);
      if (result.command == COMMAND_RETURN_BLOCK && result.ID >= 1 && result.ID <= 6 && abs(result.width - result.height) <= squareTolerance) {
        // Square block detected
        int error = abs(result.xCenter - screenCenterX);
        // Prioritize larger blocks, and among equals, closer to center
        if (result.width > largestSquareWidth || (result.width == largestSquareWidth && error < smallestError)) {
          largestSquareWidth = result.width;
          bestXCenter = result.xCenter;
          bestYCenter = result.yCenter;
          bestHeight = result.height;
          bestID = result.ID;
          smallestError = error;
          foundSquare = true;
        }
      }
    }

    if (foundSquare) {
      // Check for consistent detection
      if (bestID == lastDetectedID && abs(bestXCenter - lastXCenter) <= xCenterTolerance) {
        // Consistent block: increment counter and store xCenter
        detectionCount++;
        detectionXCenters[detectionIndex] = bestXCenter;
        detectionIndex = (detectionIndex + 1) % minDetections;
      } else {
        // New or inconsistent block: reset counter and buffer
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
      
      Serial.println(String() + F("Selected Yellow Square Block: xCenter=") + bestXCenter + 
                    F(", yCenter=") + bestYCenter + F(", width=") + largestSquareWidth + 
                    F(", height=") + bestHeight + F(", ID=") + bestID + 
                    F(", Detection Count: ") + detectionCount);
      
      // Calculate error (pixels from center) for camera servo
      int error = bestXCenter - screenCenterX;
      errorBuffer[errorBufferIndex] = error;
      errorBufferIndex = (errorBufferIndex + 1) % errorBufferSize;
      float filteredError = getFilteredError();
      
      // Camera servo: Non-blocking update
      float cameraAngleChange = Kp_camera * filteredError;
      int targetCameraAngle = constrain(currentCameraAngle - cameraAngleChange, 10, 170);
      int cameraStep = constrain(targetCameraAngle - currentCameraAngle, -maxAngleStep, maxAngleStep);
      currentCameraAngle += cameraStep;
      cameraServo.write(currentCameraAngle);
      lastCameraAngle = currentCameraAngle;
      
      // Motor and wheel servo: Activate only after minDetections
      if (detectionCount >= minDetections && millis() - detectionStartMillis >= motorDelay) {
        // Use averaged error from confirmed detections
        float confirmedError = getAveragedDetectionError();
        
        // Wheel servo: PID control
        if (abs(confirmedError) > errorDeadband) {
          integral += confirmedError * dt;
          integral = constrain(integral, -1000, 1000); // Prevent windup
          float derivative = (confirmedError - prevError) / dt;
          float output = Kp_steer * confirmedError + Ki_steer * integral + Kd_steer * derivative;
          int targetWheelAngle = constrain(currentWheelAngle + output, 40, 120);
          int wheelStep = constrain(targetWheelAngle - currentWheelAngle, -maxAngleStep, maxAngleStep);
          currentWheelAngle += wheelStep;
          wheelServo.write(currentWheelAngle);
          prevError = confirmedError;
        } else {
          Serial.println(F("Wheel servo: Centered, no steering"));
          integral = 0; // Reset integral when centered
          prevError = confirmedError;
        }
        
        digitalWrite(ledPin, HIGH);
        Serial.println(String() + F("Confirmed detection! Confirmed Error: ") + confirmedError + 
                      F(" pixels, Camera angle: ") + currentCameraAngle + 
                      F(" deg, Wheel angle: ") + currentWheelAngle + F(" deg"));
        
        moveForward();
      } else {
        Serial.println(F("Waiting for confirmed detection..."));
        stopMove();
        wheelServo.write(neutralAngle);
        currentWheelAngle = neutralAngle;
        integral = 0;
        prevError = 0;
        digitalWrite(ledPin, LOW);
      }
    } else {
      Serial.println(F("No yellow square detected!"));
      detectionStartMillis = 0;
      detectionCount = 0;
      lastDetectedID = 0;
      detectionIndex = 0;
      if (locked && (millis() - lastDetectionMillis < searchTimeout)) {
        int searchAngle = lastCameraAngle + (lastXCenter > screenCenterX ? -searchRange : searchRange);
        searchAngle = constrain(searchAngle, 10, 170);
        cameraServo.write(searchAngle);
        currentCameraAngle = searchAngle;
        Serial.println(String() + F("Targeted search at angle: ") + searchAngle);
      } else {
        locked = false;
        updateSearchAngle();
        wheelServo.write(neutralAngle);
        currentCameraAngle = neutralAngle;
        currentWheelAngle = neutralAngle;
        integral = 0;
        prevError = 0;
        moveBackward();
        delay(1000);
        stopMove();
        digitalWrite(ledPin, LOW);
      }
    }
  }
  delay(50);
}