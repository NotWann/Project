#define BLYNK_PRINT Serial
#include "HX710B.h"
#include <Wire.h>
#include "SoftwareSerial.h"
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <BlynkSimpleEsp32.h>

char auth[] = "uDPH1uDF9S67LzGE9K18JfUq47XhzTFx";
char ssid[] = "abcd";
char pass[] = "123456789";
char server[] = "prakitblog.com";
int port = 8181;

SoftwareSerial gpsSerial(16, 17);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myservo;
HX710B pressure_sensor;

//Motor propeller 
const int propeller_1A = 4;  //L
const int propeller_1B = 5;  //L
const int propeller_2A = 18; //AUDIT: Changed to 18 to avoid conflict
const int propeller_2B = 19; //AUDIT: Changed to 19 to avoid conflict
const int propellerSpeed = 128; // Reduced for testing

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

//Pressure sensor
const int DOUT = 26;  //pressure sensor data pin
const int SCLK = 25;  //pressure sensor clock pin
float PSI_Cal = 0;
float KPI_1psi = 6.89476;
float KPI_Cal = 0;
int maintain_pressure = 12; // SETTING PRESSURE (MAX 40)

//Air Pump
const int pump = 14;
bool pumpOn = false; // Track pump state

//Valve - Relay
const int valveOn = 12;   // Relay 1: Air In valve
const int valveOff = 13;  // Relay 2: Air Out valve
bool valveInOn = false;   // Track Air In valve state
bool valveOutOn = false;  // Track Air Out valve state

//Push button
const int buttonPin = 23;
int flag_button = 0;

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
  digitalWrite(propeller_1A, 0);
  digitalWrite(propeller_1B, propellerSpeed);
  digitalWrite(propeller_2A, propellerSpeed);
  digitalWrite(propeller_2B, 0);
}

// Move backward function
void moveBackward() 
{
  Serial.println(" BACKWARD ");
  digitalWrite(propeller_1A, propellerSpeed);
  digitalWrite(propeller_1B, 0);
  digitalWrite(propeller_2A, 0);
  digitalWrite(propeller_2B, propellerSpeed);
}

// Stop movement function
void stopMove() 
{
  Serial.println(" STOP ");
  digitalWrite(propeller_1A, 0);
  digitalWrite(propeller_1B, 0);
  digitalWrite(propeller_2A, 0);
  digitalWrite(propeller_2B, 0);
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
      if (count_servo >= 180) 
      {
        count_servo = 180;
        lock_servo = 1;
      }
    }
    else 
    {
      count_servo -= 10;
      if (count_servo <= 0) 
      {
        count_servo = 0;
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

  Serial.print("Distance Atas: "); Serial.println(distance_Atas);
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

  Serial.print("Distance Bawah: "); Serial.println(distance_Bawah);
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

    Serial.print("PSI: "); Serial.println(PSI_Cal);
    Serial.print("Kilopascal: "); Serial.println(KPI_Cal);
    return true; // Reading successful
  }
  return false; // Sensor not ready
}

// Update LCD display
void updateLcd() 
{
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
  if (distance_Atas >= 999) lcd.print("50 ");
  else {
    lcd.print(distance_Atas); 
    lcd.print(" cm ");
  }

  lcd.setCursor(0, 1);
  if (pumpOn) {
    lcd.print(valveInOn ? "Air In  " : valveOutOn ? "Air Out " : "Valves Off");
  } else {
    lcd.print("P.sure:"); 
    lcd.print(KPI_Cal); 
    lcd.print(" Kpa ");  
  }
}

// Non-blocking buzzer sound
void sound() 
{
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}

// GPS data reading
void gps_read() 
{
  if (gpsSerial.available()) {
    char c = gpsSerial.read();
    lastGpsDataTime = millis();
    if (c == '\n') {
      Serial.print("Raw NMEA: "); Serial.println(nmeaSentence);
      if (nmeaSentence.startsWith("$GPGGA")) {
        extractLatLong(nmeaSentence);
      }
      nmeaSentence = "";
    } else {
      nmeaSentence += c;
    }
  }

  if (millis() - lastGpsDataTime > gpsTimeout && millis() - lastGpsPrintTime > gpsInterval) {
    Serial.println("No GPS data received");
    lastGpsPrintTime = millis();
  }
}

// Extract latitude and longitude from GPS data
void extractLatLong(String nmea) {
  String rawLat = getField(nmea, 2);
  String latDir = getField(nmea, 3);
  String rawLon = getField(nmea, 4);
  String lonDir = getField(nmea, 5);
  String fixQuality = getField(nmea, 6);
  String satellites = getField(nmea, 7);

  latitude = convertToDecimal(rawLat);
  if (latDir == "S") latitude = -latitude;

  longitude = convertToDecimal(rawLon);
  if (lonDir == "W") longitude = -longitude;

  if (fixQuality.toInt() > 0 && Blynk.connected()) {
    Blynk.virtualWrite(V0, 0, latitude, longitude, "Robot Location");
    Blynk.virtualWrite(V1, latitude);
    Blynk.virtualWrite(V2, longitude);
    Serial.println("GPS data sent to Blynk Map (V0)");
  }

  Serial.println("--- GPS Data ---");
  Serial.print("Latitude: "); Serial.println(latitude, 6);
  Serial.print("Longitude: "); Serial.println(longitude, 6);
  Serial.print("Fix Quality: "); Serial.println(fixQuality);
  Serial.print("Satellites: "); Serial.println(satellites);
  Serial.println("----------------");
}

// Convert raw GPS coordinates to decimal degrees
float convertToDecimal(String rawValue) {
  if (rawValue == "") return 0.0;
  float value = rawValue.toFloat();
  int degrees = (int)(value / 100);
  float minutes = value - (degrees * 100);
  return degrees + (minutes / 60);
}

// Extract specific field from NMEA sentence
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
      lcd.setCursor(0, 1);
      lcd.print(connected ? "Blynk Connected " : "Blynk Disconnected");
      Serial.println(connected ? "Blynk Connected" : "Blynk Disconnected");
      lastConnected = connected;
      lastBlynkStatusUpdate = millis();
    }
  }
}

// Setup function
void setup() 
{
  Serial.begin(115200);
  gpsSerial.begin(9600);
  
  // Disable brownout detector (for testing only)
  //#include "soc/soc.h"
  //#include "soc/rtc_cntl_reg.h"
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
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
  pinMode(propeller_1A, OUTPUT);
  pinMode(propeller_1B, OUTPUT);
  pinMode(propeller_2A, OUTPUT);
  pinMode(propeller_2B, OUTPUT);
  
  digitalWrite(pump, LOW);    // Ensure pump is off initially
  digitalWrite(valveOn, HIGH); // Relay OFF (active-low)
  digitalWrite(valveOff, HIGH); // Relay OFF (active-low)
  stopMove();
  
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
  if (currentTime - lastUltrasonicTime >= ultrasonicInterval) {
    jarak_atas();
    jarak_bawah();
    lastUltrasonicTime = currentTime;
  }

  if (currentTime - lastPressureTime >= pressureInterval) {
    pressure_read();
    lastPressureTime = currentTime;
  }

  if (currentTime - lastLcdUpdate >= lcdUpdateInterval) {
    updateLcd();
    lastLcdUpdate = currentTime;
  }

  gps_read();
  updateBlynkStatus();

  // Handle button input
  static unsigned long lastButtonPress = 0;
  static unsigned long buttonPressStart = 0;
  static bool buttonHeld = false;
  static int pumpMode = 0; // 0: OFF, 1: Air In, 2: Air Out
  int buttonState = digitalRead(buttonPin);

  // Detect button press and hold
  if (buttonState == LOW) 
  {
    Serial.println("Button Pressed");
    if (!buttonHeld) 
    {
      buttonHeld = true;
      buttonPressStart = currentTime;
    } 
    else if (currentTime - buttonPressStart >= 1000) 
    {
      // Long press: cycle through pump modes
      if (pumpMode == 0) 
      {
        // Turn on pump + Air In valve
        digitalWrite(pump, HIGH);
        setValves(true, false); // Air In ON, Air Out OFF
        pumpOn = true;
        pumpMode = 1;
        sound();
        Serial.println("Pump ON, Air In");
      } 
      else if (pumpMode == 1) 
      {
        // Switch to Air Out valve
        setValves(false, true); // Air In OFF, Air Out ON
        pumpMode = 2;
        sound();
        Serial.println("Pump ON, Air Out");
      } 
      else if (pumpMode == 2) 
      {
        // Turn off pump and valves
        digitalWrite(pump, LOW);
        setValves(false, false); // Both valves OFF
        pumpOn = false;
        pumpMode = 0;
        sound();
        Serial.println("Pump OFF, Valves OFF");
      }
      buttonHeld = false; // Reset to allow new long press
    }
  } 
  else if (buttonHeld) 
  {
    Serial.println("Button Released");
    buttonHeld = false;
    if (currentTime - buttonPressStart < 1000 && (currentTime - lastButtonPress >= 200)) 
    {
      // Short press for movement control
      if (flag_button == 0) 
      {
        sound();
        moveForward();
        flag_button = 1;
        Serial.println("Move Forward");
      } 
      else if (flag_button == 1) 
      {
        sound();
        stopMove();
        flag_button = 0;
        Serial.println("Stop Movement");
      }
      lastButtonPress = currentTime;
    }
  }

  // Control servo based on distance_Atas
  if (distance_Atas <= 20 && distance_Atas < 999) 
  {
    stopMove();
    isSweeping = true;
    sweepStartTime = currentTime;
    servoRotated();
    flag_button = 0;
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
      Serial.println("Servo reset to 90 degrees");
    }
  }

  // Control based on distance_Bawah
  if (distance_Bawah <= 8) 
  {
    if (!pumpOn) 
    {
      digitalWrite(pump, LOW);
      setValves(false, true); // Air Out ON to release pressure
      pumpOn = true;
      sound();
      Serial.println("Distance Bawah <= 5cm, Pump ON, Air Out");
    }
  } 
  else if (pumpOn && distance_Bawah > 5) 
  {
    // Turn off pump and valves if distance_Bawah exceeds 5cm and was triggered by this condition
    digitalWrite(pump, LOW);
    setValves(false, false);
    pumpOn = false;
    Serial.println("Distance Bawah > 5cm, Pump OFF, Valves OFF");
  }
}