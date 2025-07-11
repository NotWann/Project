#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "max6675.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

char auth[] = "5ZdnQXAIwXPsJJDOU91xR6y07_z9bOgv"; // Auth token
char ssid[] = "abcd"; // WiFi ssid
char pass[] = "123456789"; // WiFi password

char server[] = "prakitblog.com"; // Custom server
int port = 8181;  // port can be different

const int thermoDO = 16;
const int thermoCS = 17;
const int thermoCLK = 18;
int lock_temp = 0;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

const int greenPin = 27;    // Green button pin
const int redPin = 32;      // Red button pin
const int ledPin = 2;       // Optional LED pin for output
int lastGreenState = HIGH;  // Previous state of green button
int lastRedState = HIGH;    // Previous state of red button
int count_temp = 0;         // Counter variable
const unsigned long debounceDelay = 50; // Debounce time in milliseconds
unsigned long lastGreenDebounceTime = 0; // Last debounce time for green button
unsigned long lastRedDebounceTime = 0;   // Last debounce time for red button
int greenButtonState;       // Current debounced state of green button
int redButtonState;         // Current debounced state of red button

const unsigned long tempReadInterval = 275; // Minimum interval for temp reading (ms)
unsigned long lastTempReadTime = 0; // Last time temperature was read
float temp = 0.00;

const int relayPin = 33;

BLYNK_WRITE(V2)
{
  count_temp = param.asInt();
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
}

void greenButton() {
  int currentGreenState = digitalRead(greenPin); // Read green button state

  // Check if green button state changed
  if (currentGreenState != lastGreenState) {
    lastGreenDebounceTime = millis(); // Reset debounce timer
  }

  // If debounce period has passed
  if ((millis() - lastGreenDebounceTime) > debounceDelay) {
    // If button state has changed
    if (currentGreenState != greenButtonState) {
      greenButtonState = currentGreenState;
      // Increment counter when green button is pressed (LOW due to INPUT_PULLUP)
      if (greenButtonState == LOW) {
        count_temp++;
        Serial.print("Count: ");
        Serial.println(count_temp);
        digitalWrite(ledPin, HIGH); // Turn LED on briefly
        delay(100);                 // Brief LED flash
        digitalWrite(ledPin, LOW);
      }
    }
  }

  lastGreenState = currentGreenState; // Save reading for next loop
}

void redButton() {
  int currentRedState = digitalRead(redPin); // Read red button state

  // Check if red button state changed
  if (currentRedState != lastRedState) {
    lastRedDebounceTime = millis(); // Reset debounce timer
  }

  // If debounce period has passed
  if ((millis() - lastRedDebounceTime) > debounceDelay) {
    // If button state has changed
    if (currentRedState != redButtonState) {
      redButtonState = currentRedState;
      // Decrement counter when red button is pressed (LOW due to INPUT_PULLUP)
      if (redButtonState == LOW) {
        count_temp--;
        Serial.print("Count: ");
        Serial.println(count_temp);
        digitalWrite(ledPin, HIGH); // Turn LED on briefly
        delay(100);                 // Brief LED flash
        digitalWrite(ledPin, LOW);
      }
    }
  }

  lastRedState = currentRedState; // Save reading for next loop
}

void connectToBlynk()
{
  unsigned long start = millis();
  while (!Blynk.connected() && millis() - start < 10000)  // 10 seconds timeout
  {
    Blynk.run();
    delay(1000);
  }
  if (Blynk.connected()) 
  {
    Serial.println("Blynk Connected!");
  }
  else 
  {
    Serial.println("Blynk connection failed!");
  }
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(greenPin, INPUT_PULLUP);
  pinMode(redPin, INPUT_PULLUP);
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  Blynk.begin(auth, ssid, pass, server, port);

  Serial.println("MAX6675 test");
  // Wait for MAX chip to stabilize
  delay(500);
}

void loop() {
  if (!Blynk.connected()) 
    {
      Serial.println("Blynk disconnected! Reconnecting...");
      connectToBlynk();
    }
  Blynk.run();

  greenButton(); // Check green button
  redButton();   // Check red button

  // Read temperature only if 275ms have passed since last read
  if (millis() - lastTempReadTime >= tempReadInterval) {

    temp = thermocouple.readCelsius();

    Serial.print("C = "); 
    Serial.println(temp);

    Blynk.virtualWrite(V0, temp);
    Blynk.virtualWrite(V1, count_temp);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: "); 
    lcd.print(temp); 
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Set Temp: "); 
    lcd.print(count_temp); 
    lcd.print("C");

    lastTempReadTime = millis(); // Update last read time
  }

  if (temp >= count_temp && lock_temp == 0)
  {
    digitalWrite(relayPin, HIGH);
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    lock_temp = 1;
  }
  else if (temp < count_temp && lock_temp == 1)
  {
    digitalWrite(relayPin, LOW);
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    lock_temp = 0;
  }
}