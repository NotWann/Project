#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define TRIG_PIN 17
#define ECHO_PIN 16
#define SENSOR 27
#define SERVO_PIN 32  

char auth[] = "VdxmW2nfuzx9aTgsRGSHyFYF-BbTUz-F";
char ssid[] = "Mycest Solution_2.4GHz";
char pass[] = "mycest246813579";

char server[] = "prakitblog.com";
int port = 8181;

LiquidCrystal_I2C lcd(0x27, 20, 4);

Servo valveServo;
int servoAngle = 0;

long currentMillis = 0;           //Variable for flowrate sensor calculation
long previousMillis = 0;
int interval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;

int valveV1 = 12;
int valveV2 = 13;

int targetPercentage = 50;  // Default target level
bool valveControlEnabled = false;   // Switch state
bool valveState = false;
int waterFluc = 20;

unsigned long startTime = 0;
bool timerRunning = false;

int flag = 0;
int lock = 0;

int buzzerPin = 2;

void IRAM_ATTR pulseCounter() 
{
  pulseCount++;
}

// Switch control for valve (V6)
BLYNK_WRITE(V6)
{
  valveControlEnabled = param.asInt();
  Serial.print("Valve Control Switch: ");
  Serial.println(valveControlEnabled ? "ON" : "OFF");
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  if(valveControlEnabled == 1)
  {
    flag = 0;
    lock = 0;
  }
}

// Step H widget to set water level target (V5)
BLYNK_WRITE(V5) 
{
  targetPercentage = param.asInt();
  Serial.print("Target Water Level Set to: ");
  Serial.print(targetPercentage);
  Serial.println("%");
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

// Manual valve control button (V4)
BLYNK_WRITE(V4) 
{
  int buttonState = param.asInt();
  digitalWrite(valveV2, buttonState);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  if (buttonState == 1)
  {
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print("VALVE: ON");
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print("VALVE: OFF");
  }
} 

// Step H widget to control servo angle (V8)
BLYNK_WRITE(V8) 
{
  servoAngle = param.asInt();
  Serial.print("Servo Angle Set to: ");
  Serial.println(servoAngle);
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

void setup() 
{
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass, server, port);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(buzzerPin, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(valveV1, OUTPUT);
  pinMode(valveV2, OUTPUT);

  valveServo.attach(SERVO_PIN);
  valveServo.write(65);  

  pulseCount = 0;
  flowRate = 0.0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
}

void loop() 
{
  Blynk.run();

  // Measure water level
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.034) / 2;

  int percentage = map(distance, 0, 65, 100, 0);
  percentage = constrain(percentage, 0, 100);


  if (timerRunning) {
  unsigned long recordTime = millis() - startTime;
  int minutes = (recordTime / 1000) / 60;
  int seconds = (recordTime / 1000) % 60;
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", minutes, seconds);  
  Blynk.virtualWrite(V7, timeStr);  // Keep updating the timer widget (V7)
}

if (valveControlEnabled)
{
  if(flag == 0)
  {
    if (!timerRunning) 
    {  // Start timer only once when enabled
      startTime = millis();
      timerRunning = true;
      Serial.println("Timer started...");
    } 
  }

    if (!valveState && percentage >= targetPercentage)
   {
      valveServo.attach(SERVO_PIN);
      valveServo.write(65);  // Valve close
      if (lock == 0)
      {
       delay(3000);
       valveServo.detach(); 
       lock = 1;
      }
      valveState = true;
      timerRunning = false;
      Blynk.virtualWrite(V6, LOW);
      flag = 1;
    }
    if (valveState == false && percentage <= (targetPercentage - waterFluc)) 
    {
      valveServo.attach(SERVO_PIN);
      valveServo.write(servoAngle);  // Valve open based on set angle
        if (lock == 0)
      {
       delay(3000);
       valveServo.detach(); 
       lock = 1;
      }
      valveState = false;
     
    }
}
else 
{
  valveServo.attach(SERVO_PIN);
  valveServo.write(65);
    if (lock == 0)
      {
       delay(3000);
       valveServo.detach(); 
       lock = 1;
      }
  timerRunning = false;
  valveState = false;
}

  // Send water level to Blynk every 1 second
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) 
  {
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
  if (currentMillis - previousMillis > interval) 
  {
    pulse1Sec = pulseCount;
    pulseCount = 0;
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    Blynk.virtualWrite(V3, flowRate);
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));
    Serial.println(" L/min");

    delay(10);
  }

   // Send flow rate status to Blynk
  if (flowRate >= 1 && flowRate <= 15) 
  {
    Blynk.virtualWrite(V2, "LOW");
    Serial.println("LOW");

  } 
  else if (flowRate > 15 && flowRate <= 30) 
  {
    Blynk.virtualWrite(V2, "MEDIUM");
    Serial.println("MEDIUM");

  } 
  else if (flowRate > 30) 
  {
    Blynk.virtualWrite(V2, "HIGH");
    Serial.println("HIGH");

    //valve V1
    digitalWrite(valveV1, HIGH);
  }
  else 
  {
    digitalWrite(valveV1, LOW);
  }

  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print("LEVEL: ");
  lcd.print(percentage);
  lcd.print("% ");

  delay(10);
}
