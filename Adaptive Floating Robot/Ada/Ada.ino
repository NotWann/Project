#include "HX710B.h"
#include <Wire.h>
#include "SoftwareSerial.h"
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

SoftwareSerial gpsSerial(17, 16);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myservo;
HX710B pressure_sensor;

//Motor propeller 
const int propeller_1A = 4;  //L
const int propeller_1B = 5;  //L
const int propeller_2A = 18; //R
const int propeller_2B = 19; //R
const int propellerSpeed = 255;

//Servo
const int servoPin = 15;
int lock_servo = 0;
int count_servo = 0;

//Ultrasonic
const int trigPin = 32;
const int echoPin = 27;
int distance;

//Pressure sensor
const int DOUT = 26;  //pressure sensor data pin
const int SCLK = 25;  //pressure sensor clock pin
float PSI_Cal = 0;
float KPI_1psi = 6.89476;
float KPI_Cal = 0;
int maintain_pressure = 12; // SETTING PRESSURRE (MAX 40)

//Air Pump
const int pump = 13;

//Push button
const int buttonPin = 23;
int flag_button = 0;

//Buzzer
const int buzzerPin = 2;

//GPS
String nmeaSentence = "";
float latitude = 0.0;
float longitude = 0.0;

//
void moveForward() 
{
  Serial.println(" FORWARD ");
  digitalWrite(propeller_1A, 0);
  digitalWrite(propeller_1B, propellerSpeed);
  digitalWrite(propeller_2A, propellerSpeed);
  digitalWrite(propeller_2B, 0);
}

void moveBackward() 
{
  Serial.println(" BACKWARD ");
  digitalWrite(propeller_1A, propellerSpeed);
  digitalWrite(propeller_1B, 0);
  digitalWrite(propeller_2A, 0);
  digitalWrite(propeller_2B, propellerSpeed);
}

void stopMove() 
{
  Serial.println(" STOP ");
  digitalWrite(propeller_1A, 0);
  digitalWrite(propeller_1B, 0);
  digitalWrite(propeller_2A, 0);
  digitalWrite(propeller_2B, 0);
}

//
void servoRotated() 
{
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastUpdate >= 100) 
  { // Update every 20ms
    if (lock_servo == 0) 
    {
      count_servo += 10;
      if (count_servo >= 180) 
      {
        lock_servo = 1;
      }
    }
    if (lock_servo == 1) 
    {
      count_servo -= 10;
      if (count_servo <= 0) 
      {
        lock_servo = 0;
      }
    }
    myservo.write(count_servo);
    Serial.print("Servo Angle: "); Serial.println(count_servo);
    Serial.print("Lock: "); Serial.println(lock_servo);
    lastUpdate = currentTime;
  }
}

void jarak_atas() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration;
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Distance: "); Serial.println(distance);
  lcd.setCursor(0, 0);
  lcd.print("Distance:"); lcd.print(distance); lcd.print(" cm ");
  delay(100);
}

void pressure_read()
{
  /*if (pressure_sensor.is_ready()) 
  {
    Serial.print("Pascal: ");
    Serial.print(pressure_sensor.pascal());
    Serial.print("  ATM: ");
    Serial.print(pressure_sensor.atm());
    Serial.print("  mmHg: ");
    Serial.print(pressure_sensor.mmHg()-3.85);
    Serial.print("  PSI: ");
    Serial.println(pressure_sensor.psi());
    */

    PSI_Cal = (pressure_sensor.mmHg()-3.85);

    if(PSI_Cal < 0.05)
    {
      PSI_Cal = 0;
      KPI_Cal = 0;
    }
    else
    {
      PSI_Cal = PSI_Cal + 0.5;
      KPI_Cal = PSI_Cal * KPI_1psi;
    }

    Serial.print("PSI: "); Serial.println(PSI_Cal);
    Serial.print("  Kilopascal: "); Serial.println(KPI_Cal);

    lcd.setCursor(0, 1);
    lcd.print("P.sure:"); lcd.print(KPI_Cal); lcd.print(" Kpa ");  
}

void sound()
{
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

//
void setup()
{
  Serial.begin(115200);
  gpsSerial.begin(9600);
  
  myservo.attach(servoPin);

  pressure_sensor.begin(DOUT, SCLK);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pump, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(propeller_1A, OUTPUT);
  pinMode(propeller_1B, OUTPUT);
  pinMode(propeller_2A, OUTPUT);
  pinMode(propeller_2B, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  stopMove();
  
}

void loop() 
{
  gps_read();
  jarak_atas();
  pressure_read();

  unsigned long lastButtonPress = 0;
  int buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && (millis() - lastButtonPress >= 200)) 
  {
    if (flag_button == 0) 
    {
      sound();
      moveForward();
      flag_button = 1;
    } 
    else if (flag_button == 1) 
    {
      sound();
      //stopMove();
      flag_button = 0;
    }
    lastButtonPress = millis();
  }

  if (distance <= 20) 
  {
    stopMove();
    servoRotated();
    flag_button = 0;
  } 
  else if (distance != 20)
  {
    myservo.write(90);
  }
}

//GPS extract location
void gps_read()
{
  if (Serial1.available()) {
    char c = Serial1.read();  // Read one character
    if (c == '\n') {          // End of sentence
      if (nmeaSentence.startsWith("$GPGGA")) {
        extractLatLong(nmeaSentence); // Extract data if GPGGA sentence
      }
      nmeaSentence = ""; // Clear buffer for next sentence
    } else {
      nmeaSentence += c; // Append character to buffer
    }
  }
}

//gps function calculation
void extractLatLong(String nmea) {
  // Example GPGGA sentence: $GPGGA,123456.78,3723.2475,N,12202.3578,W,1,08,0.9,545.4,M,46.9,M,,*47
  String rawLat = getField(nmea, 2);  // Latitude value (raw)
  String latDir = getField(nmea, 3); // Latitude direction (N/S)
  String rawLon = getField(nmea, 4); // Longitude value (raw)
  String lonDir = getField(nmea, 5); // Longitude direction (E/W)

  // Convert raw latitude and longitude to decimal degrees
  latitude = convertToDecimal(rawLat);
  if (latDir == "S") latitude = -latitude; // South is negative

  longitude = convertToDecimal(rawLon);
  if (lonDir == "W") longitude = -longitude; // West is negative

  // Display latitude and longitude in decimal degrees
  Serial.print("Latitude: ");
  Serial.println(latitude, 6); // Print with 6 decimal places
  Serial.print("Longitude: ");
  Serial.println(longitude, 6); // Print with 6 decimal places
}

float convertToDecimal(String rawValue) {
  // Convert DDMM.MMMM to decimal degrees
  float value = rawValue.toFloat();
  int degrees = (int)(value / 100); // Extract degrees (integer part)
  float minutes = value - (degrees * 100); // Extract minutes
  return degrees + (minutes / 60); // Combine degrees and minutes
}

String getField(String data, int index) {
  // Extract specific field from NMEA sentence by commas
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
  return ""; // Return empty string if field not found
}