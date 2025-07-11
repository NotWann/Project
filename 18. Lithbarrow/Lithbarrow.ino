#include <Wire.h>
#include <HX711_ADC.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

#define DOUT 4
#define SCK  5

HX711_ADC LoadCell(DOUT, SCK);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int calibrateValueAddress = 0;
unsigned long t = 0;

void setup()
{
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Lithbarrow");
  delay(10);
  lcd.clear();
  Serial.println("Starting...");

  LoadCell.begin();

  // Set calibration value (adjust this for proper conversion to kg)
  float calibrationValue = 5.79; // Adjust based on your calibration test
  LoadCell.setCalFactor(calibrationValue); // Apply calibration

  unsigned long stabilizingtime = 2000;
  boolean _tare = true; // Perform tare operation at startup
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  } else {
    Serial.println("Startup is complete");
  }
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

      weight = abs(weight) / 1000;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Weight: ");
      lcd.print(weight, 2);
      lcd.print(" kg ");

      Serial.print("Weight: ");
      Serial.print(weight, 2); 
      Serial.println(" kg ");

      newDataReady = 0;
      t = millis();
    }
  }

  if (Serial.available() > 0)
  {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

    if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
}