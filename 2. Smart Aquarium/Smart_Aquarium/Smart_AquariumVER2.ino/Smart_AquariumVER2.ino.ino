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

int menu = 0;
int distanceSet;
int setHour, setMinute;

const int potPin = 0;

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
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseInLong(ECHO_PIN, HIGH);
  int distance = (duration * 0.034) / 2;

  int absLevel = constrain(distance, 0, 65);

  Serial.print("Water Level: ");
  Serial.print(distance);
  Serial.println(" cm ");

int potValue = analogRead(potPin);
int setTarget = map(potValue, 0, 1023, 65, 0);
int absTarget = constrain(setTarget, 0, 65);

int storedValue = EEPROM.read(EEPROM_ADDR);

if (digitalRead(SAVE) == LOW)
{
  flag = 1;
  if (flag == 1)
  {
    Serial.println("Button Pressed!");
    delay(200);
    EEPROM.update(EEPROM_ADDR, absTarget);
  
    Serial.println("Value saved to EEPROM");
  }
  else 
  {
    flag = 0;
  }
}
else 
{
  flag = 0;
}

if (distance <= storedValue)
{
  digitalWrite(relayIN1, LOW);
  Serial.println("  ON  ");
  delay(100);
}
else 
{
  digitalWrite(relayIN1, HIGH);
  Serial.println("  OFF  ");
  delay(100);
}

  lcd.setCursor(0, 0);
  lcd.print("Level: ");
  lcd.print(absLevel);
  lcd.print(" cm ");

  lcd.setCursor(0, 1);
  lcd.print("Set Level: ");
  lcd.print(setTarget);
  lcd.print(" cm ");

  Serial.print("PotValue: ");
  Serial.println(potValue);
  Serial.print("Stored Set Level: ");
  Serial.println(storedValue);

  delay(10);
}

