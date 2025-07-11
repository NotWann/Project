#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RTClib.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS1307 rtc;

#define NEXT 11
#define SELECT 12
#define SAVE 13

#define EEPROM_DISTANCE_ADDR 0
#define EEPROM_HOUR_ADDR 1
#define EEPROM_MINUTE_ADDR 2

#define TRIG_PIN 9
#define ECHO_PIN 10

int relayIN1 = 2;
int relayIN2 = 4;

const int potPin = 0;
int potValue;

int menu = 0;
int distanceSet;
int setHour, setMinute;

void setup()
{
  Serial.begin(115200);
  
  lcd.init();
  lcd.backlight();

  pinMode(NEXT, INPUT_PULLUP);
  pinMode(SELECT, INPUT_PULLUP);
  pinMode(SAVE, INPUT_PULLUP);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(relayIN1, OUTPUT);
  pinMode(relayIN2, OUTPUT);

  pinMode(potPin, OUTPUT);

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    while(1);
  }

//Load  values from EEPROM
distanceSet =EEPROM.read(EEPROM_DISTANCE_ADDR);
setHour = EEPROM.read(EEPROM_HOUR_ADDR);
setMinute = EEPROM.read(EEPROM_MINUTE_ADDR);
}

void loop()
{
  int storedDistance = EEPROM.read(EEPROM_DISTANCE_ADDR);
  int storedHour = EEPROM.read(EEPROM_HOUR_ADDR);
  int storedMinute = EEPROM.read(EEPROM_MINUTE_ADDR);

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseInLong(ECHO_PIN, HIGH);
  int distance = (duration * 0.034) / 2;

  int absLevel = constrain(distance, 0, 65);

  if (digitalRead(SELECT) == LOW)
  {
    delay(200);
    menu = (menu + 1) % 3;
    while (digitalRead(SELECT) == LOW);
  }

int potValue = analogRead(potPin);

if (menu == 0) 
{
  distanceSet = map(potValue, 0, 1023, 65, 0);
  
}
else if (menu == 1)
{
  setHour = map(potValue, 0, 1023, 0, 23);
  
}
else if (menu == 2)
{
  setMinute = map(potValue, 0, 1023, 0, 59);
  
}

int absSetDistance = constrain(distanceSet, 0, 65);
int absSetHour = constrain(setHour, 0, 23);
int absSetMinute = constrain(setMinute, 0, 59);

if (digitalRead(NEXT) == LOW)
{
  delay(200);
  if (menu == 0) distanceSet++;
  else if (menu == 1) setHour = (setHour + 1) % 24;
  else if (menu == 2) setMinute = (setMinute + 1) % 60;
  while (digitalRead(NEXT) == LOW);
}

if (digitalRead(SAVE) == LOW) 
{
  delay(200);
  if (menu == 0) EEPROM.update(EEPROM_DISTANCE_ADDR, absSetDistance);
  else if (menu == 1) EEPROM.update(EEPROM_HOUR_ADDR, absSetHour);
  else if (menu == 2) EEPROM.update(EEPROM_MINUTE_ADDR, absSetMinute);

  if (menu == 1 || menu == 2) 
  {
    rtc.adjust(DateTime(2025, 2, 8, absSetHour, absSetMinute, 0));
  }

  lcd.setCursor(0, 3);
  lcd.print("Saved to EEPROM!  ");
  delay(1000);
  lcd.setCursor(0, 3);
  lcd.print("                   "); // Clear message
  while (digitalRead(SAVE) == LOW);
}

// Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mode: ");
  if (menu == 0) lcd.print("Set Distance ");
  else if (menu == 1) lcd.print("Set Hour    ");
  else if (menu == 2) lcd.print("Set Minute  ");

  lcd.setCursor(0, 1);
  lcd.print("Distance: ");
  lcd.print(distance);
  lcd.print(" cm");

  lcd.setCursor(0, 2);
  lcd.print("RTC Time: ");
  if (setHour < 10) lcd.print("0");
  lcd.print(setHour);
  lcd.print(":");
  if (setMinute < 10) lcd.print("0");
  lcd.print(setMinute);

  lcd.setCursor(0, 3);
  if (menu == 0) {
    lcd.print("Set: ");
    lcd.print(distanceSet);
    lcd.print(" cm ");
  } else if (menu == 1) {
    lcd.print("Set Hour: ");
    lcd.print(setHour);
  } else if (menu == 2) {
    lcd.print("Set Min: ");
    lcd.print(setMinute);
  }

  Serial.print("EEPROM Distance: ");
  Serial.println(storedDistance);
  Serial.print("EEPROM Hour: ");
  Serial.println(storedHour);
  Serial.print("EEPROM Minute: ");
  Serial.println(storedMinute);

  delay(100);

}

