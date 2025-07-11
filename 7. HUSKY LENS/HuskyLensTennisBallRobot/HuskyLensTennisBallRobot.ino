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
int buttonLock = 0;

// Weightings for each color ID (1 to 10, representing different yellow shades)
const float colorWeights[] = {
  0.0,  // ID 0 (not used)
  1.0,  // ID 1 (e.g., bright yellow)
  0.9,  // ID 2 (e.g., slightly dim yellow)
  0.8,  // ID 3
  0.7,  // ID 4
  0.6,  // ID 5
  0.5,  // ID 6
  0.4,  // ID 7
  0.3,  // ID 8
  0.2,  // ID 9
  0.1   // ID 10 (e.g., shadowed yellow)
};
const int maxColorID = 10; // Maximum valid color ID
const int proximityThreshold = 50; // Pixel distance for clustering
const int trackingStep = 5; // Reduced for smoother servo tracking
const int xCenterTarget = 160; // HuskyLens center (320/2)
const int trackingDeadband = 30; // Increased to reduce servo jitter
const int movementDeadband = 20; // Reduced for stricter centering
const float turnSpeedFactor = 2.0; // Increased for faster turns
const unsigned long scanInterval = 4500; // 3 seconds per scan direction
const unsigned long noDetectionTimeout = 3000; // Increased to 3 seconds
const unsigned long retreatDuration = 3000; // 3 seconds backward
const int hitSizeThreshold = 150; // Width or height to detect "hit"

enum State {
  SCANNING,
  CENTERING,
  MOVING_FORWARD,
  RETREATING
};
State currentState = SCANNING;
bool scanRight = true; // Scanning direction
unsigned long stateStartTime = 0; // Time when state began
unsigned long lastDetectionTime = 0; // Last valid cluster detection
static int currentCameraAngle = 90; // Servo angle

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
  analogWrite(leftMotorPin1, speed);
  analogWrite(leftMotorPin2, 0);
  analogWrite(rightMotorPin1, speed);
  analogWrite(rightMotorPin2, 0);
}

void turnLeft() {
  Serial.println(F("Turning Left"));
  analogWrite(leftMotorPin1, 0);
  analogWrite(leftMotorPin2, speed);
  analogWrite(rightMotorPin1, 0);
  analogWrite(rightMotorPin2, speed);
}

void stopMove() {
  Serial.println(F("Stopping Motor"));
  analogWrite(leftMotorPin1, 0);
  analogWrite(leftMotorPin2, 0);
  analogWrite(rightMotorPin1, 0);
  analogWrite(rightMotorPin2, 0);
}

void scanColor() {
  unsigned long currentMillis = millis();
  if (currentMillis - stateStartTime >= scanInterval) {
    scanRight = !scanRight; // Switch direction
    stateStartTime = currentMillis;
    Serial.print(F("Switching scan direction to "));
    Serial.println(scanRight ? F("right") : F("left"));
  }
  if (scanRight) {
    turnRight();
  } else {
    turnLeft();
  }
  // Keep servo centered during scanning
  if (currentCameraAngle != 90) {
    currentCameraAngle = 90;
    cameraServo.write(currentCameraAngle);
    Serial.println(F("Servo centered for scanning"));
  }
}

void trackColor(int xCenter) {
  int error = xCenter - xCenterTarget;
  Serial.print(F("xCenter: "));
  Serial.print(xCenter);
  Serial.print(F(", Error: "));
  Serial.println(error);

  if (abs(error) > trackingDeadband) {
    if (error > 0) {
      currentCameraAngle -= trackingStep;
    } else {
      currentCameraAngle += trackingStep;
    }
    currentCameraAngle = constrain(currentCameraAngle, 70, 140);
    Serial.print(F("Tracking angle: "));
    Serial.println(currentCameraAngle);
    //cameraServo.write(currentCameraAngle);
  }
}

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
int lastButtonState = HIGH;
bool buttonProcessed = false;

void button() {
  int currentButtonState = digitalRead(buttonPin);
  if (currentButtonState != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentButtonState == LOW && !buttonProcessed) {
      buttonLock = (buttonLock + 1) % 2;
      buttonProcessed = true;
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      Serial.print(F("Button pressed, buttonLock: "));
      Serial.println(buttonLock);
    }
    if (currentButtonState == HIGH) {
      buttonProcessed = false;
    }
  }
  lastButtonState = currentButtonState;
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

// Structure to hold block data
struct Block {
  int xCenter;
  int yCenter;
  int width;
  int height;
  int ID;
  float score;
};

// Structure to hold cluster data
struct Cluster {
  float totalScore;
  float avgXCenter;
  float avgYCenter;
  float avgWidth;
  float avgHeight;
  int blockCount;
};

void loop() {
  button();

  if (buttonLock == 0) {
    stopMove();
    digitalWrite(ledPin, LOW);
    currentState = SCANNING;
    stateStartTime = millis();
    cameraServo.write(90);
    currentCameraAngle = 90;
    return;
  }

  if (!huskylens.request()) {
    Serial.println(F("Failed to request data from HuskyLens"));
    stopMove();
    digitalWrite(ledPin, LOW);
    if (currentState != RETREATING) {
      currentState = SCANNING;
      stateStartTime = millis();
    }
    if (currentState == SCANNING) {
      scanColor();
    }
    return;
  }

  Cluster bestCluster;
  bool validClusterDetected = false;
  bestCluster.totalScore = 0.0;
  bestCluster.blockCount = 0;

  if (huskylens.available()) {
    // Collect all valid blocks
    Block blocks[20];
    int blockCount = 0;

    while (huskylens.available() && blockCount < 20) {
      HUSKYLENSResult result = huskylens.read();
      if (result.command == COMMAND_RETURN_BLOCK && result.ID >= 1 && result.ID <= maxColorID) {
        // Filter small blocks
        if (result.width < 10 || result.height < 10) continue;
        blocks[blockCount].xCenter = result.xCenter;
        blocks[blockCount].yCenter = result.yCenter;
        blocks[blockCount].width = result.width;
        blocks[blockCount].height = result.height;
        blocks[blockCount].ID = result.ID;
        blocks[blockCount].score = colorWeights[result.ID];
        Serial.print(F("Block ID: "));
        Serial.print(result.ID);
        Serial.print(F(", xCenter: "));
        Serial.print(result.xCenter);
        Serial.print(F(", yCenter: "));
        Serial.print(result.yCenter);
        Serial.print(F(", Score: "));
        Serial.println(blocks[blockCount].score);
        blockCount++;
      }
    }

    if (blockCount > 0) {
      // Cluster blocks
      Cluster clusters[20];
      int clusterCount = 0;
      bool assigned[20] = {false};

      for (int i = 0; i < blockCount; i++) {
        if (assigned[i]) continue;

        clusters[clusterCount].totalScore = blocks[i].score;
        clusters[clusterCount].avgXCenter = blocks[i].xCenter;
        clusters[clusterCount].avgYCenter = blocks[i].yCenter;
        clusters[clusterCount].avgWidth = blocks[i].width;
        clusters[clusterCount].avgHeight = blocks[i].height;
        clusters[clusterCount].blockCount = 1;
        assigned[i] = true;

        for (int j = i + 1; j < blockCount; j++) {
          if (assigned[j]) continue;
          int dx = blocks[i].xCenter - blocks[j].xCenter;
          int dy = blocks[i].yCenter - blocks[j].yCenter;
          float distance = sqrt(dx * dx + dy * dy);
          if (distance <= proximityThreshold) {
            clusters[clusterCount].totalScore += blocks[j].score;
            clusters[clusterCount].avgXCenter = (clusters[clusterCount].avgXCenter * clusters[clusterCount].blockCount + blocks[j].xCenter) / (clusters[clusterCount].blockCount + 1);
            clusters[clusterCount].avgYCenter = (clusters[clusterCount].avgYCenter * clusters[clusterCount].blockCount + blocks[j].yCenter) / (clusters[clusterCount].blockCount + 1);
            clusters[clusterCount].avgWidth = (clusters[clusterCount].avgWidth * clusters[clusterCount].blockCount + blocks[j].width) / (clusters[clusterCount].blockCount + 1);
            clusters[clusterCount].avgHeight = (clusters[clusterCount].avgHeight * clusters[clusterCount].blockCount + blocks[j].height) / (clusters[clusterCount].blockCount + 1);
            clusters[clusterCount].blockCount++;
            assigned[j] = true;
          }
        }
        clusterCount++;
      }

      // Find best cluster
      int bestClusterIndex = -1;
      float highestScore = 0.0;
      for (int i = 0; i < clusterCount; i++) {
        if (clusters[i].totalScore > highestScore) {
          highestScore = clusters[i].totalScore;
          bestClusterIndex = i;
        }
      }

      if (bestClusterIndex >= 0) {
        bestCluster = clusters[bestClusterIndex];
        validClusterDetected = true;
        lastDetectionTime = millis();
        Serial.print(F("Best cluster: Score: "));
        Serial.print(bestCluster.totalScore);
        Serial.print(F(", avgXCenter: "));
        Serial.print(bestCluster.avgXCenter);
        Serial.print(F(", avgYCenter: "));
        Serial.print(bestCluster.avgYCenter);
        Serial.print(F(", avgWidth: "));
        Serial.print(bestCluster.avgWidth);
        Serial.print(F(", avgHeight: "));
        Serial.println(bestCluster.avgHeight);
      }
    }
  }

  // State machine
  unsigned long currentMillis = millis();
  int avgXCenter = (int)bestCluster.avgXCenter;
  int error = avgXCenter - xCenterTarget;
  int turnSpeed = constrain(abs(error) * turnSpeedFactor,  150, speed);

  switch (currentState) {
    case SCANNING:
      if (validClusterDetected) {
        Serial.println(F("Cluster detected, switching to CENTERING"));
        currentState = CENTERING;
        stateStartTime = currentMillis;
        stopMove();
      } else {
        scanColor();
      }
      break;

    case CENTERING:
      if (!validClusterDetected) {
        if (currentMillis - lastDetectionTime >= noDetectionTimeout) {
          Serial.println(F("No detection for timeout, switching to SCANNING"));
          currentState = SCANNING;
          stateStartTime = currentMillis;
          scanColor();
        }
        break;
      }

      // Blink LED
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);

      trackColor(avgXCenter);
      Serial.print(F("Centering: avgXCenter: "));
      Serial.print(avgXCenter);
      Serial.print(F(", Error: "));
      Serial.println(error);

      // Require stable centering for 500ms
      static unsigned long centeredStartTime = 0;
      if (abs(error) <= movementDeadband) {
        if (centeredStartTime == 0) {
          centeredStartTime = currentMillis;
        }
        if (currentMillis - centeredStartTime >= 500) {
          Serial.println(F("Cluster centered, switching to MOVING_FORWARD"));
          currentState = MOVING_FORWARD;
          stateStartTime = currentMillis;
          centeredStartTime = 0;
        }
      } else {
        centeredStartTime = 0; // Reset if not centered
        if (error > 0) {
          Serial.print(F("Cluster on right, turning right with speed: "));
          Serial.println(turnSpeed);
          //turnRight();
          Serial.println(F("Turning Right"));
          analogWrite(leftMotorPin1, turnSpeed);
          analogWrite(leftMotorPin2, 0);
          analogWrite(rightMotorPin1, turnSpeed);
          analogWrite(rightMotorPin2, 0);
        } else {
          Serial.print(F("Cluster on left, turning left with speed: "));
          Serial.println(turnSpeed);
          //turnLeft();
          Serial.println(F("Turning Left"));
          analogWrite(leftMotorPin1, 0);
          analogWrite(leftMotorPin2, turnSpeed);
          analogWrite(rightMotorPin1, 0);
          analogWrite(rightMotorPin2, turnSpeed);
        }
      }
      break;

    case MOVING_FORWARD:
      if (!validClusterDetected) {
        if (currentMillis - lastDetectionTime >= noDetectionTimeout) {
          Serial.println(F("No detection for timeout, switching to RETREATING"));
          currentState = RETREATING;
          stateStartTime = currentMillis;
          moveBackward();
        }
        break;
      }

      // Check for "hit" (cluster size too large)
      if (bestCluster.avgWidth >= hitSizeThreshold || bestCluster.avgHeight >= hitSizeThreshold) {
        Serial.println(F("Hit detected, switching to RETREATING"));
        currentState = RETREATING;
        stateStartTime = currentMillis;
        moveBackward();
        break;
      }

      // Blink LED
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);

      trackColor(avgXCenter);
      Serial.print(F("Moving forward: avgXCenter: "));
      Serial.print(avgXCenter);
      Serial.print(F(", Error: "));
      Serial.println(error);

      moveForward();
      break;

    case RETREATING:
      if (currentMillis - stateStartTime >= retreatDuration) {
        Serial.println(F("Retreat complete, switching to SCANNING"));
        currentState = SCANNING;
        stateStartTime = currentMillis;
        scanColor();
      } else {
        moveBackward();
      }
      break;
  }

  Serial.print("State: ");
  switch (currentState) {
    case SCANNING: Serial.println(F("SCANNING")); break;
    case CENTERING: Serial.println(F("CENTERING")); break;
    case MOVING_FORWARD: Serial.println(F("MOVING_FORWARD")); break;
    case RETREATING: Serial.println(F("RETREATING")); break;
  }
}