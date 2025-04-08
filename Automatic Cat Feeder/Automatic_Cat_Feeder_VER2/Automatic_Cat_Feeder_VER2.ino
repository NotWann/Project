#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include "time.h"
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 5      // SDA/SS Pin
#define RST_PIN 32    // Reset Pin

#define EEPROM_SIZE 64
#define START_ADDR 0
#define STOP_ADDR 10

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);

/////////////////////////////
char auth[] = "KndqlGwGYVh7aZOcEEjqy-IfZjv4kP2R";
char ssid[] = "abcd";
char pass[] = "123456789";

char server[] = "prakitblog.com";
int port = 8181;

//////////////////////////////
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800; // GMT+8 for Singapore
const int   daylightOffset_sec = 0;

int startHour = 0;
int startMinute = 0;

int stopHour = 0;
int stopMinute = 0;

bool systemRunning = false;

String startTime = "00:00"; // Default values
String stopTime = "00:00";  

//////////////////////////////
const int motorPin1 = 23;
const int motorPin2 = 25;
const int enablePin = 26;

const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 100;

/////////////////////////////
int limitSwitch = 27;
int limitCounter = 0;
int prevLimitState = HIGH;  // Store the last state of the limit switch

//////////////////////////////
const int buzzerPin = 2;

//////////////////////////////
struct Cat {
  String name;
  float mass;
  String diet;
  byte uid[4];
};

// Array of registered cats
Cat cats[] = {
    {"Oyen", 10.4, "Gemuk", {0x13, 0x58, 0x94, 0x14}}, 
    {"Abu", 5.2, "Kurus", {0x83, 0xB0, 0x17, 0x0D}},
};

////////////////////////////////////////

// Last scanned UID storage (initially empty)
byte lastUID[4] = {0, 0, 0, 0};

// Function to compare two UIDs
bool compareUID(byte scannedUID[], byte storedUID[]) 
{
    for (int i = 0; i < 4; i++) {
        if (scannedUID[i] != storedUID[i]) return false;
    }
    return true;
}

// Store the last scanned UID
void copyUID(byte source[], byte destination[])
{
    for (int i = 0; i < 4; i++) {
        destination[i] = source[i];
    }
}

/////////////////////////////////
//Start time
BLYNK_WRITE(V4)  // Step H Widget for startHour
{
  startHour = param.asInt();
  Serial.print("Set Start Hour: ");
  Serial.println(startHour);
  updateBlynkTimeDisplay();
  saveEEPROM(START_ADDR, startTime);
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
}

BLYNK_WRITE(V5)  // Step H Widget for startMinute
{
  startMinute = param.asInt();
  Serial.print("Set Start Minute: ");
  Serial.println(startMinute);
  updateBlynkTimeDisplay();
  saveEEPROM(START_ADDR, startTime);
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
}

//Stop time
BLYNK_WRITE(V8)  // Step H Widget for stopHour
{
  stopHour = param.asInt();
  Serial.print("Set Stop Hour: ");
  Serial.println(stopHour);
  updateBlynkTimeDisplay();
  saveEEPROM(STOP_ADDR, stopTime);
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
}

BLYNK_WRITE(V9)  // Step H Widget for stopMinute
{
  stopMinute = param.asInt();
  Serial.print("Set Stop Minute: ");
  Serial.println(stopMinute);
  updateBlynkTimeDisplay();
  saveEEPROM(STOP_ADDR, stopTime);
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
}

// Function to update the Blynk Text Widget with the formatted time
void updateBlynkTimeDisplay()
{
  char startStr[9];
  sprintf(startStr, "%02d:%02d", startHour, startMinute);  // Format as HH:MM
  Blynk.virtualWrite(V6, startStr);
  startTime = String(startStr);  // Assign to String object

  char stopStr[9];
  sprintf(stopStr, "%02d:%02d", stopHour, stopMinute);
  Blynk.virtualWrite(V7, stopStr);
  stopTime = String(stopStr);
}

/////////////////////////////////
void setup() 
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  lcd.clear();

  pinMode(limitSwitch, INPUT_PULLUP);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(enablePin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  ledcWrite(enablePin, 0);
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);

  Blynk.begin(auth, ssid, pass, server, port);

  ledcAttachChannel(enablePin, freq, resolution, pwmChannel);

  lcd.init();
  lcd.backlight();

  SPI.begin(); 
  rfid.PCD_Init(); 

  lcd.setCursor(0, 0);
  lcd.print("Automatic Cat");
  lcd.setCursor(0, 1);
  lcd.print("Feeder");

  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  startTime = readEEPROM(START_ADDR);
  stopTime = readEEPROM(STOP_ADDR);

  Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
  Serial.print("Stored Start Time: "); 
  Serial.println(startTime);
  Serial.print("Stored Stop Time: "); 
  Serial.println(stopTime);
  Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning");
  lcd.setCursor(0, 1);
  lcd.print("RFID Tag...");

  Serial.println("Scan an RFID tag...");
}

void loop() 
{
  Blynk.run();

////////////////////////////////////Time///////////////////////////////////////
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) 
  {
    Serial.println("Failed to obtain time");
    return;
  }

  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  Blynk.virtualWrite(V3, timeStr);

  Serial.print("Real Time (timeStr): ");
  Serial.println(timeStr);
  Serial.print("Set Start Time: ");
  Serial.println(startTime);
  Serial.print("Set Stop Time: ");
  Serial.println(stopTime);
  delay(1000);

  if (String(timeStr).substring(0, 5) >= startTime && String(timeStr).substring(0, 5) <= stopTime && systemRunning == false)
  {
    Serial.println("____SYSTEM____START____");

    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);

    systemRunning = true;
  }

  if (String(timeStr).substring(0, 5) <= startTime || String(timeStr).substring(0, 5) >= stopTime && systemRunning == true)
  {
    Serial.println("____SYSTEM____STOP____");

    systemRunning = false;
  }


///////////////////////////RFID FUNCTION//////////////////////////////////
  int limitState = digitalRead(limitSwitch); // Read limit switch state

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) 
  {
    return; // No new card detected
  }

  // Ignore if the same card is scanned continuously
  if (compareUID(rfid.uid.uidByte, lastUID)) 
  {
    rfid.PICC_HaltA();
    return;
  }

  // Store the new UID
  copyUID(rfid.uid.uidByte, lastUID);

  Serial.print("Card UID: ");

  for (byte i = 0; i < rfid.uid.size; i++) 
  {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Check if the RFID matches a stored UID
  for (int i = 0; i < sizeof(cats) / sizeof(cats[0]); i++) 
  {
    if (compareUID(rfid.uid.uidByte, cats[i].uid)) 
    {
      Serial.println("Cat Found!");
      Serial.print("Name: "); 
      Serial.println(cats[i].name);
      Serial.print("Mass: "); 
      Serial.print(cats[i].mass); 
      Serial.println(" kg");
      Serial.print("Diet: "); 
      Serial.println(cats[i].diet);
      Serial.println("----------------------");

      Blynk.virtualWrite(V0, cats[i].name);
      Blynk.virtualWrite(V1, cats[i].mass);
      Blynk.virtualWrite(V2, cats[i].diet);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Name: " + cats[i].name);
      lcd.setCursor(0, 1);
      lcd.print("Mass: " + String(cats[i].mass, 2) + "kg");

      break;
    }
  }

  /////////////////FEEDER FUNCTION////////////////////////////
  byte oyenUID[] = {0x13, 0x58, 0x94, 0x14};
  byte abuUID[] = {0x83, 0xB0, 0x17, 0x0D};

  if (systemRunning == true)   
  {
    // Reset counter when new RFID is detected
    limitCounter = 0;

    //  Oyen feeder
    if (compareUID(rfid.uid.uidByte, oyenUID))
    {
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);

      ledcWrite(enablePin, dutyCycle);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);

      while (limitCounter < 6) 
      {
        limitState = digitalRead(limitSwitch); // Read limit switch again
                
        if (prevLimitState == HIGH && limitState == LOW) 
        { 
          limitCounter++; 
          Serial.print("Gear Tooth Count: ");
          Serial.println(limitCounter);
          delay(200);
          }

          prevLimitState = limitState;
        }

      // Stop the motor after reaching the desired count
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);
      delay(200);
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);
      delay(200);

      ledcWrite(enablePin, 0);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, LOW);

      Serial.println("Motor OFF - Feeding Done");

      lcd.clear();
      lcd.setCursor(0, 0);

      lcd.print("DONE!");
      delay(1000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Scanning...");
    }

    //  Abu feeder
    if (compareUID(rfid.uid.uidByte, abuUID))
    {
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);

      ledcWrite(enablePin, dutyCycle);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);

      while (limitCounter < 12) 
      {
        limitState = digitalRead(limitSwitch); // Read limit switch again
                
        if (prevLimitState == HIGH && limitState == LOW) 
        { 
          limitCounter++; 
          Serial.print("Gear Tooth Count: ");
          Serial.println(limitCounter);
          delay(200);
          }

          prevLimitState = limitState;
        }

      // Stop the motor after reaching the desired count
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);
      delay(200);
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);
      delay(200);

      ledcWrite(enablePin, 0);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, LOW);

      Serial.println("Motor OFF - Feeding Done");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DONE!");
      delay(1000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Scanning...");
    }
  }
  
 
  rfid.PICC_HaltA();  // RESET RFID

  ////////////////////////////Debug/////////////////////////////////////
  Serial.print("Limit: ");
  Serial.println(limitState);
  
  delay(10);
}

// Store time to EEPROM
void saveEEPROM(int addr, String value) 
{
  for (int i = 0; i < value.length(); i++) 
  {
    EEPROM.write(addr + i, value[i]);
  }
  EEPROM.write(addr + value.length(), '\0'); // Null terminator
  EEPROM.commit();  // Save changes
}

// Read time from EEPROM
String readEEPROM(int addr) 
{
  char value[6];  // "HH:MM" + null terminator
  for (int i = 0; i < 6; i++) 
  {
    value[i] = EEPROM.read(addr + i);
  }
  return String(value);
}