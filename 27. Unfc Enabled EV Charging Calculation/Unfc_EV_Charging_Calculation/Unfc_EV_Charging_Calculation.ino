#include <Wire.h>
#include <Adafruit_PN532.h>
#define SDA_PIN A4
#define SCL_PIN A5
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

/*#######################################################*/
/* EDIT SECTION*/
float harga = 0.07;  // 7 sen per watt-minute
/*#######################################################*/

/*#######################################################*/
/* Pin definition */
/*#######################################################*/
const int buzzer_pin = 13;
const int relay_charging_pin = 7;
const int current_sensor_pin = A0;
const int voltage_sensor_pin = A3;

/*#######################################################*/
/* Variable for NFC Module reading card UID */
/*#######################################################*/
uint8_t targetUID[] = { 0x3, 0xE6, 0x8, 0x35 };  // Example UID
uint8_t targetUIDLength = 4;                     // Length of the target UID
bool valid = false;
int lock_card = 0;
int lock_charging = 0;

/*#######################################################*/
/* Variable for set the price based on watt-minute */
/*#######################################################*/
float base_price = 0.0;               // Starting price (e.g., fixed fee)
float price_per_watt_minute = harga;  // Price per watt-minute in RM (adjust as needed)
float total_price = 0.0;
float total_energy_watt_minutes = 0.0;  // Total energy used in watt-minutes

/*#######################################################*/
/* Variable for tracking time and calculate estimated time 
   take to fully charge based on the initial voltage 
   reading before chaarging start */
/*#######################################################*/
unsigned long count_time = 0;
unsigned long last_update_time = 0;       // Track time of last energy calculation
unsigned long estimated_charge_time = 0;  // Estimated time to full charge (ms)
bool timerRunning = false;                // Variable for check if the timer is running or not
char timeString[9];                       // Time count up ( Time Used: )
char remainingTimeString[9];              // Estimated time to full charge (ms) (Time Left: )
char finalTimeString[9] = "00:00:00";     // Holds the last charging session time

/*#######################################################*/
/* Variable for calculate the estimated time based on 
   initial voltage reading before charging*/
/*#######################################################*/
float initial_voltage = 0.0;                // Initial voltage when charging starts
const float battery_capacity_mAh = 2200.0;  // Battery capacity in mAh
const float target_voltage = 8.4;           // Target full charge voltage
const float charging_efficiency = 0.9;      // 90% efficiency

/*#######################################################*/
/* Variable for current sensor */
/*#######################################################*/
int sensitivity = 66;
int adcValue = 0;
int offsetVoltage = 2500;
double adcVoltage = 0;
double currentValue = 0;

/*#######################################################*/
/* Variable for voltage sensor */
/*#######################################################*/
float adc_voltage = 0.0;
float in_voltage = 0.0;
float R1 = 30000.0;
float R2 = 7500.0;
float ref_voltage = 5.0;
int adc_value = 0;

/*#######################################################*/
/* Smoothing reading variable for read analog pin for 
   voltage and current sensor */
/*#######################################################*/
float filtered_voltage = 0.0;
float filtered_current = 0.0;
const float alpha = 0.1;  // Smoothing factor

/*#######################################################*/
/* Function to calculate power */
/*#######################################################*/
float calculate_power(float V, float I) {
  return V * I;
}

/*#######################################################*/
/* Function to get the current sensor reading */
/*#######################################################*/
void read_current() {
  adcValue = analogRead(current_sensor_pin);

  /******************************************************************/
  // Filtered current sensor reading
  /******************************************************************/
  adcVoltage = (adcValue / 1024.0) * 5200;
  currentValue = ((adcVoltage - offsetVoltage) / sensitivity);
  filtered_current = (alpha * currentValue) + ((1 - alpha) * filtered_current);
}

/*#######################################################*/
/* Function to get the voltage sensor reading*/
/*#######################################################*/
void read_voltage() {
  adc_value = analogRead(voltage_sensor_pin);

  /******************************************************************/
  // Filtered voltage sensor reading
  /******************************************************************/
  adc_voltage = (adc_value * ref_voltage) / 1024.0;
  in_voltage = adc_voltage * (R1 + R2) / R2;
  filtered_voltage = (alpha * in_voltage) + ((1 - alpha) * filtered_voltage);

  if (lock_card == 1) {
    if (filtered_voltage > 7.90 && lock_charging == 0) {
      digitalWrite(relay_charging_pin, LOW);
      Serial.println("Battery Full");
      digitalWrite(buzzer_pin, HIGH);
      delay(100);
      digitalWrite(buzzer_pin, LOW);

      sprintf(timeString, "00:00:00");  // Reset Time Used:
      total_price = base_price;         // Reset price
      total_energy_watt_minutes = 0.0;  // Reset energy
      lock_charging = 0;                // Reset charging
      lock_card = 0;                    // Reset NFC
      timerRunning = false;             // Stop the timer

      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("     BATTERY   ");
      lcd.setCursor(0, 2);
      lcd.print("      FULL   ");
      delay(2000);
    }
    /******************************************************************/
    else if (filtered_voltage < target_voltage && lock_charging == 0) {
      digitalWrite(relay_charging_pin, HIGH);
      Serial.println("Charging...");
      lock_charging = 1;

      count_time = millis();               // Start the timer count millis
      initial_voltage = filtered_voltage;  // Capture initial voltage
      last_update_time = count_time;       // Initialize energy tracking
      timerRunning = true;                 // Start timer tracking

      total_price = base_price;         // Reset price
      total_energy_watt_minutes = 0.0;  // Reset energy

      /******************************************************************/
      // Estimate charging time (in seconds) Formula
      /******************************************************************/
      if (filtered_current > 0) {  // Prevent division by zero
        float remaining_capacity_mAh = battery_capacity_mAh * (target_voltage - initial_voltage) / target_voltage;
        estimated_charge_time = (remaining_capacity_mAh / (filtered_current * 1000)) * 3600 * 1000 / charging_efficiency;
      } else {
        estimated_charge_time = 0;  // Default if current is zero
      }
      Serial.print("Estimated charge time (s): ");
      Serial.println(estimated_charge_time / 1000);
    }
    /******************************************************************/
    else if (lock_charging == 1) {
      // Check if estimated time has elapsed
      if (millis() - count_time >= estimated_charge_time) {
        digitalWrite(relay_charging_pin, LOW);
        Serial.println("Charging Complete (Time-based)");
        digitalWrite(buzzer_pin, HIGH);
        delay(100);
        digitalWrite(buzzer_pin, LOW);

        strcpy(finalTimeString, timeString);  // Save final used time
        lock_charging = 0;                    // Reset charging
        lock_card = 0;                        // Reset NFC
        timerRunning = false;                 // Stop the timer

        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("     CHARGING   ");
        lcd.setCursor(0, 2);
        lcd.print("     COMPLETE   ");
        delay(2000);
      }

      /******************************************************************/
      // Turn off charger if no battery is plug in
      /******************************************************************/
      else if (filtered_voltage > 8.79) {
        digitalWrite(relay_charging_pin, LOW);
        Serial.println("No Battery");
        digitalWrite(buzzer_pin, HIGH);
        delay(100);
        digitalWrite(buzzer_pin, LOW);

        sprintf(timeString, "00:00:00");  // Reset Time Used:
        total_price = base_price;         // Reset price
        total_energy_watt_minutes = 0.0;  // Reset energy
        lock_charging = 0;                // Reset charging
        lock_card = 0;                    // Reset NFC
        timerRunning = false;             // Stop the timer

        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("        NO   ");
        lcd.setCursor(0, 2);
        lcd.print("      BATTERY   ");
        delay(2000);
      }
    }
    /******************************************************************/
  }
}

/*#######################################################*/
/* Function to read the card UID 
   @lock_card = 1 : if valid card detected
   @lock_card = 0 : no valid card present */
/*#######################################################*/
void read_card() {
  uint8_t success;
  uint8_t uid[7] = { 0 };  // Buffer to store UID
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50);

  if (success) {
    Serial.println("Found an NFC card!");
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(" 0x");
      Serial.print(uid[i], HEX);
    }
    Serial.println("");

    if (uidLength == targetUIDLength) {
      valid = true;
      for (uint8_t i = 0; i < uidLength; i++) {
        if (uid[i] != targetUID[i]) {
          valid = false;
          break;
        }
      }

      /******************************************************************/
      if (valid) {
        Serial.println("Authorized card detected!");
        digitalWrite(buzzer_pin, HIGH);
        delay(100);
        digitalWrite(buzzer_pin, LOW);

        strcpy(finalTimeString, "00:00:00");
        lock_card = 1;
      } else {
        Serial.println("Unauthorized card! UID does not valid.");
        digitalWrite(relay_charging_pin, LOW);
        digitalWrite(buzzer_pin, HIGH);
        delay(100);
        digitalWrite(buzzer_pin, LOW);

        sprintf(timeString, "00:00:00");  // Reset Time Used:
        lock_card = 0;                    // Reset NFC
        lock_charging = 0;                // Stop Charging
        timerRunning = false;             // Stop time
        total_price = base_price;         // Reset price
        total_energy_watt_minutes = 0.0;  // Reset Energy
        count_time = 0;                   // Reset timer
        last_update_time = 0;             // Reset energy tracking
      }
      /******************************************************************/
    }
    /******************************************************************/
    else {
      Serial.println("Unauthorized card! UID length does not valid.");
      digitalWrite(relay_charging_pin, LOW);
      digitalWrite(buzzer_pin, HIGH);
      delay(100);
      digitalWrite(buzzer_pin, LOW);

      sprintf(timeString, "00:00:00");  // Reset Time Used:
      lock_card = 0;                    // Reset NFC
      lock_charging = 0;                // Stop Charging
      timerRunning = false;             // Stop time
      total_price = base_price;         // Reset price
      total_energy_watt_minutes = 0.0;  // Reset Energy
      count_time = 0;                   // Reset timer
      last_update_time = 0;             // Reset energy tracking
    }
    /******************************************************************/
  }
}
/*#######################################################*/
/* Function to tracking the time and update the price 
   based on the watt-minute */
/*#######################################################*/
void run_time() {
  if (timerRunning) {
    unsigned long current_time = millis();
    unsigned long elapsedMillis = current_time - count_time;
    unsigned long seconds = elapsedMillis / 1000;

    /******************************************************************/
    // Calculate energy (watt-minutes) for this interval
    /******************************************************************/
    unsigned long interval_ms = current_time - last_update_time;
    float interval_minutes = interval_ms / 60000.0;                           // Convert ms to minutes
    float power_watts = calculate_power(filtered_voltage, filtered_current);  // Power in watts
    total_energy_watt_minutes += power_watts * interval_minutes;              // Accumulate energy
    last_update_time = current_time;                                          // Update for next interval

    /******************************************************************/
    // Update price based on energy used Formula
    /******************************************************************/
    total_price = base_price + (total_energy_watt_minutes * price_per_watt_minute);

    unsigned int hours = seconds / 3600;
    unsigned int minutes = (seconds % 3600) / 60;
    unsigned int secs = seconds % 60;

    sprintf(timeString, "%02d:%02d:%02d", hours, minutes, secs);  // Time Used:

    /******************************************************************/
    // Calculate remaining time
    /******************************************************************/
    unsigned long remainingMillis = (estimated_charge_time > elapsedMillis) ? (estimated_charge_time - elapsedMillis) : 0;
    unsigned long remainingSeconds = remainingMillis / 1000;
    unsigned int r_hours = remainingSeconds / 3600;
    unsigned int r_minutes = (remainingSeconds % 3600) / 60;
    unsigned int r_secs = remainingSeconds % 60;

    sprintf(remainingTimeString, "%02d:%02d:%02d", r_hours, r_minutes, r_secs);  // Time Left:

    /******************************************************************/
    // Display
    /******************************************************************/
    Serial.print("Time: ");
    Serial.print(timeString);
    Serial.print(", Energy: ");
    Serial.print(total_energy_watt_minutes, 2);
    Serial.println(" Wm");
  } else {
    strcpy(timeString, finalTimeString);
    // sprintf(timeString, "00:00:00");              // Reset Time Used:
    sprintf(remainingTimeString, "00:00:00");  // Reset Time Left:
  }
}

/*#######################################################*/
/* Function to display in the LCD Display (20x4)
   Format > Time Used:00:00:00
            Time Left:00:00:00
            V:0.00  C:0.00
            Wm:0.00 RM:0.00 */
/*#######################################################*/
void display() {
  lcd.setCursor(0, 0);
  lcd.print("Time Used:");
  lcd.print(timeString);
  lcd.print("  ");  //Clear remaining character

  lcd.setCursor(0, 1);
  lcd.print("Time Left:");
  lcd.print(remainingTimeString);
  lcd.print("  ");  //Clear remaining character

  lcd.setCursor(0, 2);
  lcd.print("V:");
  lcd.print(filtered_voltage, 2);
  lcd.print("V ");
  lcd.print("C:");
  lcd.print(filtered_current, 2);
  lcd.print("A ");

  lcd.setCursor(0, 3);
  lcd.print("W/m:");
  lcd.print(total_energy_watt_minutes, 2);
  lcd.print(" ");  //Clear remaining character
  lcd.print(" RM:");
  lcd.print(total_price, 2);
  lcd.print(" ");  //Clear remaining character
}

/*#######################################################*/
/* SETUP */
/*#######################################################*/
void setup() {
  Serial.begin(9600);
  pinMode(buzzer_pin, OUTPUT);
  pinMode(relay_charging_pin, OUTPUT);

  digitalWrite(relay_charging_pin, LOW);

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN532 module. Check connections.");
    while (1)
      ;
  }

  Serial.println("Found PN532 module.");
  nfc.SAMConfig();
  Serial.println("Waiting for an NFC card...");

  /******************************************************************/
  // lcd start up display
  /******************************************************************/
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("    Unfc Enabled");
  lcd.setCursor(0, 1);
  lcd.print("    EV Charging");
  lcd.setCursor(0, 2);
  lcd.print(" Calculation System");
  delay(1000);
  lcd.clear();
}

/*#######################################################*/
/* LOOP */
/*#######################################################*/
void loop() {
  read_card();
  read_voltage();
  read_current();
  run_time();
  display();
}