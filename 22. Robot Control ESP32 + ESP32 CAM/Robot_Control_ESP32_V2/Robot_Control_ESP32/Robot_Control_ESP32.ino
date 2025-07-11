#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>

char auth[] = "ltXcwGqvpI2UCt3UmT33No1w6TaoLsHN";
char ssid[] = "abcd";
char pass[] = "123456789";

char server[] = "prakitblog.com";
int port = 8181;

const int motorA1_pin = 12;
const int motorA2_pin = 13;
const int motorB1_pin = 14;
const int motorB2_pin = 15;
int setSpeed = 100;

// PWM settings
const int PWM_CHANNEL_A1 = 0;  
const int PWM_CHANNEL_A2 = 1; 
const int PWM_CHANNEL_B1 = 2;  
const int PWM_CHANNEL_B2 = 3;  
const int PWM_FREQ = 1000;     // PWM frequency (1 kHz)
const int PWM_RESOLUTION = 8;  // 8-bit resolution (0–255)

const int buzzer_pin = 2;

int lock_forward = 0;
int lock_reverse = 0;
int lock_turnright = 0;
int lock_turnleft = 0;

void moveForward() {
  Serial.println("FORWARD");
  ledcWrite(PWM_CHANNEL_A1, setSpeed);
  ledcWrite(PWM_CHANNEL_A2, 0);
  ledcWrite(PWM_CHANNEL_B1, setSpeed);
  ledcWrite(PWM_CHANNEL_B2, 0);
}

void reverse() {
  Serial.println("REVERSE");
  ledcWrite(PWM_CHANNEL_A1, 0);
  ledcWrite(PWM_CHANNEL_A2, setSpeed);
  ledcWrite(PWM_CHANNEL_B1, 0);
  ledcWrite(PWM_CHANNEL_B2, setSpeed);
}

void turnRight() {
  Serial.println("RIGHT");
  ledcWrite(PWM_CHANNEL_A1, setSpeed);
  ledcWrite(PWM_CHANNEL_A2, 0);
  ledcWrite(PWM_CHANNEL_B1, 0);
  ledcWrite(PWM_CHANNEL_B2, setSpeed);
}

void turnLeft() {
  Serial.println("LEFT");
  ledcWrite(PWM_CHANNEL_A1, 0);
  ledcWrite(PWM_CHANNEL_A2, setSpeed);
  ledcWrite(PWM_CHANNEL_B1, setSpeed);
  ledcWrite(PWM_CHANNEL_B2, 0);
}

void stop() {
  Serial.println("STOP");
  ledcWrite(PWM_CHANNEL_A1, 0);
  ledcWrite(PWM_CHANNEL_A2, 0);
  ledcWrite(PWM_CHANNEL_B1, 0);
  ledcWrite(PWM_CHANNEL_B2, 0);
}

BLYNK_WRITE(V1) {
  lock_forward = param.asInt();
  if (lock_forward == 1) 
  {
    moveForward();
    digitalWrite(buzzer_pin, HIGH);
  }
  else if (lock_forward == 0) 
  {
    stop();
    digitalWrite(buzzer_pin, LOW);
  }
}

BLYNK_WRITE(V2) {
  lock_reverse = param.asInt();
  if (lock_reverse == 1) 
  {
    reverse();
    digitalWrite(buzzer_pin, HIGH);
  }
  else if (lock_reverse == 0) 
  {
    stop();
    digitalWrite(buzzer_pin, LOW);
  }
}

BLYNK_WRITE(V3) {
  lock_turnright = param.asInt();
  if (lock_turnright == 1) 
  {
    turnRight();
    digitalWrite(buzzer_pin, HIGH);
  }
  else if (lock_turnright == 0) 
  {
    stop();
    digitalWrite(buzzer_pin, LOW);
  }
}

BLYNK_WRITE(V4) {
  lock_turnleft = param.asInt();
  if (lock_turnleft == 1) 
  {
    turnLeft();
    digitalWrite(buzzer_pin, HIGH);
  }
  else if (lock_turnleft == 0) 
  {
    stop();
    digitalWrite(buzzer_pin, LOW);
  }
}

BLYNK_WRITE(V0) 
{
  setSpeed = constrain(param.asInt(), 70, 255);
  Serial.print("Set Speed: ");
  Serial.println(setSpeed);
  digitalWrite(buzzer_pin, HIGH);
  delay(100);
  digitalWrite(buzzer_pin, LOW);
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
    for (int i = 0; i < 2; i++)
    {
      digitalWrite(buzzer_pin, HIGH);
      delay(100);
      digitalWrite(buzzer_pin, LOW);
    }
  }
  else 
  {
    Serial.println("Blynk connection failed!");
  }
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass, server, port);
  // Configure PWM channels
  ledcSetup(PWM_CHANNEL_A1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_A2, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B2, PWM_FREQ, PWM_RESOLUTION);

  // Attach PWM pins
  ledcAttachPin(motorA1_pin, PWM_CHANNEL_A1);
  ledcAttachPin(motorA2_pin, PWM_CHANNEL_A2);
  ledcAttachPin(motorB1_pin, PWM_CHANNEL_B1);
  ledcAttachPin(motorB2_pin, PWM_CHANNEL_B2);

  pinMode(buzzer_pin, OUTPUT);
}

void loop() {
  if (!Blynk.connected()) 
  {
    Serial.println("Blynk disconnected! Reconnecting...");
    connectToBlynk();
  }
  Blynk.run();

  delay(10);
}
