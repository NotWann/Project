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
const float Kp_camera = 0.1;
const float Kp_steer = 0.15;
const int screenCenterX = 160;
const int neutralAngle = 90;
const int maxAngleStep = 5; // Max 5° per 50ms loop for smoothness
const unsigned long motorDelay = 500; // Delay motor start by 500ms

int motor1Pin1 = 18;
int motor1Pin2 = 19;

int count_angle = neutralAngle; // Start at neutral
bool searchForward = true; // Oscillation direction
unsigned long previousSearchMillis = 0;
const long searchInterval = 100; // Update every 100ms for smoothness
const int searchStep = 5; // 5° steps
bool locked = false; // Locking state
int lastCameraAngle = neutralAngle; // Last known angle
int lastXCenter = screenCenterX; // Last known xCenter
unsigned long lastDetectionMillis = 0; // Time of last detection
const long searchTimeout = 2000; // Search for 2s after losing color
const int searchRange = 45; // Search ±45° from last angle
static int currentCameraAngle = neutralAngle;
static int currentWheelAngle = neutralAngle;
static bool motorStarted = false; // Track motor state
static unsigned long detectionStartMillis = 0; // Motor delay timer

void moveForward() {
  if (!motorStarted) {
    Serial.println(F("Moving Forward"));
    analogWrite(motor1Pin1, 110);
    analogWrite(motor1Pin2, 0);
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
  }
}

void printResult(HUSKYLENSResult result);

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
      Serial.println(String() + F("Targeted search at angle: ") + searchAngle);
    } else {
      locked = false;
      updateSearchAngle(); // Oscillate to find color
      wheelServo.write(neutralAngle);
      stopMove();
      digitalWrite(ledPin, LOW);
    }
  } else if (!huskylens.isLearned()) {
    Serial.println(F("No colors learned. Press learn button on HuskyLens to learn a color!"));
    locked = false;
    updateSearchAngle();
    wheelServo.write(neutralAngle);
    stopMove();
    digitalWrite(ledPin, LOW);
  } else if (!huskylens.available()) {
    Serial.println(F("No color detected on screen!"));
    if (locked && (millis() - lastDetectionMillis < searchTimeout)) {
      int searchAngle = lastCameraAngle + (lastXCenter > screenCenterX ? -searchRange : searchRange);
      searchAngle = constrain(searchAngle, 10, 170);
      cameraServo.write(searchAngle);
      Serial.println(String() + F("Targeted search at angle: ") + searchAngle);
    } else {
      locked = false;
      updateSearchAngle();
      wheelServo.write(neutralAngle);
      stopMove();
      digitalWrite(ledPin, LOW);
    }
  } else {
    Serial.println(F("###########"));
    HUSKYLENSResult result = huskylens.read();
    printResult(result);
  }
  delay(50);
}

void printResult(HUSKYLENSResult result) {
  if (result.command == COMMAND_RETURN_BLOCK && result.ID >= 1 && result.ID <= 6) {
    locked = true;
    lastDetectionMillis = millis();
    lastXCenter = result.xCenter;
    
    if (detectionStartMillis == 0) {
      detectionStartMillis = millis();
    }
    
    Serial.println(String() + F("Yellow Block: xCenter=") + result.xCenter + 
                  F(", yCenter=") + result.yCenter + F(", width=") + result.width + 
                  F(", height=") + result.height + F(", ID=") + result.ID);
    
    // Calculate error (pixels from center)
    int error = result.xCenter - screenCenterX;
    
    // Camera servo: Non-blocking update
    float cameraAngleChange = Kp_camera * error;
    int targetCameraAngle = constrain(currentCameraAngle - cameraAngleChange, 10, 170);
    int cameraStep = constrain(targetCameraAngle - currentCameraAngle, -maxAngleStep, maxAngleStep);
    currentCameraAngle += cameraStep;
    cameraServo.write(currentCameraAngle);
    lastCameraAngle = currentCameraAngle;
    
    // Wheel servo: Non-blocking update
    float wheelAngleChange = Kp_steer * error;
    int targetWheelAngle = constrain(currentWheelAngle + wheelAngleChange, 40, 120);
    int wheelStep = constrain(targetWheelAngle - currentWheelAngle, -maxAngleStep, maxAngleStep);
    currentWheelAngle += wheelStep;
    wheelServo.write(currentWheelAngle);
    
    digitalWrite(ledPin, HIGH);
    Serial.println(String() + F("Yellow detected! Error: ") + error + 
                  F(" pixels, Camera angle: ") + currentCameraAngle + 
                  F(" deg, Wheel angle: ") + currentWheelAngle + F(" deg"));
    
    if (millis() - detectionStartMillis >= motorDelay) {
      moveForward();
    }
  } else {
    Serial.println(F("No yellow or unknown object!"));
    detectionStartMillis = 0;
    if (locked && (millis() - lastDetectionMillis < searchTimeout)) {
      int searchAngle = lastCameraAngle + (lastXCenter > screenCenterX ? -searchRange : searchRange);
      searchAngle = constrain(searchAngle, 10, 170);
      cameraServo.write(searchAngle);
      Serial.println(String() + F("Targeted search at angle: ") + searchAngle);
    } else {
      locked = false;
      updateSearchAngle();
      wheelServo.write(neutralAngle);
      currentCameraAngle = neutralAngle;
      currentWheelAngle = neutralAngle;
      stopMove();
      digitalWrite(ledPin, LOW);
    }
  }
}