/*################################################################*/
/*Blynk Legacy */
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>

char auth[] = "YFH-6ajcrFkyOmauW1_8UcYbJioGvEr9";  // Auth token
char ssid[] = "abcd";                              // WiFi ssid
char pass[] = "123456789";                         // WiFi password

char server[] = "prakitblog.com";  // Custom server
int port = 8181;                   // port

/*################################################################*/
#include <HX711_ADC.h>
const int HX711_dout = 4;  //mcu > HX711 dout pin
const int HX711_sck = 5;   //mcu > HX711 sck pin
HX711_ADC LoadCell(HX711_dout, HX711_sck);

/*################################################################*/
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

/*################################################################*/
/*Indicator*/
const int buzzer_pin = 12;
const int led_pin = 2;

/*################################################################*/
/*HX711 read weight code*/
const float calibrationValue = -28.53;
unsigned long t = 0;
int flagWeight = 0;
float set_weight = 10.00;

void read_weight() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 20;

  if (LoadCell.update()) newDataReady = true;

  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float weight = LoadCell.getData();

      weight = abs(weight) / 1000;  // Convert g to kg
      lcd.setCursor(0, 0);
      lcd.print("Weight:"); lcd.print(weight, 2); lcd.print(" kg ");
      lcd.setCursor(0, 1);
      lcd.print("Set:"); lcd.print(set_weight, 2); lcd.print(" kg");

      // Display blynk
      Blynk.virtualWrite(V0, weight);

      Serial.print("Weight: ");
      Serial.print(weight, 2);
      Serial.println(" kg ");

      newDataReady = 0;
      t = millis();

      if (weight < set_weight && weight > 0.99 && flagWeight == 0) {
        digitalWrite(buzzer_pin, HIGH);
        digitalWrite(led_pin, HIGH);
        Serial.println("Weight Reach!");
        flagWeight = 1;
      }
      else if (weight > set_weight && flagWeight == 1) {
        digitalWrite(buzzer_pin, LOW);
        digitalWrite(led_pin, LOW);
        flagWeight = 0;
      }
      else if (weight < 1.00) {
        digitalWrite(buzzer_pin, LOW);
        digitalWrite(led_pin, LOW);
        flagWeight = 0;
      }
    }
  }
}

/*################################################################*/
/*Blynk code section*/
void connectToBlynk() {
  unsigned long start = millis();
  while (!Blynk.connected() && millis() - start < 10000)  // 10 seconds timeout
  {
    Blynk.run();
    delay(1000);
  }
  if (Blynk.connected()) {
    Serial.println("Blynk Connected!");
  } else {
    Serial.println("Blynk connection failed!");
  }
}

/****************************************************************/
/*Set Weight Widget code*/
BLYNK_WRITE(V1) {
  set_weight = param.asInt();
  digitalWrite(buzzer_pin, HIGH);
  delay(100);
  digitalWrite(buzzer_pin, LOW);
  lcd.setCursor(0, 1);
  lcd.print("               ");
  Serial.print("Set Weight: "); Serial.print(set_weight); Serial.println("kg");
}

/*################################################################*/
void setup() {
  Serial.begin(115200);
  pinMode(buzzer_pin, OUTPUT);
  pinMode(led_pin, OUTPUT);

  /****************************************************************/
  // Initialize blynk config
  Blynk.begin(auth, ssid, pass, server, port);

  /****************************************************************/
  /*Initialize lcd*/
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Gas Tank");
  lcd.setCursor(0, 1);
  lcd.print("Weight Detector");
  delay(1000);
  lcd.clear();
  Serial.println("Starting...");

  /****************************************************************/
  /*Initialize HX711 sensor*/
  LoadCell.begin();
  LoadCell.setCalFactor(calibrationValue);  // Apply calibration

  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1)
      ;
  } else {
    Serial.println("Startup is complete");
  }

  Serial.print("Current Calibration: ");
  Serial.println(calibrationValue, 2);
  /****************************************************************/
}

/*################################################################*/
void loop() {
  // Run Blynk
  if (!Blynk.connected()) {
    Serial.println("Blynk disconnected! Reconnecting...");
    connectToBlynk();
  }
  Blynk.run();
  
  /****************************************************************/
  read_weight();
}
