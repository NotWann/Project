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

BlynkTimer timer;

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;

int valveV1 = 12;
int valveV2 = 13;

int targetPercentage = 50;  // Default target level
bool valveControlEnabled = false;  // Switch state

unsigned long startTime = 0;
bool timerRunning = false;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// Step H widget to set water level target (V5)
BLYNK_WRITE(V5) {
  targetPercentage = param.asInt();
  Serial.print("Target Water Level Set to: ");
  Serial.print(targetPercentage);
  Serial.println("%");
}

// Switch control for valve (V6)
BLYNK_WRITE(V6) {
  valveControlEnabled = param.asInt();
  Serial.print("Valve Control Switch: ");
  Serial.println(valveControlEnabled ? "ON" : "OFF");
}

// Manual valve control button (V4)
BLYNK_WRITE(V4) {
  int buttonState = param.asInt();
  digitalWrite(valveV2, buttonState);
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

  valveServo.attach(SERVO_PIN);
  valveServo.write(0);  // Start with valve fully closed

  pulseCount = 0;
  flowRate = 0.0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
}

void loop() {
  Blynk.run();

  // Measure water level
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.034) / 2;

  float percentage = map(distance, 0, 75, 100, 0);
  percentage = constrain(percentage, 0, 100);

  // Valve Control Based on Step H Widget and Switch
  if (valveControlEnabled) {
    if (percentage <= targetPercentage) {
      if (!timerRunning) {
        startTime = millis();
        timerRunning = true;
        Serial.println("Timer started...");
      }
      valveServo.write(180);  // Fully opened
    } if (percentage >= targetPercentage) {
        if (timerRunning) {
          float recordTime = millis() - startTime;  
          Blynk.virtualWrite(V7, (recordTime / 1000) / 60);     //convert to minute
          Serial.print((recordTime / 1000) / 60);
          Serial.println(" min.");
          timerRunning = false;
        }
      valveServo.write(0); //Fully closed
    }
  } else {
      Blynk.virtualWrite(V7, "0");
      valveServo.write(0);
      timerRunning = false;
  }

  // Send water level to Blynk every 1 second
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    Blynk.virtualWrite(V1, percentage);
    Serial.print("Water Level: ");
    Serial.print(percentage);
    Serial.println("%");
  }

  Serial.print("Target: ");
  Serial.print(targetPercentage);
  Serial.print("%, Valve Angle: ");
  Serial.println(valveServo.read());

  // Flow rate calculations
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

    delay(5);
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

  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print("Level: ");
  lcd.print(percentage);
  lcd.print("% ");
  lcd.setCursor(0, 1);
  lcd.print("Target: ");
  lcd.print(targetPercentage);
  lcd.print("%");

  delay(10);
}
