#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>  

#define TRIG_PIN 17
#define ECHO_PIN 16
#define LED 2
#define SENSOR 27
#define SERVO_PIN 32  

char auth[] = "dievkxq0PgsVns1Xa59RPVQccRZyb2dw";
char ssid[] = "Siuuuu";
char pass[] = "abc9519992";

char server[] = "blynk.tk";
int port = 8080;

LiquidCrystal_I2C lcd(0x27, 20, 4);
Servo valveServo; 

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;

int valveV1 = 12;
int valveV2 = 13;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// Step H widget to set water level target
BLYNK_WRITE(V6) {
  targetPercentage = param.asInt();
  Serial.print("Target Water Level Set to: ");
  Serial.print(targetPercentage);
  Serial.println("%");
}

// Switch control for valve servo
BLYNK_WRITE(V5) {
  valveControlEnabled = param.asInt();
  Serial.print("Valve Control Switch: ");
  Serial.println(valveControlEnabled ? "ON" : "OFF");
}

// button for valve V3
BLYNK_WRITE(V4) {
  int buttonState = param.asInt();  // Read the button state (0 or 1)
  
  if (buttonState == 1) {
    digitalWrite(valveV2, HIGH);
  } else {
    digitalWrite(valveV2, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass, server, port);
  lcd.init();
  lcd.backlight();

  pinMode(LED, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(valveV1, OUTPUT);
  pinMode(valveV2, OUTPUT);

  // Initialize the servo
  valveServo.attach(SERVO_PIN);
  valveServo.write(0);  // Start with valve fully closed

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
}

void loop() {
  Blynk.run();

  // Measure distance
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.034) / 2;

  // Convert distance to percentage
  float percentage = map(distance, 0, 75, 100, 0);
  percentage = constrain(percentage, 0, 100);

  // Servo Control Based on Percentage
  int valveAngle;
  if (percentage >= 90) {
    valveAngle = 0;  // Fully Closed
  } else if (percentage <= 10) {
    valveAngle = 180;  // Fully Open
  } else {
    valveAngle = map(percentage, 10, 90, 180, 0);  // Gradual opening
  }
  valveServo.write(valveAngle);

  // Send data to Blynk every 1 second
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    Blynk.virtualWrite(V1, percentage);
    Serial.print("Sending to Blynk: ");
    Serial.println(percentage);
  }

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print("cm, Percentage: ");
  Serial.print(percentage);
  Serial.print("%, Valve Angle: ");
  Serial.println(valveAngle);

  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    pulse1Sec = pulseCount;
    pulseCount = 0;
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    Blynk.virtualWrite(V3, flowRate);
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));
    Serial.println(" L/min");

    delay(500);
  }

   // Send flow rate status to Blynk
  if (flowRate >= 1 && flowRate <= 15) {
    Blynk.virtualWrite(V2, "LOW");
    Serial.println("LOW");

  } else if (flowRate > 15 && flowRate <= 30) {
    Blynk.virtualWrite(V2, "MEDIUM");
    Serial.println("MEDIUM");

  } else if (flowRate > 30) {
    Blynk.virtualWrite(V2, "HIGH");
    Serial.println("HIGH");

    //valve V1
    digitalWrite(valveV1, HIGH);
  } else {
    digitalWrite(valveV1, LOW);
  }

  BLYNK_WRITE(V4);
  
  // Update LCD display
  lcd.setCursor(0, 0);
  lcd.print("Percentage: ");
  lcd.print(percentage);
  lcd.print("%");

  delay(500);
}
