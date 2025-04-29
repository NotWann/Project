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
const int targetColorID[2] = {1, 2};   // ID of the yellow ball (ID 1) and another color (ID 2)
const float colorWeights[2] = {1.0, 0.5}; // Weights for each color: ID 1 (yellow) = 1.0, ID 2 = 0.5
const float baseScore = 100.0; // Base score for matching a target color
const float areaWeight = 0.1;  // Weight for block area
const float distancePenalty = 0.5; // Penalty for distance from center
const unsigned long noDetectionTimeout = 2000; // 2 seconds timeout for no detection after confirmed detection
const int minAngleForForward = 30; // Minimum angle for forward movement
const int maxAngleForForward = 150; // Maximum angle for forward movement

int motor1Pin1 = 18;
int motor1Pin2 = 19;

int count_angle = neutralAngle; // Start at neutral
bool searchForward = true; // Oscillation direction
unsigned long previousSearchMillis = 0;
const long searchInterval = 200; // 200ms for slower movement (10°/s)
const int searchStep = 2; // 2° steps for slower scanning
bool locked = false; // Locking state for color detection
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
static bool hasMovedForwardAfterDetection = false; // Flag to track forward movement after confirmed detection
static bool isMovingForward = true; // Flag to track movement direction
static int incorrectDetectionCount = 0; // Count consecutive incorrect detections
static int lastIncorrectID = 0; // Track the last incorrect ID detected
static unsigned long lastIncorrectDetectionMillis = 0; // Timestamp of last incorrect detection
static const long incorrectDetectionCooldown = 1000; // 1 second cooldown for incorrect detections
static const int maxIncorrectDetections = 3; // Max consecutive incorrect detections before moving on

// Servo locking and angle rejection variables
bool servoLocked = false;
int lockedAngle = neutralAngle;
const int maxRejectedAngles = 10;
int rejectedAngles[maxRejectedAngles] = {0};
int rejectedAngleSweepCount[maxRejectedAngles] = {0};
int rejectedAngleCount = 0;
int sweepCount = 0;

// Slight forward movement variable
static bool hasMovedSlightlyForward = false;

// Function for temporary reject that servo angle if the continuous scan happen for a non-desired detection
bool isAngleRejected(int angle) {
  for (int i = 0; i < rejectedAngleCount; i++) {
    if (rejectedAngles[i] == angle) {
      return true;
    }
  }
  return false;
}

// Function to add those rejected angle and skip that angle for the next 2 full sweep servo scan rotation ( 10 - 170 degree)
void addRejectedAngle(int angle) {
  if (rejectedAngleCount < maxRejectedAngles) {
    rejectedAngles[rejectedAngleCount] = angle;
    rejectedAngleSweepCount[rejectedAngleCount] = 0;
    rejectedAngleCount++;
    Serial.print(F("Angle rejected: ")); Serial.println(angle);
  }
}

// Function to update the rejected angle
void updateRejectedAngles() {
  for (int i = 0; i < rejectedAngleCount; i++) {
    rejectedAngleSweepCount[i]++;
    if (rejectedAngleSweepCount[i] >= 2) {
      for (int j = i; j < rejectedAngleCount - 1; j++) {
        rejectedAngles[j] = rejectedAngles[j + 1];
        rejectedAngleSweepCount[j] = rejectedAngleSweepCount[j + 1];
      }
      rejectedAngleCount--;
      i--;
      Serial.print(F("Angle un-rejected after 2 sweeps: ")); Serial.println(rejectedAngles[i]);
    }
  }
}

void moveForward() {
  if (!motorStarted) {
    Serial.println(F("Moving Forward"));
    analogWrite(motor1Pin1, 100);
    analogWrite(motor1Pin2, 0);
    motorStarted = true;
    hasMovedForwardAfterDetection = true;
    isMovingForward = true;
    Serial.println(F("hasMovedForwardAfterDetection set to true"));
  }
}

void moveSlightlyForward() {
  if (!motorStarted) {
    Serial.println(F("Moving slightly forward to confirm detection"));
    analogWrite(motor1Pin1, 100);
    analogWrite(motor1Pin2, 0);
    delay(200);
    analogWrite(motor1Pin1, 0);
    analogWrite(motor1Pin2, 0);
    Serial.println(F("Stopped slight forward movement"));
  }
}

void moveBackward() {
  if (!motorStarted) {
    Serial.println(F("Moving Backward"));
    analogWrite(motor1Pin1, 0);
    analogWrite(motor1Pin2, 100);
    motorStarted = true;
    isMovingForward = false;
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
        sweepCount++;
        updateRejectedAngles();
        Serial.print(F("Sweep count: ")); Serial.println(sweepCount);
      }
    } 
    else {
      count_angle -= searchStep;
      if (count_angle <= 10) {
        count_angle = 10;
        searchForward = true;
        sweepCount++;
        updateRejectedAngles();
        Serial.print(F("Sweep count: "));
        Serial.println(sweepCount);
      }
    }
    Serial.print(F("Search angle: ")); Serial.println(count_angle);
    cameraServo.write(count_angle);
    currentCameraAngle = count_angle;
  }
}

// Function for calculate the error
float getFilteredError() {
  int sum = 0;
  for (int i = 0; i < errorBufferSize; i++) {
    sum += errorBuffer[i];
  }
  return (float)sum / errorBufferSize;
}

// Function for get the average detection
float getAveragedDetectionError() {
  int sum = 0;
  for (int i = 0; i < minDetections; i++) {
    sum += detectionXCenters[i] - screenCenterX;
  }
  return (float)sum / minDetections;
}

/*Function to calculate the weighting assign to each:
  1. Color ID
  2. Block
  3. Center distance of the block 
  The highest weighting score is set as a first priority
*/
float calculateWeightedScore(HUSKYLENSResult result) {
  float score = 0.0;
  bool isTargetColor = false;
  float colorWeight = 0.0;

  for (int i = 0; i < 2; i++) {
    if (result.ID == targetColorID[i]) {
      isTargetColor = true;
      colorWeight = colorWeights[i];
      break;
    }
  }

  if (isTargetColor && abs(result.width - result.height) <= squareTolerance) {
    float blockArea = result.width * result.height;
    float centerDistance = abs(result.xCenter - screenCenterX);
    score = (colorWeight * baseScore) + (blockArea * areaWeight) - (centerDistance * distancePenalty);
  }

  return score;
}

// Function to display result (Debug)
void printResult(HUSKYLENSResult result) {
  if (result.command == COMMAND_RETURN_BLOCK) {
    float score = calculateWeightedScore(result);
    if (score > 0) {
      Serial.print(F("Potential Target Block: xCenter=")); Serial.print(result.xCenter);
      Serial.print(F(", yCenter=")); Serial.print(result.yCenter);
      Serial.print(F(", width=")); Serial.print(result.width);
      Serial.print(F(", height=")); Serial.print(result.height);
      Serial.print(F(", ID=")); Serial.print(result.ID);
      Serial.print(F(", Score=")); Serial.println(score);
    } 
    else {
      Serial.print(F("Incorrect or non-square block: xCenter=")); Serial.print(result.xCenter);
      Serial.print(F(", yCenter=")); Serial.print(result.yCenter);
      Serial.print(F(", width=")); Serial.print(result.width);
      Serial.print(F(", height=")); Serial.print(result.height);
      Serial.print(F(", ID=")); Serial.println(result.ID);
    }
  }
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  stopMove();

  cameraServo.attach(cameraServoPin);
  wheelServo.attach(wheelServoPin);
  cameraServo.write(neutralAngle);
  wheelServo.write(neutralAngle);
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  while (!huskylens.begin(mySerial)) {
    Serial.println(F("HuskyLens begin failed!"));
    Serial.println(F("1.Please recheck the \"Protocol Type\" in HUSKYLENS (General Settings>>Protocol Type>>Serial 9600)"));
    Serial.println(F("2.Please recheck the connection."));
    delay(100);
  }
  Serial.println(F("HuskyLens initialized successfully"));
}

void loop() {
  //Attemp connection with Husky Lens
  if (!huskylens.request()) {
    Serial.println(F("Failed to request data from HuskyLens, check connection!"));
    /*Check if there is no detection for a certain time after the last detection.
      Search time is 500ms*/
    if (locked && (millis() - lastDetectionMillis < searchTimeout)) {
      int searchAngle = lastCameraAngle + (lastXCenter > screenCenterX ? -searchRange : searchRange);
      searchAngle = constrain(searchAngle, 10, 170);
      cameraServo.write(searchAngle);
      currentCameraAngle = searchAngle;
      Serial.print(F("Targeted search at angle: ")); Serial.println(searchAngle);
    } 
    else {
      //Reset detection function, servo camera angle at neutral (90 degree)
      locked = false;
      detectionCount = 0;
      lastDetectedID = 0;
      detectionIndex = 0;
      servoLocked = false;
      hasMovedSlightlyForward = false;
      updateSearchAngle();
      wheelServo.write(neutralAngle);
      currentWheelAngle = neutralAngle;
      stopMove();
      digitalWrite(ledPin, LOW);
    }
  } 
  // Reset if husky doesn't detect anything.
  else if (!huskylens.isLearned()) {
    Serial.println(F("No colors learned. Press learn button on HuskyLens to learn a color!"));
    locked = false;
    detectionCount = 0;
    lastDetectedID = 0;
    detectionIndex = 0;
    servoLocked = false;
    hasMovedSlightlyForward = false;
    updateSearchAngle();
    wheelServo.write(neutralAngle);
    currentWheelAngle = neutralAngle;
    stopMove();
    digitalWrite(ledPin, LOW);
  } 
  //Function for search the color for the servo camera sweep scan angle
  else if (!huskylens.available()) {
    Serial.println(F("No color detected on screen!"));
    if (locked && (millis() - lastDetectionMillis < searchTimeout)) {
      int searchAngle = lastCameraAngle + (lastXCenter > screenCenterX ? -searchRange : searchRange);
      searchAngle = constrain(searchAngle, 10, 170);
      cameraServo.write(searchAngle);
      currentCameraAngle = searchAngle;
      Serial.print(F("Targeted search at angle: ")); Serial.println(searchAngle);
    } 
    else {
      //Reset
      locked = false;
      detectionCount = 0;
      lastDetectedID = 0;
      detectionIndex = 0;
      servoLocked = false;
      hasMovedSlightlyForward = false;
      updateSearchAngle();
      wheelServo.write(neutralAngle);
      currentWheelAngle = neutralAngle;
      
      /*Check if the Robot has move forward
        If true, the Robot will move 1 seconds backward, and then reset the condition for the move forward logic*/
      if (hasMovedForwardAfterDetection) {
        if (millis() - lastDetectionMillis >= noDetectionTimeout) {
          moveBackward();
          delay(1000);
          stopMove();
          hasMovedForwardAfterDetection = false;
          Serial.println(F("Moved backward after no detection timeout"));
          delay(500);
        }
      }
      
      digitalWrite(ledPin, LOW);
    }
  } 
  else {
    Serial.println(F("###########"));
    float highestScore = 0.0;
    int bestXCenter = screenCenterX;
    int bestYCenter = 0;
    int bestWidth = 0;
    int bestHeight = 0;
    int bestID = 0;
    bool foundPotentialTarget = false;

    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();
      printResult(result);
      
      if (result.command == COMMAND_RETURN_BLOCK) {
        //Check if servo is locked after the color is detected, if yes 
        if (!servoLocked && !isAngleRejected(currentCameraAngle)) {
          servoLocked = true;
          lockedAngle = currentCameraAngle;
          Serial.print(F("Servo locked at angle: ")); Serial.println(lockedAngle);
        }
        
        float score = calculateWeightedScore(result);
        if (score > highestScore) {
          highestScore = score;
          bestXCenter = result.xCenter;
          bestYCenter = result.yCenter;
          bestWidth = result.width;
          bestHeight = result.height;
          bestID = result.ID;
          foundPotentialTarget = true;
          incorrectDetectionCount = 0;
        } 
        else if (score <= 0) {
          if (result.ID == lastIncorrectID && 
              abs(result.xCenter - lastXCenter) <= xCenterTolerance && 
              (millis() - lastIncorrectDetectionMillis < incorrectDetectionCooldown)) {
            incorrectDetectionCount++;
            Serial.print(F("Repetitive incorrect detection, count: ")); Serial.println(incorrectDetectionCount);
          } 
          else {
            lastIncorrectID = result.ID;
            lastIncorrectDetectionMillis = millis();
            incorrectDetectionCount = 1;
            Serial.print(F("New incorrect detection, starting count: ")); Serial.println(incorrectDetectionCount);
          }

          if (incorrectDetectionCount >= maxIncorrectDetections) {
            Serial.println(F("Too many incorrect detections, rejecting angle and resuming scan..."));
            addRejectedAngle(lockedAngle);
            servoLocked = false;
            incorrectDetectionCount = 0;
          }
          continue;
        }
      }
    }

    if (foundPotentialTarget) {
      if (!hasMovedSlightlyForward) {
        moveSlightlyForward();
        hasMovedSlightlyForward = true;
      }

      if (bestID == lastDetectedID && abs(bestXCenter - lastXCenter) <= xCenterTolerance) {
        detectionCount++;
        detectionXCenters[detectionIndex] = bestXCenter;
        detectionIndex = (detectionIndex + 1) % minDetections;
      } 
      else {
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
      
      Serial.print(F("Selected Best Block: xCenter=")); Serial.print(bestXCenter);
      Serial.print(F(", yCenter=")); Serial.print(bestYCenter);
      Serial.print(F(", width=")); Serial.print(bestWidth);
      Serial.print(F(", height=")); Serial.print(bestHeight);
      Serial.print(F(", ID=")); Serial.print(bestID);
      Serial.print(F(", Score=")); Serial.print(highestScore);
      Serial.print(F(", Detection Count: ")); Serial.println(detectionCount);
      
      if (!servoLocked) {
        int error = bestXCenter - screenCenterX;
        errorBuffer[errorBufferIndex] = error;
        errorBufferIndex = (errorBufferIndex + 1) % errorBufferSize;
        float filteredError = getFilteredError();
        
        if (abs(filteredError) > cameraErrorDeadband) {
          float cameraAngleChange = Kp_camera * filteredError;
          int targetCameraAngle = constrain(currentCameraAngle - cameraAngleChange, 10, 170);
          int cameraStep = constrain(targetCameraAngle - currentCameraAngle, -maxAngleStep, maxAngleStep);
          currentCameraAngle += cameraStep;
          cameraServo.write(currentCameraAngle);
          lastCameraAngle = currentCameraAngle;
          Serial.print(F("Camera servo adjusted, error: "));
          Serial.println(filteredError);
        } 
        else {
          Serial.println(F("Camera servo: Centered, no adjustment needed"));
        }
      }
      
      // Adjust wheel servo angle for potential detection
      int targetWheelAngle;
      if (isMovingForward) {
        targetWheelAngle = map(currentCameraAngle, 10, 170, 120, 40); // Forward mapping
      } 
      else {
        targetWheelAngle = map(currentCameraAngle, 10, 170, 40, 120); // Reverse mapping for backward movement
      }
      int wheelStep = constrain(targetWheelAngle - currentWheelAngle, -maxAngleStep, maxAngleStep);
      currentWheelAngle += wheelStep;
      wheelServo.write(currentWheelAngle);
      Serial.print(F("Wheel servo adjusted for potential detection, angle: ")); Serial.println(currentWheelAngle);
      
      // Check camera angle for movement direction on potential detection
      if (currentCameraAngle < minAngleForForward || currentCameraAngle > maxAngleForForward) {
        moveBackward();
        delay(1000);
        stopMove();
        hasMovedForwardAfterDetection = false; // Did not move forward
        Serial.println(F("Moved backward due to camera angle outside 30-150 degrees for potential detection"));
      } 
      else {
        stopMove(); // Stop motor until confirmed detection
      }
      
      if (detectionCount >= minDetections && millis() - detectionStartMillis >= motorDelay) {
        digitalWrite(ledPin, HIGH);
        Serial.print(F("Confirmed detection! Camera angle: ")); Serial.print(currentCameraAngle);
        Serial.print(F(" deg, Wheel angle: ")); Serial.print(currentWheelAngle); Serial.println(F(" deg"));
        
        // Check camera angle again for confirmed detection (already handled above, but ensure consistency)
        if (currentCameraAngle < minAngleForForward || currentCameraAngle > maxAngleForForward) {
          moveBackward();
          delay(1000);
          stopMove();
          hasMovedForwardAfterDetection = false;
          Serial.println(F("Moved backward due to camera angle outside 30-150 degrees for confirmed detection"));
        } 
        else {
          moveForward();
        }
        
        servoLocked = false;
        hasMovedSlightlyForward = false;
      } 
      else {
        Serial.println(F("Waiting for confirmed detection..."));
        digitalWrite(ledPin, LOW);
      }
    } 
    else {
      Serial.println(F("No potential target detected!"));
      detectionStartMillis = 0;
      detectionCount = 0;
      lastDetectedID = 0;
      detectionIndex = 0;
      hasMovedSlightlyForward = false;
      if (locked && (millis() - lastDetectionMillis < searchTimeout)) {
        int searchAngle = lastCameraAngle + (lastXCenter > screenCenterX ? -searchRange : searchRange);
        searchAngle = constrain(searchAngle, 10, 170);
        cameraServo.write(searchAngle);
        currentCameraAngle = searchAngle;
        Serial.print(F("Targeted search at angle: ")); Serial.println(searchAngle);
      } 
      else {
        locked = false;
        servoLocked = false;
        updateSearchAngle();
        wheelServo.write(neutralAngle);
        currentWheelAngle = neutralAngle;
        
        if (hasMovedForwardAfterDetection) {
          if (millis() - lastDetectionMillis >= noDetectionTimeout) {
            moveBackward();
            delay(1000);
            stopMove();
            hasMovedForwardAfterDetection = false;
            Serial.println(F("Moved backward after no detection timeout"));
            delay(500);
          }
        }
        
        digitalWrite(ledPin, LOW);
      }
    }
  }
  delay(50);
}