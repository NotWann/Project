#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define TRIG_PIN 17
#define ECHO_PIN 16
#define LED 2
#define SENSOR  27

char auth[] = "dievkxq0PgsVns1Xa59RPVQccRZyb2dw";
char ssid[] = "Siuuuu";
char pass[] = "abc9519992";

char server[] = "blynk.tk";
int port = 8080;

LiquidCrystal_I2C lcd(0x27, 20, 4);

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

void setup()
{
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass, server, port);
  lcd.init();
  lcd.backlight();
  
  pinMode(LED, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
}

void loop()
{
  Blynk.run();
 
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);

  float distance = (duration * 0.034) / 2;

  float percentage = map(distance, 0, 75, 100, 0);
  percentage = constrain(percentage, 0, 100);

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) { // Send every 1 second
    lastUpdate = millis();
 
    Blynk.virtualWrite(V1, percentage);
    Serial.print("Sending to Blynk: ");
    Serial.println(percentage);
  }

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println("cm");

  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    
    pulse1Sec = pulseCount;
    pulseCount = 0;

  flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

  Serial.print("Flow rate: ");
  Serial.print(int(flowRate));  
  Serial.println("L/min");
  delay(500);
  }

  if (flowRate >= 1 && flowRate <= 15){
    Blynk.virtualWrite(V2, "LOW");
    Serial.println(" ");
    Serial.println("LOW");
  } 
  else if (flowRate > 15 && flowRate <=30 ){
    Blynk.virtualWrite(V2, "MEDIUM");
    Serial.println(" ");
    Serial.println("MEDIUM");

  } 
  else if (flowRate > 30){
    Blynk.virtualWrite(V2, "HIGH");
    Serial.println(" ");
    Serial.println("HIGH");
  }
  
  lcd.setCursor(0, 0);
  lcd.print("Percentage: ");
  lcd.print(percentage);
  lcd.print("%");

  delay(500); 
}
