#define BLYNK_PRINT Serial
#include "HX710B.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <BlynkSimpleEsp32.h>
#include <HardwareSerial.h>
HardwareSerial gpsSerial(2); //16, 17

char auth[] = "gzZjZdLRJt10MUQry91bZm_tXsjJCCLX";  //Email: floatrobot0@gmail.com
char ssid[] = "abcd";                              //Password: floatrobot123#
char pass[] = "123456789";
char server[] = "prakitblog.com";
int port = 8181;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myservo;
HX710B pressure_sensor;

//Motor propeller 
const int propeller_1A = 4;  //L
const int propeller_1B = 5;  //L
const int propeller_2A = 18; //R
const int propeller_2B = 19; //R
const int propellerSpeed = 100; // Reduced for testing

//Servo
const int servoPin = 15;
int lock_servo = 0;
int count_servo = 90; // Initialize to 90 degrees
bool isSweeping = false;
unsigned long sweepStartTime = 0;
const unsigned long continueSweepDuration = 5000; // 5 seconds to continue sweeping

//Ultrasonic atas
const int trigPin_Atas = 32;
const int echoPin_Atas = 27;
int distance_Atas = 0;

//Ultrasonic bawah
const int trigPin_Bawah = 33;
const int echoPin_Bawah = 34;
int distance_Bawah = 0;
static bool distanceBawahTriggered = false; // Track if distance_Bawah triggered the pump

//Pressure sensor
const int DOUT = 26;  //pressure sensor data pin
const int SCLK = 25;  //pressure sensor clock pin
float PSI_Cal = 0;
float KPI_1psi = 6.89476;
float KPI_Cal = 0;
int lock_pressure = 0;
// For pump release timing
bool isReleasingPressure = false; // Tracks if pump is in release pressure state
unsigned long releaseStartTime = 0; // Stores when the release started
const unsigned long releaseDuration = 35000; // 20 seconds for release

//Air Pump
const int pump = 14;

//Valve - Relay
const int valveOn = 12;   // Relay 1: Air In valve
const int valveOff = 13;  // Relay 2: Air Out valve
bool valveInOn = false;   // Track Air In valve state
bool valveOutOn = false;  // Track Air Out valve state

//Push button
const int buttonPin = 23;

//Buzzer
const int buzzerPin = 2;
bool buzzerOn = false;
unsigned long buzzerStartTime = 0;
const unsigned long buzzerDuration = 100; // Buzzer on for 100ms

//GPS
String nmeaSentence = "";
float latitude = 0.0;
float longitude = 0.0;
unsigned long lastGpsDataTime = 0;
const unsigned long gpsTimeout = 10000; // 10 seconds timeout for no GPS data

//Timing for sensor readings and LCD
unsigned long lastUltrasonicTime = 0;
unsigned long lastPressureTime = 0;
unsigned long lastGpsPrintTime = 0;
unsigned long lastLcdUpdate = 0;
const unsigned long ultrasonicInterval = 100; // Read ultrasonic every 100ms
const unsigned long pressureInterval = 500; // Read pressure every 500ms
const unsigned long gpsInterval = 1000; // Check GPS status every 1000ms
const unsigned long lcdUpdateInterval = 500; // Update LCD every 500ms

// Move forward function
void moveForward() 
{
  Serial.println(" FORWARD ");
  analogWrite(propeller_1A, 0);
  analogWrite(propeller_1B, propellerSpeed);
  analogWrite(propeller_2A, propellerSpeed);
  analogWrite(propeller_2B, 0);
}

// Move backward function
void moveBackward() 
{
  Serial.println(" BACKWARD ");
  analogWrite(propeller_1A, propellerSpeed);
  analogWrite(propeller_1B, 0);
  analogWrite(propeller_2A, 0);
  analogWrite(propeller_2B, propellerSpeed);
}

// Stop movement function
void stopMove() 
{
  Serial.println(" STOP ");
  analogWrite(propeller_1A, 0);
  analogWrite(propeller_1B, 0);
  analogWrite(propeller_2A, 0);
  analogWrite(propeller_2B, 0);
}

// Servo sweep function
void servoRotated() 
{
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastUpdate >= 100) 
  {
    if (lock_servo == 0) 
    {
      count_servo += 10;
      if (count_servo >= 130) 
      {
        count_servo = 130;
        lock_servo = 1;
      }
    }
    else 
    {
      count_servo -= 10;
      if (count_servo <= 50) 
      {
        count_servo = 50;
        lock_servo = 0;
      }
    }
    myservo.write(count_servo);
    Serial.print("Servo Angle: "); Serial.println(count_servo);
    Serial.print("Lock: "); Serial.println(lock_servo);
    lastUpdate = currentTime;
  }
}

// Ultrasonic distance_Atas measurement
void jarak_atas() 
{
  digitalWrite(trigPin_Atas, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin_Atas, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_Atas, LOW);

  unsigned long duration = pulseIn(echoPin_Atas, HIGH, 5000); // 5ms timeout (~85cm max)
  if (duration == 0) {
    distance_Atas = 999; // Indicate no echo
  } else {
    distance_Atas = duration * 0.034 / 2;
  }

  //Serial.print("Distance Atas: "); Serial.println(distance_Atas);
}

// Ultrasonic distance_Bawah measurement
void jarak_bawah() 
{
  digitalWrite(trigPin_Bawah, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin_Bawah, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_Bawah, LOW);

  unsigned long duration = pulseIn(echoPin_Bawah, HIGH, 5000); // 5ms timeout (~85cm max)
  if (duration == 0) {
    distance_Bawah = 999; // Indicate no echo
  } else {
    distance_Bawah = duration * 0.034 / 2;
  }

  //Serial.print("Distance Bawah: "); Serial.println(distance_Bawah);
}

// Non-blocking pressure sensor reading
bool pressure_read() 
{
  if (pressure_sensor.is_ready()) 
  {
    float pascal = (pressure_sensor.read_average(3) * 2.98023e-7) * 200 + 500;
    float mmHg = pascal * 0.00750062;

    PSI_Cal = (mmHg - 3.85);
    if (PSI_Cal < 0.05)
    {
      PSI_Cal = 0;
      KPI_Cal = 0;
    }
    else
    {
      PSI_Cal = PSI_Cal + 0.5;
      KPI_Cal = PSI_Cal * KPI_1psi;
    }

    //Serial.print("PSI: "); Serial.println(PSI_Cal);
    //Serial.print("Kilopascal: "); Serial.println(KPI_Cal);
    return true; // Reading successful
  }
  return false; // Sensor not ready
}

// Update LCD display
void updateLcd() 
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("D1:");
  if (distance_Atas >= 999) lcd.print("50cm");
  else 
  {
    lcd.print(distance_Atas); 
    lcd.print("cm "); 
  }
  lcd.setCursor(8, 0);
  lcd.print("D2:");
  if (distance_Bawah >= 999) lcd.print("50cm");
  else 
  {
    lcd.print(distance_Bawah);  
    lcd.print("cm"); 
  }

  lcd.setCursor(0, 1);
  lcd.print("P.sure:"); 
  lcd.print(KPI_Cal); 
  lcd.print(" Kpa ");   
}

// Non-blocking buzzer sound
void sound() 
{
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

// GPS read function
void gps_read()
{
  if (gpsSerial.available()) {
    char c = gpsSerial.read();  // Read one character
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

  Serial.print("Raw Lat: "); Serial.println(rawLat);
  Serial.print("Raw Lon: "); Serial.println(rawLon);

  // Convert raw latitude and longitude to decimal degrees
  latitude = convertToDecimal(rawLat);
  if (latDir == "S") latitude = -latitude; // South is negative

  longitude = convertToDecimal(rawLon);
  if (lonDir == "W") longitude = -longitude; // West is negative

  Blynk.virtualWrite(V0, 0, latitude, longitude, "Robot Location");
  Blynk.virtualWrite(V1, latitude);
  Blynk.virtualWrite(V2, longitude);

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

// Control valves
void setValves(bool airIn, bool airOut) {
  valveInOn = airIn;
  valveOutOn = airOut;
  digitalWrite(valveOn, airIn ? LOW : HIGH);   // Active-low relay for Air In
  digitalWrite(valveOff, airOut ? LOW : HIGH); // Active-low relay for Air Out
  Serial.print("Valve In: "); Serial.println(airIn ? "ON" : "OFF");
  Serial.print("Valve Out: "); Serial.println(airOut ? "ON" : "OFF");
}

// Blynk connection status update
void updateBlynkStatus() {
  static unsigned long lastBlynkStatusUpdate = 0;
  static bool lastConnected = false;
  if (millis() - lastBlynkStatusUpdate >= 5000) {
    bool connected = Blynk.connected();
    if (connected != lastConnected) {
      lcd.clear();
      Serial.println(connected ? "Blynk Connected" : "Blynk Disconnected");
      lastConnected = connected;
      lastBlynkStatusUpdate = millis();
    }
  }
}

// System reset
void resetSystem() {
  stopMove();
  digitalWrite(pump, LOW);
  setValves(false, false);
  distanceBawahTriggered = false;
  isSweeping = false;
  count_servo = 90;
  lock_servo = 0;
  lock_pressure = 0;
  myservo.write(90);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Reset");
  sound();
  Serial.println("System Reset: Propellers OFF, Pump OFF, Valves OFF, Servo at 90");
  delay(1000); // Show reset message briefly
  lcd.clear();
}

// Setup function
void setup() 
{
  Serial.begin(115200);
  pinMode(propeller_1A, OUTPUT);
  pinMode(propeller_1B, OUTPUT);
  pinMode(propeller_2A, OUTPUT);
  pinMode(propeller_2B, OUTPUT);
  
  stopMove();

  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  
  myservo.attach(servoPin);
  myservo.write(90); // Initialize servo to 90 degrees

  pressure_sensor.begin(DOUT, SCLK);
  pinMode(trigPin_Atas, OUTPUT);
  pinMode(echoPin_Atas, INPUT);
  pinMode(trigPin_Bawah, OUTPUT);
  pinMode(echoPin_Bawah, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pump, OUTPUT);
  pinMode(valveOn, OUTPUT);
  pinMode(valveOff, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(pump, LOW);    // Ensure pump is off initially
  digitalWrite(valveOn, HIGH); // Relay OFF (active-low)
  digitalWrite(valveOff, HIGH); // Relay OFF (active-low)
  
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Connecting ...");

  Blynk.begin(auth, ssid, pass, server, port);

  Serial.println("System Started. Waiting for GPS data...");
}

// Main loop
void loop() 
{
  Blynk.run(); // Handle Blynk communication

  unsigned long currentTime = millis();

  // Read sensors at controlled intervals
  if (currentTime - lastUltrasonicTime >= ultrasonicInterval) 
  {
    jarak_atas();
    jarak_bawah();
    lastUltrasonicTime = currentTime;
  }

  if (currentTime - lastPressureTime >= pressureInterval) 
  {
    pressure_read();
    lastPressureTime = currentTime;
  }

  if (currentTime - lastLcdUpdate >= lcdUpdateInterval) 
  {
    updateLcd();
    lastLcdUpdate = currentTime;
  }

  gps_read();
  updateBlynkStatus();

  // Push button reset
  static unsigned long lastButtonPress = 0;
  int buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && (currentTime - lastButtonPress >= 200)) 
  {
    resetSystem();
    lastButtonPress = currentTime;
  }

  // Control servo based on distance_Atas
  if (distance_Atas <= 15 && distanceBawahTriggered == true) 
  {
    moveBackward();
    isSweeping = true;
    sweepStartTime = currentTime;
    servoRotated();
  }
  else if (isSweeping) 
  {
    if (currentTime - sweepStartTime < continueSweepDuration) 
    {
      servoRotated();
    } 
    else 
    {
      isSweeping = false;
      count_servo = 90;
      lock_servo = 0;
      myservo.write(90);
      moveForward();
      Serial.println("Servo returned to 90 degrees");
    }
  } 
  else 
  {
    if (count_servo != 90) 
    {
      count_servo = 90;
      lock_servo = 0;
      myservo.write(90);
      moveForward();
      Serial.println("Servo reset to 90 degrees");
    }
  }

  // Control pump based on distance_Bawah 
  if (distance_Bawah < 35 && lock_pressure == 1 && !isReleasingPressure) 
  {
    stopMove();
    digitalWrite(pump, HIGH);
    setValves(false, true); // Air Out ON to release pressure
    sound();
    Serial.println("Distance Bawah <= 35cm, Pump ON, Air Out");
    isReleasingPressure = true;
    releaseStartTime = millis();
  }
  if (isReleasingPressure && millis() - releaseStartTime >= releaseDuration)
  {
    digitalWrite(pump, LOW);
    setValves(false, false); // All valves OFF
    sound();
    isReleasingPressure = false;
  }

  //Check water level if above 15cm turn ON pump, Air In ON
  else if (distance_Bawah >= 35 && distance_Bawah <= 40 && distanceBawahTriggered == false)
  {
    // Turn on pump + Air In valve
    digitalWrite(pump, HIGH);
    setValves(true, true); // Air In ON, Air Out OFF
    distanceBawahTriggered = true;
    sound();
    Serial.println("Pump ON, Air In");
  }

  if (KPI_Cal >= 28.00 && lock_pressure == 0)
  {
    delay(1000);
    moveForward();
    sound();
    lock_pressure = 1;
    Serial.println("Pressure HIGH, Pump STOP");
  }

  delay(50);
}