#include <Wire.h>
#include <Adafruit_PN532.h>
#include <LiquidCrystal_I2C.h>

// NFC Configuration
#define SDA_PIN A4
#define SCL_PIN A5
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// LCD Configuration
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Pin Definitions
const int buzzer_pin = 13;
const int relay_charging_pin = 7;
const int voltage_sensor_pin = A3;

// Charging Parameters
const float BATTERY_CAPACITY = 2600.0;         // mAh, 2S 18650
const float CHARGING_CURRENT = 2000.0;         // mA (2A from TP4056)
const float FULL_CHARGE_VOLTAGE = 8.0;         // Voltage threshold for 2S full charge
const unsigned long FULL_CHARGE_DELAY = 2000;  // 2 seconds for confirmation
const float MIN_VOLTAGE = 6.0;                 // 2S discharged voltage
const float MAX_VOLTAGE = 8.4;                 // 2S fully charged voltage

// Target UID for NFC
uint8_t targetUID[] = { 0x3, 0xE6, 0x8, 0x35 };  // Example UID
const uint8_t targetUIDLength = 4;               // Length of the target UID
bool valid = false;
bool charging_locked = false;             // Track if charging is locked
unsigned long estimated_charge_time = 0;  // Estimated time to full charge (ms)

// Full Charge Detection State
bool full_charge_detected = false;
unsigned long full_charge_time = 0;

// Relay Timer Variables
unsigned long relay_delay_start_time = 0;
bool relay_delay_active = false;

// Timer Variables
unsigned long count_time = 0;
bool timerRunning = false;
char timeString[9] = "00:00:00";

// Voltage Sensor Variables
float adc_voltage = 0.0;
float in_voltage = 0.0;
const float R1 = 30000.0;       // Voltage divider resistor
const float R2 = 7500.0;        // Voltage divider resistor
const float ref_voltage = 5.0;  // Arduino reference voltage
int adc_value = 0;
float filtered_voltage = 0.0;

// Smoothing Factor
const float alpha = 0.1;  // Smoothing factor for voltage sensor

// Stabilization Variables
unsigned long stabilization_start_time = 0;
bool waiting_for_stabilization = false;
bool voltage_captured = false;

// Placeholder for Current and Power (no current sensor)
float filtered_current = 0.0;  // Placeholder
float price = 0.0;             // Placeholder

// Calculate power (placeholder, returns 0.0 without current)
float calculate_power(float V, float I) {
  return 0.0;  // No current sensor, so power is 0
}

// Read voltage from sensor
void read_voltage() {
  adc_value = analogRead(voltage_sensor_pin);
  adc_voltage = (adc_value * ref_voltage) / 1024.0;
  in_voltage = adc_voltage * (R1 + R2) / R2;
  filtered_voltage = (alpha * in_voltage) + ((1 - alpha) * filtered_voltage);
}

// Estimate charging time based on initial voltage
unsigned long estimate_charge_time(float voltage) {
  // Clamp voltage between MIN_VOLTAGE and MAX_VOLTAGE
  if (voltage < MIN_VOLTAGE) voltage = MIN_VOLTAGE;
  if (voltage > MAX_VOLTAGE) voltage = MAX_VOLTAGE;

  // Estimate SoC (linear approximation)
  float soc = (voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE);  // 0 to 1
  float remaining_capacity = BATTERY_CAPACITY * (1.0 - soc);          // mAh
  float cc_time_hours = remaining_capacity / CHARGING_CURRENT;        // Hours for CC phase
  float total_time_hours = cc_time_hours * 1.4;                       // Add 40% for CV phase
  unsigned long total_time_ms = total_time_hours * 3600000;           // Convert to ms
  Serial.print("Initial Voltage: ");
  Serial.print(voltage, 2);
  Serial.print("V, Estimated SoC: ");
  Serial.print(soc * 100, 0);
  Serial.print("%, Charge Time: ");
  Serial.print(total_time_ms / 60000.0, 1);
  Serial.println(" minutes");

  // Display estimated time on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Est. Time: ");
  lcd.print(total_time_ms / 60000.0, 1);
  lcd.print(" min");
  lcd.setCursor(0, 1);
  lcd.print("Voltage: ");
  lcd.print(voltage, 2);
  lcd.print(" V");
  
  return total_time_ms;
}

// Read NFC card
void read_card() {
  uint8_t success;
  uint8_t uid[7] = { 0 };  // Buffer for UID
  uint8_t uidLength;

  // Only process card scans if not locked or to unlock with invalid card
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

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

      if (valid && !charging_locked) {
        Serial.println("Authorized card detected!");
        voltage_captured = false;  // Reset flag when new session begins
        charging_locked = true;
        waiting_for_stabilization = true;
        stabilization_start_time = millis();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Stabilizing...");
        digitalWrite(buzzer_pin, HIGH);  // Single beep
        delay(200);
        digitalWrite(buzzer_pin, LOW);
      } else if (!valid && charging_locked) {
        Serial.println("Unauthorized card! Stopping charging.");
        charging_locked = false;
        timerRunning = false;
        waiting_for_stabilization = false;
        full_charge_detected = false;
        digitalWrite(relay_charging_pin, LOW);  // Stop charging
        digitalWrite(buzzer_pin, HIGH);         // Double beep
        delay(200);
        digitalWrite(buzzer_pin, LOW);
        delay(200);
        digitalWrite(buzzer_pin, HIGH);
        delay(200);
        digitalWrite(buzzer_pin, LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Charging Stopped");
        delay(2000);  // Show stop message briefly
      } else if (!valid) {
        Serial.println("Unauthorized card! UID does not match.");
        digitalWrite(relay_charging_pin, LOW);
        digitalWrite(buzzer_pin, HIGH);  // Double beep
        delay(200);
        digitalWrite(buzzer_pin, LOW);
        delay(200);
        digitalWrite(buzzer_pin, HIGH);
        delay(200);
        digitalWrite(buzzer_pin, LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Invalid Card");
        delay(2000);  // Show invalid message briefly
      }
    } else {
      Serial.println("Unauthorized card! UID length does not match.");
      if (charging_locked) {
        charging_locked = false;
        timerRunning = false;
        waiting_for_stabilization = false;
        full_charge_detected = false;
        digitalWrite(relay_charging_pin, LOW);  // Stop charging
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Charging Stopped");
        delay(2000);  // Show stop message briefly
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Invalid Card");
        delay(2000);  // Show invalid message briefly
      }
      digitalWrite(buzzer_pin, HIGH);  // Double beep
      delay(200);
      digitalWrite(buzzer_pin, LOW);
      delay(200);
      digitalWrite(buzzer_pin, HIGH);
      delay(200);
      digitalWrite(buzzer_pin, LOW);
    }
  } else if (!charging_locked) {
    digitalWrite(relay_charging_pin, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Waiting for Card");
  }
}

// Manage charging timer
void run_time() {
  if (timerRunning) {
    unsigned long elapsedMillis = millis() - count_time;
    unsigned long seconds = elapsedMillis / 1000;

    unsigned int hours = seconds / 3600;
    unsigned int minutes = (seconds % 3600) / 60;
    unsigned int secs = seconds % 60;

    sprintf(timeString, "%02d:%02d:%02d", hours, minutes, secs);

    // Update LCD with original format
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    lcd.print(timeString);
    lcd.setCursor(0, 1);
    lcd.print("V: ");
    lcd.print(filtered_voltage, 1);
    lcd.print("V | C: ");
    lcd.print(filtered_current, 1);  // Placeholder: 0.0A
    lcd.print("A");
    lcd.setCursor(0, 2);
    lcd.print("Power: ");
    lcd.print(calculate_power(filtered_voltage, filtered_current), 1);  // Placeholder: 0.0W
    lcd.print("W");
    lcd.setCursor(0, 3);
    lcd.print("Total:RM ");
    lcd.print(price, 2);  // Placeholder: 0.00

    // Check if estimated charge time is reached
    if (elapsedMillis >= estimated_charge_time) {
      full_charge_detected = true;
      full_charge_time = millis();
      digitalWrite(relay_charging_pin, LOW);
      timerRunning = false;
      Serial.println("Estimated charge time reached");
    }
  }
}

// Confirm full charge with voltage sensor
void status() {
  if (full_charge_detected && (millis() - full_charge_time >= FULL_CHARGE_DELAY)) {
    if (filtered_voltage > FULL_CHARGE_VOLTAGE) {
      Serial.println("Battery confirmed full (voltage > 8.0V)");
      charging_locked = false;
      timerRunning = false;
      digitalWrite(relay_charging_pin, LOW);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Battery Full");
      lcd.setCursor(0, 1);
      lcd.print("Voltage: ");
      lcd.print(filtered_voltage, 1);
      lcd.print(" V");
      digitalWrite(buzzer_pin, HIGH);
      delay(500);
      digitalWrite(buzzer_pin, LOW);
      delay(2000);  // Show full message briefly
    } else {
      Serial.println("Full charge not confirmed (voltage <= 8.0V)");
      charging_locked = false;
      timerRunning = false;
      digitalWrite(relay_charging_pin, LOW);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Charge Stopped");
      lcd.setCursor(0, 1);
      lcd.print("Voltage Low: ");
      lcd.print(filtered_voltage, 1);
      lcd.print(" V");
      digitalWrite(buzzer_pin, HIGH);
      delay(200);
      digitalWrite(buzzer_pin, LOW);
      delay(200);
      digitalWrite(buzzer_pin, HIGH);
      delay(200);
      digitalWrite(buzzer_pin, LOW);
      delay(2000);  // Show error message briefly
    }
    full_charge_detected = false;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(buzzer_pin, OUTPUT);
  pinMode(relay_charging_pin, OUTPUT);
  pinMode(voltage_sensor_pin, INPUT);
  digitalWrite(relay_charging_pin, LOW);

  // Initialize NFC module
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN532 module. Check connections.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PN532 Error");
    digitalWrite(buzzer_pin, HIGH);
    delay(1000);
    digitalWrite(buzzer_pin, LOW);
    while (1)
      ;  // Halt execution
  }

  Serial.println("Found PN532 module.");
  nfc.SAMConfig();
  Serial.println("Waiting for an NFC card...");

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NFC Enabled");
  lcd.setCursor(0, 1);
  lcd.print("EV Charging");
  lcd.setCursor(0, 2);
  lcd.print("System Ready");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for Card");
}

void loop() {
  read_voltage();
  read_card();

  if (charging_locked) {
    // Handle stabilization phase (5 seconds)
    if (waiting_for_stabilization && !voltage_captured && millis() - stabilization_start_time >= 5000) {
      // Capture voltage and calculate estimated charge time
      estimated_charge_time = estimate_charge_time(filtered_voltage);
      Serial.print("Stable Voltage captured: ");
      Serial.println(filtered_voltage, 2);
      voltage_captured = true;  // Mark voltage as captured
      relay_delay_active = true;  // Start relay delay
      relay_delay_start_time = millis();  // Record relay delay start time
      waiting_for_stabilization = false;  // End stabilization phase
      delay(2000);  // Show estimated time for 2 seconds
    }

    // Handle relay delay (3 seconds after stabilization)
    if (relay_delay_active && millis() - relay_delay_start_time >= 3000 && !waiting_for_stabilization) {
      digitalWrite(relay_charging_pin, HIGH);  // Turn on the relay
      timerRunning = true;
      count_time = millis();
      relay_delay_active = false;
      Serial.println("Relay ON after 3s delay.");
    }

    // Show voltage during stabilization phase
    if (waiting_for_stabilization) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Stabilizing...");
      lcd.setCursor(0, 1);
      lcd.print("Voltage: ");
      lcd.print(filtered_voltage, 1);
      lcd.print(" V");
    }

    // Run timer
    if (timerRunning) {
      run_time();
    }

    // Check for full charge
    if (full_charge_detected) {
      status();
    }
  }

  if (waiting_for_stabilization) {
    Serial.print("Stabilizing Voltage: ");
    Serial.println(filtered_voltage, 2);
  }
}