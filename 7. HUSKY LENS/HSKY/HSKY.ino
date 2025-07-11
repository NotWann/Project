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
  0.1   // ID 10 (e.g., shadowed.yellow)
};
const int maxColorID = 10; // Maximum valid color ID

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
      } else {
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
    cameraServo.write(currentCameraAngle);
    count_angle = currentCameraAngle;
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

void loop() {
  button();

  if (!huskylens.request()) {
    Serial.println(F("Failed to request data from HuskyLens"));
    stopMove();
    digitalWrite(ledPin, LOW);
    searchColor();
    return;
  }

  if (huskylens.available()) {
    float highestScore = 0.0;
    int bestXCenter = 0;
    int bestID = 0;
    bool validBlockDetected = false;

    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();
      if (result.command == COMMAND_RETURN_BLOCK && result.ID >= 1 && result.ID <= maxColorID) {
        // Calculate score based on weight
        float score = colorWeights[result.ID];
        Serial.print(F("Block ID: "));
        Serial.print(result.ID);
        Serial.print(F(", Weight: "));
        Serial.print(colorWeights[result.ID]);
        Serial.print(F(", Score: "));
        Serial.println(score);

        // Optional: Adjust score based on block size or centrality
        // Example: score *= (result.width * result.height) / 1000.0; // Favor larger blocks
        // Example: score *= (1.0 - abs(result.xCenter - xCenterTarget) / 160.0); // Favor centered blocks

        if (score > highestScore) {
          highestScore = score;
          bestXCenter = result.xCenter;
          bestID = result.ID;
          validBlockDetected = true;
        }
      }
    }

    if (validBlockDetected) {
      Serial.print(F("Best block ID: "));
      Serial.print(bestID);
      Serial.print(F(", Score: "));
      Serial.println(highestScore);

      trackColor(bestXCenter);

      int error = bestXCenter - xCenterTarget;
      const int movementDeadband = 30;
      const float turnSpeedFactor = 1.5;

      digitalWrite(ledPin, LOW);
      delay(100);
      digitalWrite(ledPin, HIGH);

      if (abs(error) <= movementDeadband) {
        Serial.println(F("Block centered, moving forward"));
        moveForward();
      } else {
        int turnSpeed = constrain(abs(error) * turnSpeedFactor, 110, speed);
        if (error > 0) {
          Serial.print(F("Block on right, turning right with speed: "));
          Serial.println(turnSpeed);
          analogWrite(leftMotorPin1, turnSpeed);
          analogWrite(leftMotorPin2, 0);
          analogWrite(rightMotorPin1, turnSpeed);
          analogWrite(rightMotorPin2, 0);
        } else {
          Serial.print(F("Block on left, turning left with speed: "));
          Serial.println(turnSpeed);
          analogWrite(leftMotorPin1, 0);
          analogWrite(leftMotorPin2, turnSpeed);
          analogWrite(rightMotorPin1, 0);
          analogWrite(rightMotorPin2, turnSpeed);
        }
      }
    } else {
      Serial.println(F("No valid yellow block detected."));
      stopMove();
      digitalWrite(ledPin, LOW);
      searchColor();
    }
  } else {
    Serial.println(F("No blocks detected."));
    stopMove();
    digitalWrite(ledPin, LOW);
    searchColor();
  }

  Serial.print("Button: ");
  Serial.println(buttonLock);
}