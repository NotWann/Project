#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>

#include <Wire.h>
#include <SPI.h>

#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

#include <TimeLib.h>
#include <WidgetRTC.h>
BlynkTimer timer;
WidgetRTC rtc;

#define SS_PIN 5      // SDA/SS Pin
#define RST_PIN 32    // Reset Pin

//#define START_ADDR 0
//#define STOP_ADDR 10

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);

/////////////////////////////

char auth[] = "KndqlGwGYVh7aZOcEEjqy-IfZjv4kP2R";
char ssid[] = "abcd";
char pass[] = "123456789";

char server[] = "prakitblog.com";
int port = 8181;

//////////////////////////////

bool alarmSet_on = false; // Status apakah alarm telah diset
int alarmHour_on;
int alarmMinute_on;

bool alarmSet_off = false; // Status apakah alarm telah diset
int alarmHour_off;
int alarmMinute_off;

bool systemRunning = false;
int lockBuntat = 0;
int lockGegey = 0;
int lockOyen = 0;
int lockGrey = 0;
int lockLeftie = 0;

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
    {"Buntat", 6.51, "Male", {0x13, 0x58, 0x94, 0x14}}, 
    {"Gegey", 1.66, "Male Kitten", {0x83, 0xB0, 0x17, 0x0D}},
    {"Oyen", 2.17, "Male", {0x69, 0x3C, 0x14, 0x2B}},
    {"Grey", 5.67, "Male", {0xD1, 0xB1, 0xEE, 0x24}},
    {"Leftie", 1.35, "Female Kitten", {0x93, 0xB2, 0x4C, 0x14}},
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

// Function to update the Blynk Text Widget with the formatted time

void clockDisplay()
{
  String currentTime = String(hour()) + ":" + minute() + ":" + second();
  String currentDate = String(day()) + " " + month() + " " + year();
  Serial.print("Current time: ");
  Serial.print(currentTime);
  Serial.print(" ");
  Serial.print(currentDate);
  Serial.println();

  if (alarmSet_on && (hour() == alarmHour_on && minute() == alarmMinute_on)) {
    Serial.println("masuk on");
    delay(1000); // Buzzer berbunyi selama 1 detik
    //digitalWrite(buzzerPin, LOW); // Mematikan buzzer
   digitalWrite(buzzerPin, HIGH);
   delay(200);
   digitalWrite(buzzerPin, LOW);

    alarmSet_on = false; // Reset status alarm agar hanya berbunyi sekali
    systemRunning = true;
  }

  if (alarmSet_off && (hour() == alarmHour_off  && minute() == alarmMinute_off )) {
    Serial.println("masuk off");
    delay(1000); // Buzzer berbunyi selama 1 detik
    //digitalWrite(buzzerPin, LOW); // Mematikan buzzer
    digitalWrite(buzzerPin, HIGH);
    delay(200);
    digitalWrite(buzzerPin, LOW);
    alarmSet_off  = false; // Reset status alarm agar hanya berbunyi sekali
    systemRunning = true;
  }

}

BLYNK_CONNECTED() 
{
  rtc.begin();
}

BLYNK_WRITE(V10) {
  TimeInputParam t(param);

  // Process start time

  if (t.hasStartTime())
  {
    Serial.println(String("Start: ") +
                   t.getStartHour() + ":" +
                   t.getStartMinute() + ":" +
                   t.getStartSecond());
  }
  else if (t.isStartSunrise())
  {
    Serial.println("Start at sunrise");
  }
  else if (t.isStartSunset())
  {
    Serial.println("Start at sunset");
  }
  else
  {
    // Do nothing
  }

  // Process stop time

  if (t.hasStopTime())
  {
    Serial.println(String("Stop: ") +
                   t.getStopHour() + ":" +
                   t.getStopMinute() + ":" +
                   t.getStopSecond());
  }
  else if (t.isStopSunrise())
  {
    Serial.println("Stop at sunrise");
  }
  else if (t.isStopSunset())
  {
    Serial.println("Stop at sunset");
  }
  else
  {
    // Do nothing: no stop time was set
  }

  // Process timezone
  // Timezone is already added to start/stop time

  Serial.println(String("Time zone: ") + t.getTZ());

  // Get timezone offset (in seconds)
  Serial.println(String("Time zone offset: ") + t.getTZ_Offset());

  // Process weekdays (1. Mon, 2. Tue, 3. Wed, ...)

  for (int i = 1; i <= 7; i++) {
    if (t.isWeekdaySelected(i)) {
      Serial.println(String("Day ") + i + " is selected");
    }
  }

    alarmHour_on = t.getStartHour();
    alarmMinute_on = t.getStartMinute();
    alarmSet_on = true; // Menandakan alarm telah diset
    Serial.println(String("Alarm set for: ") + alarmHour_on + ":" + alarmMinute_on);

    alarmHour_off = t.getStopHour();
    alarmMinute_off = t.getStopMinute();
    alarmSet_off = true; // Menandakan alarm telah diset
    Serial.println(String("Alarm set for: ") + alarmHour_off + ":" + alarmMinute_off);
 

  Serial.println();
}

/////////////////////////////////
void setup() 
{
  Serial.begin(115200);

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
 
  setSyncInterval(10 * 60); 
  timer.setInterval(10000L, clockDisplay);

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
  timer.run();
  clockDisplay();

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
      Blynk.virtualWrite(V1, String(cats[i].mass, 2));
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
  byte BuntatUID[] = {0x13, 0x58, 0x94, 0x14};
  byte GegeyUID[] = {0x83, 0xB0, 0x17, 0x0D};
  byte OyenUID[] = {0x69, 0x3C, 0x14, 0x2B};
  byte GreyUID[] = {0xD1, 0xB1, 0xEE, 0x24};
  byte LeftieUID[] = {0x93, 0xB2, 0x4C, 0x14};

  if (systemRunning == true)   
  {
    // Reset counter when new RFID is detected
    limitCounter = 0;

    //  Oyen feeder
    if (compareUID(rfid.uid.uidByte, BuntatUID))
    {
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);

      ledcWrite(enablePin, dutyCycle);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);

      lockBuntat = 1;

      while (limitCounter < 8) 
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
    if (compareUID(rfid.uid.uidByte, GegeyUID))
    {
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);

      ledcWrite(enablePin, dutyCycle);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);

      lockGegey = 1;

      while (limitCounter < 4) 
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

    // Dolah Feeder
    if (compareUID(rfid.uid.uidByte, OyenUID))
    {
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);

      ledcWrite(enablePin, dutyCycle);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);

      lockOyen = 1;

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

    //Jupiter Feeder
     if (compareUID(rfid.uid.uidByte, GreyUID))
    {
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);

      ledcWrite(enablePin, dutyCycle);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);

      lockGrey = 1;

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

    //Atan Feeder
     if (compareUID(rfid.uid.uidByte, LeftieUID))
    {
      digitalWrite(buzzerPin, HIGH);
      delay(200);
      digitalWrite(buzzerPin, LOW);

      ledcWrite(enablePin, dutyCycle);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);

      lockLeftie = 1;

      while (limitCounter < 4) 
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
  
  if (lockBuntat == 1 && lockGegey == 1 && lockOyen == 1 && lockGrey == 1 && lockLeftie == 1 )
  {
    systemRunning = false;
    lockBuntat = 0;
    lockGegey = 0;
    lockOyen = 0;
    lockGrey = 0;
    lockLeftie = 0;
  }

  rfid.PICC_HaltA();  // RESET RFID

  ////////////////////////////Debug/////////////////////////////////////
  Serial.print("Limit: ");
  Serial.println(limitState);
  
  delay(10);
}



