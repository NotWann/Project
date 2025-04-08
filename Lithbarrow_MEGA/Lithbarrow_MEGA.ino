#include <Wire.h>
#include <HX711_ADC.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

#define DOUT 4
#define SCK  5

HX711_ADC LoadCell(DOUT, SCK);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int relayIN1esp = 24;
const int relayIN2esp = 25;
const int dataEspPin = 23;

const float calibrationValue = -29.67;
unsigned long t = 0;

int lockR1 = 0;
int lockR2 = 0;
int flagWeight = 0;

void setup()
{
  Serial.begin(115200);
  pinMode(relayIN1esp, INPUT);    //  pin for receiving signal for LCD display
  pinMode(relayIN2esp, INPUT);

  pinMode(dataEspPin, OUTPUT);    //  pin for sending signal to ESP32

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Litfbarrow");
  delay(1000);
  lcd.clear();
  Serial.println("Starting...");

  LoadCell.begin();

  LoadCell.setCalFactor(calibrationValue); // Apply calibration

  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag())
  {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else
  {
    Serial.println("Startup is complete");
  }

  Serial.print("Current Calibration: ");
  Serial.println(calibrationValue, 2);
}

void loop()
{
  static boolean newDataReady = 0;
  const int serialPrintInterval = 20;

  if (LoadCell.update()) newDataReady = true;

  if (newDataReady)
  {
    if (millis() > t + serialPrintInterval)
    {
      float weight = LoadCell.getData();

      weight = abs(weight) / 1000;     // Convert g to kg
      lcd.setCursor(0, 0);
      lcd.print("WEIGHT: ");
      lcd.print(weight, 2);
      lcd.print(" kg ");

      Serial.print("Weight: ");
      Serial.print(weight, 2);
      Serial.println(" kg ");

      newDataReady = 0;
      t = millis();
      
      if (weight > 10.00 && flagWeight == 0)
      {
        digitalWrite(dataEspPin, HIGH);
        Serial.println("  | Sent to ESP32 | ");
        flagWeight = 1;
      }
      if (weight < 10.00 && flagWeight == 1)
      {
        digitalWrite(dataEspPin, LOW);
        flagWeight = 0;
      }
    }
  }

  bool relayIN1State = digitalRead(relayIN1esp);
  bool relayIN2State = digitalRead(relayIN2esp);
  
  if (relayIN1State == true && lockR1 == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("MOTOR: STOP");
    Serial.println("MOTOR: STOP");
    lockR1 = 1;
  }
  if (relayIN1State == false && lockR1 == 1) 
  {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("MOTOR: UP");
    Serial.println("MOTOR: UP");
    lockR1 = 0;
  }

  if (relayIN2State == true && lockR2 == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("MOTOR: STOP");
    Serial.println("MOTOR: STOP");
    lockR2 = 1;
  }

  if (relayIN2State == false && lockR2 == 1) 
  {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("MOTOR: DOWN");
    Serial.println("MOTOR: DOWN");
    lockR2 = 0;
  }
 
  delay(30);
}
