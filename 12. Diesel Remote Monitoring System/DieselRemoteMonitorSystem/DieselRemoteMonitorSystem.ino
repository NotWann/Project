#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>

char auth[] = "cSt40eiU8o1xbX_zgVONWENa7lFi-CHl"; // Auth token
char ssid[] = "abcd"; // WiFi ssid
char pass[] = "123456789"; // WiFi password

char server[] = "prakitblog.com"; // Custom server
int port = 8181;  // Port can be different

const int buzzerPin = 2;
const int trigPin = 23;
const int echoPin = 26;
int distance;
int dieselCapacity;
int setLowestLevel;
int setHeightTank = 0;

int flagValue = 0;

// GPS
String nmeaSentence = "";
float latitude = 0;
float longitude = 0;
unsigned long lastBlynkUpdate = 0;
const unsigned long blynkInterval = 1000; // Update Blynk every 5 seconds

// Timing for sensor readings
unsigned long lastGpsPrintTime = 0;
const unsigned long gpsInterval = 1000; // Check GPS status every 1000ms

BLYNK_WRITE(V1) 
{
  setLowestLevel = param.asInt();
  Serial.print("Set Lowest Level: ");
  Serial.print(setLowestLevel);
  Serial.println("cm");
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

BLYNK_WRITE(V2) 
{
  setHeightTank = param.asInt();
  Serial.print("Set Tank Height: ");
  Serial.print(setHeightTank);
  Serial.println("cm");
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

WidgetLED led1(V6);

void gps_read()
{
  if (Serial2.available()) {
    char c = Serial2.read();
    Serial.print(c); // Debug: Print raw GPS data
    if (c == '\n') {
      Serial.println("Received sentence: " + nmeaSentence);
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
  String fixQuality = getField(nmea, 6);
  if (fixQuality == "0") {
    Serial.println("No GPS fix");
    return;
  }
  String rawLat = getField(nmea, 2);
  String latDir = getField(nmea, 3);
  String rawLon = getField(nmea, 4);
  String lonDir = getField(nmea, 5);

  if (rawLat == "" || rawLon == "" || latDir == "" || lonDir == "") {
    Serial.println("Invalid or incomplete GPGGA sentence");
    return;
  }

  latitude = convertToDecimal(rawLat);
  if (latDir == "S") latitude = -latitude;
  longitude = convertToDecimal(rawLon);
  if (lonDir == "W") longitude = -longitude;

  Serial.print("Raw Lat: "); Serial.println(rawLat);
  Serial.print("Raw Lon: "); Serial.println(rawLon);
  Serial.print("Lat Dir: "); Serial.println(latDir);
  Serial.print("Lon Dir: "); Serial.println(lonDir);
  Serial.println(" ");
  Serial.print("Latitude: "); Serial.println(latitude, 6);
  Serial.print("Longitude: "); Serial.println(longitude, 6);
  Serial.println(" ");

  if (latitude != 0 && longitude != 0 && millis() - lastBlynkUpdate >= blynkInterval) {
    Blynk.virtualWrite(V3, 0, latitude, longitude, "Diesel Tank location");
    Blynk.virtualWrite(V4, latitude);
    Blynk.virtualWrite(V5, longitude);
    lastBlynkUpdate = millis();
  }
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

void readCapacity() 
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  if (setHeightTank > 0) {
    if (distance > setHeightTank) {
      distance = setHeightTank;
    } else if (distance <= 0) {
      distance = setHeightTank;
    }
  } else {
    distance = 0;
  }

  dieselCapacity = setHeightTank - distance;
  if (dieselCapacity < 0) {
    dieselCapacity = 0;
  }

  Serial.print("Distance: "); Serial.print(distance); Serial.println(" cm");
  Serial.print("Diesel Level: "); Serial.print(dieselCapacity); Serial.println(" cm");
  Serial.println(" ");
  Blynk.virtualWrite(V0, dieselCapacity);
  delay(25);
}

void connectToBlynk()
{
  unsigned long start = millis();
  while (!Blynk.connected() && millis() - start < 10000) {
    Blynk.run();
    delay(1000);
  }
  if (Blynk.connected()) {
    Serial.println("Blynk Connected!");
  } else {
    Serial.println("Blynk connection failed!");
  }
}

void setup()
{
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
  Blynk.begin(auth, ssid, pass, server, port);
  Serial.println("Setup complete");
}

void loop()
{
  if (!Blynk.connected()) {
    Serial.println("Blynk disconnected! Reconnecting...");
    connectToBlynk();
  }
  Blynk.run();
  gps_read();
  readCapacity();

  if (dieselCapacity < setLowestLevel && flagValue == 0) {
    // Hantar notification dekat email
    Blynk.email("anwaralif05@gmail.com", "ALERT", "Diesel level LOW!");
    led1.on();
    Serial.println("ALERT! Diesel Level LOW!");
    Serial.println(" ");
    digitalWrite(buzzerPin, HIGH);
    flagValue = 1;
  } 
  if (dieselCapacity > setLowestLevel && flagValue == 1) {
    led1.off();
    Serial.println("Diesel Level Normal");
    Serial.println(" ");
    digitalWrite(buzzerPin, LOW);
    flagValue = 0;
  }

  delay(10);
}