#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// I2C LCD configuration
#define I2C_SDA 27
#define I2C_SCL 32
LiquidCrystal_I2C lcd(0x27, 16, 2);

char auth[] = "pBA-a97XSXFTYG1-3J6Z8m81O_XXKcha"; // Auth token
char ssid[] = "abcd"; // WiFi ssid
char pass[] = "123456789"; // WiFi password

char server[] = "prakitblog.com"; // Custom server
int port = 8181;  // port can be different

const int motorPinA1 = 18;  
const int motorPinA2 = 19;  
//int speed = 255;           

const int encoderPin = 2;   

const int greenLedPin = 23;
const int yellowLedPin = 26;
const int redLedPin = 25;

const int buzzerPin = 4;

int setMotorSpeed = 100;
int motorSwitch = 0;

// Encoder variables
volatile unsigned int counter = 0; // Pulse counter (volatile for interrupt)
int rpm = 0;  
int trueRpm = 0;                    // Rotations per minute
const int slotsPerRevolution = 12; // Adjust based on your encoder disc

BLYNK_WRITE(V1)
{
  setMotorSpeed = param.asInt();
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

BLYNK_WRITE(V2)
{
  motorSwitch = param.asInt();
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
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

// Motor control function
void on_Motor() 
{
  analogWrite(motorPinA1, setMotorSpeed);
  analogWrite(motorPinA2, 0);
}
void off_Motor() 
{
  analogWrite(motorPinA1, 0);
  analogWrite(motorPinA2, 0);
}

// Interrupt service routine to count pulses
void IRAM_ATTR countPulse() {
  counter++; // Increment pulse counter
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  // Initialize pins
  pinMode(motorPinA1, OUTPUT);
  pinMode(motorPinA2, OUTPUT);
  pinMode(encoderPin, INPUT_PULLUP); 
  pinMode(greenLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Blynk.begin(auth, ssid, pass, server, port);  // Initialize blynk config

  lcd.init();
  lcd.backlight();
  lcd.clear();

  attachInterrupt(digitalPinToInterrupt(encoderPin), countPulse, RISING);

}

void loop() {
  if (!Blynk.connected()) 
    {
      Serial.println("Blynk disconnected! Reconnecting...");
      connectToBlynk();
    }
  Blynk.run();

  static unsigned long previousMillis = 0;
  const long interval = 1000; // Calculate RPM every 1 second

  if (motorSwitch == 1)
  {
    on_Motor();
  }
  else if (motorSwitch == 0)
  {
    off_Motor();
  }
  

  // Check if 1 second has passed
  if (millis() - previousMillis >= interval) {
    // Calculate RPM: (pulses / slots per revolution) * (60 seconds / interval)
    noInterrupts(); // Disable interrupts briefly to read counter safely
    rpm = (counter / slotsPerRevolution) * 60;
    trueRpm = map(rpm, 0, 1680, 0, 960);
    counter = 0; // Reset counter
    interrupts(); // Re-enable interrupts
    previousMillis += interval;

    Blynk.virtualWrite(V0, trueRpm);

    Serial.print("Speed: ");
    Serial.print(rpm);
    Serial.println(" RPM");
    Serial.print("True Speed: ");
    Serial.print(trueRpm);
    Serial.println(" RPM");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Speed: "); lcd.print(trueRpm); lcd.print(" RPM");
    lcd.setCursor(0, 1);
    lcd.print("Motor Speed: "); lcd.print(setMotorSpeed);

    if (trueRpm <= 500) 
    {
      Serial.println("SLOW");
      digitalWrite(greenLedPin, LOW);
      digitalWrite(yellowLedPin, LOW);
      digitalWrite(redLedPin, HIGH);
    }
    else if (trueRpm > 500 && trueRpm <= 700) 
    {
      Serial.println("MEDIUM");
      digitalWrite(greenLedPin, LOW);
      digitalWrite(yellowLedPin, HIGH);
      digitalWrite(redLedPin, LOW);
    }
    else if (trueRpm > 700 && trueRpm <= 1000) 
    {
      Serial.println("FAST");
      digitalWrite(greenLedPin, HIGH);
      digitalWrite(yellowLedPin, LOW);
      digitalWrite(redLedPin, LOW);
    }
  }
}