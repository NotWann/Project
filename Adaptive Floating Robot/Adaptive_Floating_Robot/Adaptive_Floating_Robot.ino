#include "HX710B.h"
#include <Wire.h>
#include "SoftwareSerial.h"
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

SoftwareSerial mySerial(17, 16); // RX, TX
LiquidCrystal_I2C lcd(0x27, 16, 2);

static const int servoPin = 12;
Servo servo1;
int lock_servo = 0;
int count_servo = 0;

const int DOUT = 26; // Sensor data pin
const int SCLK = 25; // Sensor clock pin
HX710B pressure_sensor;

float PSI_Cal = 0;
float KPI_1psi = 6.89476;
float KPI_Cal = 0;
int maintain_pressure = 12; // SETTING PRESSURE (MAX 40)

const int trigPin = 32;
const int echoPin = 27;
long duration;
int distance;

const int buttonPin = 23;
int buttonState = 0;
int flag_1 = 0;

const int pump = 13;
const int buzzer = 2;

const int propeller_1A = 4;
const int propeller_1B = 5;
const int propeller_2A = 18;
const int propeller_2B = 19;
const int propellerSpeed = 255;

String nmeaSentence = "";
float latitude = 0.0;
float longitude = 0.0;

void moveForward() {
  Serial.println(" FORWARD ");
  analogWrite(propeller_1A, 0);
  analogWrite(propeller_1B, propellerSpeed);
  analogWrite(propeller_2A, propellerSpeed);
  analogWrite(propeller_2B, 0);
}

void moveBackward() {
  Serial.println(" BACKWARD ");
  analogWrite(propeller_1A, propellerSpeed);
  analogWrite(propeller_1B, 0);
  analogWrite(propeller_2A, 0);
  analogWrite(propeller_2B, propellerSpeed);
}

void stopMove() {
  Serial.println(" STOP ");
  analogWrite(propeller_1A, 0);
  analogWrite(propeller_1B, 0);
  analogWrite(propeller_2A, 0);
  analogWrite(propeller_2B, 0);
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  servo1.attach(servoPin);
  lcd.init();
  lcd.backlight();
  pressure_sensor.begin(DOUT, SCLK);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pump, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(propeller_1A, OUTPUT);
  pinMode(propeller_1B, OUTPUT);
  pinMode(propeller_2A, OUTPUT);
  pinMode(propeller_2B, OUTPUT);
  stopMove();
}

void loop() {
  //gps_read();
  servo_rotated();
  //pressure_read();
  //jarak_atas();
  //delay(100);
}

void gps_read() {
  if (mySerial.available()) {
    char c = mySerial.read();
    if (c == '\n') {
      if (nmeaSentence.startsWith("$GPGGA")) {
        extractLatLong(nmeaSentence);
      }
      nmeaSentence = "";
    } else {
      nmeaSentence += c;
    }
  }
}

void extractLatLong(String nmea) {
  String rawLat = getField(nmea, 2);
  String latDir = getField(nmea, 3);
  String rawLon = getField(nmea, 4);
  String lonDir = getField(nmea, 5);

  latitude = convertToDecimal(rawLat);
  if (latDir == "S") latitude = -latitude;

  longitude = convertToDecimal(rawLon);
  if (lonDir == "W") longitude = -longitude;

  Serial.print("Latitude: ");
  Serial.println(latitude, 6);
  Serial.print("Longitude: ");
  Serial.println(longitude, 6);
}

float convertToDecimal(String rawValue) {
  float value = rawValue.toFloat();
  int degrees = (int)(value / 100);
  float minutes = value - (degrees * 100);
  return degrees + (minutes / 60);
}

String getField(String data, int index) {
  int commaCount = 0;
  int startIndex = 0;
  for (size_t i = 0; i < data.length(); i++) {
    if (data[i] == ',') {
      commaCount++;
      if (commaCount == index) {
        startIndex = i + 1;
      } else if (commaCount == index + 1) {
        return data.substring(startIndex, i);
      }
    }
  }
  return "";
}

void pressure_read() {
  if (pressure_sensor.is_ready()) {
    PSI_Cal = (pressure_sensor.mmHg() - 3.85);
    if (PSI_Cal < 0.05) {
      PSI_Cal = 0;
      KPI_Cal = 0;
    } else {
      PSI_Cal = PSI_Cal + 0.5;
      KPI_Cal = PSI_Cal * KPI_1psi;
    }
    Serial.print("PSI: ");
    Serial.println(PSI_Cal);
    Serial.print("Kilopascal: ");
    Serial.println(KPI_Cal);
    lcd.setCursor(0, 1);
    lcd.print("P.sure:");
    lcd.print(KPI_Cal);
    lcd.print(" Kpa ");
  }
}

void jarak_atas() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  Serial.print("Distance: ");
  Serial.println(distance);
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
  lcd.print(distance);
  lcd.print(" cm ");
}

void servo_rotated() {
  if (lock_servo == 0) {
    count_servo += 10;
    if (count_servo >= 180) {
      lock_servo = 1;
    }
  }
  if (lock_servo == 1) {
    count_servo -= 10;
    if (count_servo <= 0) {
      lock_servo = 0;
    }
  }
  servo1.write(count_servo);
}