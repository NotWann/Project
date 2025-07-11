#include <SoftwareSerial.h>
SoftwareSerial BTSerial(10, 11); // RX, TX
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int pirPin = 3;
int lockPir = 0;

void setup() {
  Serial.begin(115200); 
  BTSerial.begin(115200); // HC-05 baud rate (match your AT+UART setting)

  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(pirPin, INPUT);

  Serial.println("Master ready.");
  lcd.setCursor(0, 0);
  lcd.print("HELLO WORLD!");
  lcd.setCursor(0, 1);
  lcd.print("TKJAKDOAIJS");
}

void loop() {
  int pirState = digitalRead(pirPin);
  if (pirState == HIGH && lockPir == 0)
  {
    BTSerial.write('2');
    Serial.println("Motion detected: LED Blinking, Buzzer ON");
    lockPir = 1;
    delay(200);
  }
  else if (pirState == LOW && lockPir == 1)
  {
    BTSerial.write('3');
    Serial.println("No Motion detected.");
    lockPir = 0;
    delay(200);
  }
  delay(10);
}