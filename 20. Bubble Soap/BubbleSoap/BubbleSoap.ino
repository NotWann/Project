#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

#define SENSOR1_PIN A12 // Analog pin for first sensor Vo
#define SENSOR2_PIN A13 // Analog pin for second sensor Vo
#define SAMPLES 10      // Number of samples for averaging

// Lookup table: {voltage, distance in cm} for low-reflectivity objects
float lookupTable[][2] = {
  {0.19, 80}, {0.39, 60}, {0.52, 40}, {0.77, 30}, {1.23, 20}, {2.3, 10}
};

// EDIT SECTION: Timer for pumps (use unsigned long to prevent overflow)
const unsigned long timer_1 = 41000UL;  // 41 seconds (Soap)
const unsigned long timer_2 = 94000UL;  // 1 minute 34 seconds
const unsigned long timer_3 = 160000UL; // 2 minute 40 seconds
const unsigned long timer_4 = 41000UL;  // 41 seconds (Pewangi)
const unsigned long timer_5 = 94000UL;  // 1 minute 34 seconds
const unsigned long timer_6 = 160000UL; // 2 minute 40 seconds

// Indicator Tank Levels
const int soap_level_low = 15;   // Lowest level
const int soap_level_high = 30;  // Highest level
const int pewangi_level_low = 15;  // Lowest level
const int pewangi_level_high = 30; // Highest level

// Pinout for push buttons
const int buttonPin_select_S = 30;
const int buttonPin_select_P = 31;
const int buttonPin_Start = 32;
const int buttonPin_Pos = 33;

// Pinout for LED push buttons
const int led_select_S = 34;
const int led_select_P = 35;
const int led_Start = 36;
const int led_Pos = 37;

// Pinout for LED traffic light indicators
const int soap_indicator_G = 5;
const int soap_indicator_Y = 6;
const int soap_indicator_R = 7;
const int pewangi_indicator_G = 9;
const int pewangi_indicator_Y = 10;
const int pewangi_indicator_R = 11;

// Pinout for LED timer indicators
const int soap_select_button_indicator1 = 38;
const int soap_select_button_indicator2 = 39;
const int soap_select_button_indicator3 = 40;
const int pewangi_select_button_indicator1 = 42;
const int pewangi_select_button_indicator2 = 43;
const int pewangi_select_button_indicator3 = 44;

// Buzzer and relay pinouts
const int buzzerPin = 13;
const int e18_sensor = 26;
const int relay_pump_soap = 22;
const int relay_pump_pewangi = 23;

int soap_menu = 0;
int pewangi_menu = 0;
int lock_Start = 0;
int lock_rfid_reading = 0;
int lock_distance = 0;
bool pump_completed = false;

unsigned long lastDebounceTime_S = 0;
unsigned long lastDebounceTime_P = 0;
unsigned long lastDebounceTime_System = 0;
const unsigned long debounceDelay = 50; // Increased to 200ms

unsigned long pump_on_duration_soap = 0;
unsigned long pump_on_duration_pewangi = 0;
unsigned long pump_start_time_soap = 0;
unsigned long pump_start_time_pewangi = 0;
bool pump_active_soap = false;
bool pump_active_pewangi = false;

int lastState_S = HIGH;
int lastState_P = HIGH;
int lastState_Start = HIGH;
int lastState_Pos = HIGH;

unsigned long buzzerStartTime = 0;
bool buzzerActive = false;

bool button_paused_soap = false;
bool button_paused_pewangi = false;
bool sensor_paused_soap = false;
bool sensor_paused_pewangi = false;
bool allow_resume_soap = false;
bool allow_resume_pewangi = false;
unsigned long remaining_duration_soap = 0;
unsigned long remaining_duration_pewangi = 0;

// LCD variables
int card_id = 0;
int set = 0;
int status = 0;
int timer_set[6];

unsigned long lastDisplayTime = 0;
const unsigned long displayInterval = 1000; // Display every 1 second

float interpolate(float voltage) {
  int tableSize = sizeof(lookupTable) / sizeof(lookupTable[0]);
  if (voltage <= lookupTable[0][0]) return lookupTable[0][1];
  if (voltage >= lookupTable[tableSize-1][0]) return lookupTable[tableSize-1][1];
  for (int i = 0; i < tableSize-1; i++) {
    if (voltage >= lookupTable[i][0] && voltage <= lookupTable[i+1][0]) {
      float v0 = lookupTable[i][0], v1 = lookupTable[i+1][0];
      float d0 = lookupTable[i][1], d1 = lookupTable[i+1][1];
      return d0 + (voltage - v0) * (d1 - d0) / (v1 - v0);
    }
  }
  return 10; // Default fallback (10 cm)
}

void sharp_sensor() {
  long sum1 = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum1 += analogRead(SENSOR1_PIN);
    delay(5);
  }
  float analogValue1 = sum1 / (float)SAMPLES;
  float voltage1 = analogValue1 * (5.0 / 1023.0);
  float distance1 = interpolate(voltage1);
  if (distance1 < 10) distance1 = 10;
  if (distance1 > 80) distance1 = 80;

  long sum2 = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum2 += analogRead(SENSOR2_PIN);
    delay(5);
  }
  float analogValue2 = sum2 / (float)SAMPLES;
  float voltage2 = analogValue2 * (5.0 / 1023.0);
  float distance2 = interpolate(voltage2);
  if (distance2 < 10) distance2 = 10;
  if (distance2 > 80) distance2 = 80;

  if (distance1 < soap_level_low) {
    digitalWrite(soap_indicator_G, HIGH);
    digitalWrite(soap_indicator_Y, LOW);
    digitalWrite(soap_indicator_R, LOW);
  } else if (distance1 > soap_level_low && distance1 < soap_level_high) {
    digitalWrite(soap_indicator_G, LOW);
    digitalWrite(soap_indicator_Y, HIGH);
    digitalWrite(soap_indicator_R, LOW);
  } else if (distance1 > soap_level_high) {
    digitalWrite(soap_indicator_G, LOW);
    digitalWrite(soap_indicator_Y, LOW);
    digitalWrite(soap_indicator_R, HIGH);
  }

  if (distance2 < pewangi_level_low) {
    digitalWrite(pewangi_indicator_G, HIGH);
    digitalWrite(pewangi_indicator_Y, LOW);
    digitalWrite(pewangi_indicator_R, LOW);
  } else if (distance2 > pewangi_level_low && distance2 < pewangi_level_high) {
    digitalWrite(pewangi_indicator_G, LOW);
    digitalWrite(pewangi_indicator_Y, HIGH);
    digitalWrite(pewangi_indicator_R, LOW);
  } else if (distance2 > pewangi_level_high) {
    digitalWrite(pewangi_indicator_G, LOW);
    digitalWrite(pewangi_indicator_Y, LOW);
    digitalWrite(pewangi_indicator_R, HIGH);
  }

  /*Serial.println("=== Sensor Readings ===");
  Serial.print("Soap: ");
  Serial.print(distance1);
  Serial.println(" cm");
  Serial.print("Pewangi: ");
  Serial.print(distance2);
  Serial.println(" cm");
  Serial.println();*/
  delay(25);
}

void select_button() {
  unsigned long currentMillis = millis();
  if (lock_rfid_reading == 1) {
    if (currentMillis - lastDebounceTime_S >= debounceDelay) {
      int currentState_S = digitalRead(buttonPin_select_S);
      if (lastState_S == LOW && currentState_S == HIGH) {
        soap_menu = (soap_menu + 1) % 4;
        set = 1;
        pewangi_menu = 0;
        pump_on_duration_pewangi = 0;
        pump_active_pewangi = false;
        button_paused_pewangi = false;
        sensor_paused_pewangi = false;
        allow_resume_pewangi = false;
        remaining_duration_pewangi = 0;
        if (soap_menu == 0) pump_on_duration_soap = 0;
        else if (soap_menu == 1) pump_on_duration_soap = timer_1;
        else if (soap_menu == 2) pump_on_duration_soap = timer_2;
        else if (soap_menu == 3) pump_on_duration_soap = timer_3;
        char timeStr[6];
        int total_seconds = pump_on_duration_soap / 1000;
        sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
        Serial.print("Soap Menu: ");
        Serial.println(soap_menu);
        Serial.print("Soap Pump Duration: ");
        Serial.print(timeStr);
        Serial.println(" (MM:SS)");
        Serial.println("Pewangi Menu: Reset to 0");
        digitalWrite(led_select_S, HIGH);
        digitalWrite(led_select_P, LOW);
        sound();
        lastDebounceTime_S = currentMillis;
      }
      if (currentState_S != lastState_S) {
        lastDebounceTime_S = currentMillis;
        lastState_S = currentState_S;
      }
    }

    if (currentMillis - lastDebounceTime_P >= debounceDelay) {
      int currentState_P = digitalRead(buttonPin_select_P);
      if (lastState_P == LOW && currentState_P == HIGH) {
        pewangi_menu = (pewangi_menu + 1) % 4;
        set = 2;
        soap_menu = 0;
        pump_on_duration_soap = 0;
        pump_active_soap = false;
        button_paused_soap = false;
        sensor_paused_soap = false;
        allow_resume_soap = false;
        remaining_duration_soap = 0;
        if (pewangi_menu == 0) pump_on_duration_pewangi = 0;
        else if (pewangi_menu == 1) pump_on_duration_pewangi = timer_4;
        else if (pewangi_menu == 2) pump_on_duration_pewangi = timer_5;
        else if (pewangi_menu == 3) pump_on_duration_pewangi = timer_6;
        char timeStr[6];
        int total_seconds = pump_on_duration_pewangi / 1000;
        sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
        Serial.print("Pewangi Menu: ");
        Serial.println(pewangi_menu);
        Serial.print("Pewangi Pump Duration: ");
        Serial.print(timeStr);
        Serial.println(" (MM:SS)");
        Serial.println("Soap Menu: Reset to 0");
        digitalWrite(led_select_P, HIGH);
        digitalWrite(led_select_S, LOW);
        sound();
        lastDebounceTime_P = currentMillis;
      }
      if (currentState_P != lastState_P) {
        lastDebounceTime_P = currentMillis;
        lastState_P = currentState_P;
      }
    }
  }
}

void update_menu_indicators() {
  if (soap_menu == 1) {
    digitalWrite(soap_select_button_indicator1, HIGH);
    digitalWrite(soap_select_button_indicator2, LOW);
    digitalWrite(soap_select_button_indicator3, LOW);
  } else if (soap_menu == 2) {
    digitalWrite(soap_select_button_indicator1, LOW);
    digitalWrite(soap_select_button_indicator2, HIGH);
    digitalWrite(soap_select_button_indicator3, LOW);
  } else if (soap_menu == 3) {
    digitalWrite(soap_select_button_indicator1, LOW);
    digitalWrite(soap_select_button_indicator2, LOW);
    digitalWrite(soap_select_button_indicator3, HIGH);
  } else {
    digitalWrite(soap_select_button_indicator1, LOW);
    digitalWrite(soap_select_button_indicator2, LOW);
    digitalWrite(soap_select_button_indicator3, LOW);
  }

  if (pewangi_menu == 1) {
    digitalWrite(pewangi_select_button_indicator1, HIGH);
    digitalWrite(pewangi_select_button_indicator2, LOW);
    digitalWrite(pewangi_select_button_indicator3, LOW);
  } else if (pewangi_menu == 2) {
    digitalWrite(pewangi_select_button_indicator1, LOW);
    digitalWrite(pewangi_select_button_indicator2, HIGH);
    digitalWrite(pewangi_select_button_indicator3, LOW);
  } else if (pewangi_menu == 3) {
    digitalWrite(pewangi_select_button_indicator1, LOW);
    digitalWrite(pewangi_select_button_indicator2, LOW);
    digitalWrite(pewangi_select_button_indicator3, HIGH);
  } else {
    digitalWrite(pewangi_select_button_indicator1, LOW);
    digitalWrite(pewangi_select_button_indicator2, LOW);
    digitalWrite(pewangi_select_button_indicator3, LOW);
  }
}

void system_button() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastDebounceTime_System >= debounceDelay) {
    int currentState_Start = digitalRead(buttonPin_Start);
    if (currentState_Start == LOW && lastState_Start == HIGH) {
      Serial.println("Start Button Pressed");
      if (lock_Start == 0) {
        Serial.println("Start System");
        digitalWrite(led_Start, HIGH);
        digitalWrite(led_Pos, LOW);
        status = 0; // WAITING
        sound();
        lock_Start = 1;
      } else if ((button_paused_soap || sensor_paused_soap || button_paused_pewangi || sensor_paused_pewangi) && lock_Start == 1) {
        Serial.println("Allowing Pump Resume");
        digitalWrite(led_Start, HIGH);
        digitalWrite(led_Pos, LOW);
        status = 3; // PAUSE (waiting for object)
        sound();
        if (button_paused_soap || sensor_paused_soap) {
          char timeStr[6];
          int total_seconds = remaining_duration_soap / 1000;
          sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
          Serial.print("Soap Ready to Resume, Waiting for Object, Remaining: ");
          Serial.print(timeStr);
          Serial.println(" (MM:SS)");
          // Check if object is still detected
          if (digitalRead(e18_sensor) == LOW && soap_menu >= 1 && soap_menu <= 3 && !pump_active_soap && !pump_active_pewangi) {
            digitalWrite(relay_pump_soap, HIGH);
            digitalWrite(relay_pump_pewangi, LOW);
            pump_start_time_soap = millis();
            pump_active_soap = true;
            button_paused_soap = false;
            sensor_paused_soap = false;
            status = 1; // FILLING
            char timeStr[6];
            int total_seconds = remaining_duration_soap / 1000;
            sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
            Serial.print("Soap Pump Resumed Immediately: ");
            Serial.print(timeStr);
            Serial.println(" (MM:SS)");
          }
        } else if (button_paused_pewangi || sensor_paused_pewangi) {
          char timeStr[6];
          int total_seconds = remaining_duration_pewangi / 1000;
          sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
          Serial.print("Pewangi Ready to Resume, Waiting for Object, Remaining: ");
          Serial.print(timeStr);
          Serial.println(" (MM:SS)");
          // Check if object is still detected
          if (digitalRead(e18_sensor) == LOW && pewangi_menu >= 1 && pewangi_menu <= 3 && !pump_active_soap && !pump_active_pewangi) {
            digitalWrite(relay_pump_soap, LOW);
            digitalWrite(relay_pump_pewangi, HIGH);
            pump_start_time_pewangi = millis();
            pump_active_pewangi = true;
            button_paused_pewangi = false;
            sensor_paused_pewangi = false;
            status = 1; // FILLING
            char timeStr[6];
            int total_seconds = remaining_duration_pewangi / 1000;
            sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
            Serial.print("Pewangi Pump Resumed Immediately: ");
            Serial.print(timeStr);
            Serial.println(" (MM:SS)");
          }
        }
      }
      lastDebounceTime_System = currentMillis;
    }
    lastState_Start = currentState_Start;

    int currentState_Pos = digitalRead(buttonPin_Pos);
    if (currentState_Pos == LOW && lastState_Pos == HIGH && lock_Start == 1) {
      Serial.println("Pause Button Pressed");
      if (pump_active_soap || pump_active_pewangi) {
        Serial.println("Pause Pump");
        digitalWrite(led_Start, LOW);
        digitalWrite(led_Pos, HIGH);
        status = 3; // PAUSE
        sound();
        if (pump_active_soap) {
          unsigned long elapsed = millis() - pump_start_time_soap;
          remaining_duration_soap = (elapsed < remaining_duration_soap) ? (remaining_duration_soap - elapsed) : 0;
          digitalWrite(relay_pump_soap, LOW);
          pump_active_soap = false;
          button_paused_soap = true;
          sensor_paused_soap = false;
          char timeStr[6];
          int total_seconds = remaining_duration_soap / 1000;
          sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
          Serial.print("Soap Pump Paused by Button, Remaining: ");
          Serial.print(timeStr);
          Serial.println(" (MM:SS)");
        } else if (pump_active_pewangi) {
          unsigned long elapsed = millis() - pump_start_time_pewangi;
          remaining_duration_pewangi = (elapsed < remaining_duration_pewangi) ? (remaining_duration_pewangi - elapsed) : 0;
          digitalWrite(relay_pump_pewangi, LOW);
          pump_active_pewangi = false;
          button_paused_pewangi = true;
          sensor_paused_pewangi = false;
          char timeStr[6];
          int total_seconds = remaining_duration_pewangi / 1000;
          sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
          Serial.print("Pewangi Pump Paused by Button, Remaining: ");
          Serial.print(timeStr);
          Serial.println(" (MM:SS)");
        }
      } else if (button_paused_soap || sensor_paused_soap || button_paused_pewangi || sensor_paused_pewangi) {
        Serial.println("Pump Already Paused");
        digitalWrite(led_Start, LOW);
        digitalWrite(led_Pos, HIGH);
        status = 3; // PAUSE
        sound();
        char timeStr[6];
        int total_seconds = remaining_duration_soap / 1000;
        sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
        Serial.print("Soap Remaining: ");
        Serial.print(timeStr);
        Serial.println(" (MM:SS)");
        total_seconds = remaining_duration_pewangi / 1000;
        sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
        Serial.print("Pewangi Remaining: ");
        Serial.print(timeStr);
        Serial.println(" (MM:SS)");
      } else {
        Serial.println("Stop System");
        digitalWrite(led_Start, LOW);
        digitalWrite(led_Pos, HIGH);
        status = 0; // WAITING
        sound();
        lock_Start = 0;
        digitalWrite(led_select_S, LOW);
        digitalWrite(led_select_P, LOW);
        digitalWrite(relay_pump_soap, LOW);
        digitalWrite(relay_pump_pewangi, LOW);
        button_paused_soap = false;
        button_paused_pewangi = false;
        sensor_paused_soap = false;
        sensor_paused_pewangi = false;
        remaining_duration_soap = 0;
        remaining_duration_pewangi = 0;
        soap_menu = 0;
        pewangi_menu = 0;
        pump_on_duration_soap = 0;
        pump_on_duration_pewangi = 0;
        set = 0;
        card_id = 0;
      }
      lastDebounceTime_System = currentMillis;
    }
    lastState_Pos = currentState_Pos;
  }
}

// Function read rfid for Nano via uart
void read_rfid_value() {
  if (Serial1.available() > 0) {
    byte receivedValue = Serial1.read();
    Serial.print(F("Received value: "));
    Serial.println(receivedValue);
    if (receivedValue == 0) {
      sound();
      Serial.println(F("Card Detected! (UID: FC5E9832)"));
      lcd.clear();
      card_id = 1;
      lock_rfid_reading = 1;
      status = 0; // Set to WAITING
    } else if (receivedValue == 1) {
      sound();
      Serial.println(F("Card Detected! (UID: B3A3CD4F)"));
      lcd.clear();
      card_id = 2;
      lock_rfid_reading = 1;
      status = 0; // Set to WAITING
    } else {
      Serial.println(F("Unknown value received, ignoring."));
      card_id = 0;
      lock_rfid_reading = 0;
      status = 0; // Set to WAITING
    }
    delay(100);
  }
}

void read_distance_e18() {
  if (lock_rfid_reading == 1 && lock_Start == 1) {
    int state = digitalRead(e18_sensor);
    if (state == LOW && lock_distance == 0) {
      Serial.println("Object Detected");
      digitalWrite(buzzerPin, HIGH);
      lock_distance = 1;
      if (pump_completed) {
        pump_completed = false; // Allow new pump cycle
      }
      // Start or resume soap pump if not button-paused or allowed to resume
      if (soap_menu >= 1 && soap_menu <= 3 && !pump_active_soap && !pump_active_pewangi && (!button_paused_soap || allow_resume_soap)) {
        digitalWrite(relay_pump_soap, HIGH);
        digitalWrite(relay_pump_pewangi, LOW);
        pump_start_time_soap = millis();
        pump_active_soap = true;
        button_paused_soap = false;
        sensor_paused_soap = false;
        allow_resume_soap = false;
        status = 1; // FILLING
        if (remaining_duration_soap == 0) {
          remaining_duration_soap = pump_on_duration_soap; // Initial start
          char timeStr[6];
          int total_seconds = remaining_duration_soap / 1000;
          sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
          Serial.print("Starting Soap Pump for: ");
          Serial.print(timeStr);
          Serial.println(" (MM:SS)");
        } else {
          char timeStr[6];
          int total_seconds = remaining_duration_soap / 1000;
          sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
          Serial.print("Resuming Soap Pump for: ");
          Serial.print(timeStr);
          Serial.println(" (MM:SS)");
        }
      }
      // Start or resume pewangi pump if not button-paused or allowed to resume
      else if (pewangi_menu >= 1 && pewangi_menu <= 3 && !pump_active_soap && !pump_active_pewangi && (!button_paused_pewangi || allow_resume_pewangi)) {
        digitalWrite(relay_pump_soap, LOW);
        digitalWrite(relay_pump_pewangi, HIGH);
        pump_start_time_pewangi = millis();
        pump_active_pewangi = true;
        button_paused_pewangi = false;
        sensor_paused_pewangi = false;
        allow_resume_pewangi = false;
        status = 1; // FILLING
        if (remaining_duration_pewangi == 0) {
          remaining_duration_pewangi = pump_on_duration_pewangi; // Initial start
          char timeStr[6];
          int total_seconds = remaining_duration_pewangi / 1000;
          sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
          Serial.print("Starting Pewangi Pump for: ");
          Serial.print(timeStr);
          Serial.println(" (MM:SS)");
        } else {
          char timeStr[6];
          int total_seconds = remaining_duration_pewangi / 1000;
          sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
          Serial.print("Resuming Pewangi Pump for: ");
          Serial.print(timeStr);
          Serial.println(" (MM:SS)");
        }
      }
    } else if (state == HIGH && lock_distance == 1) {
      Serial.println("All Clear");
      digitalWrite(buzzerPin, LOW);
      lock_distance = 0;
      if (pump_active_soap) {
        unsigned long elapsed = millis() - pump_start_time_soap;
        remaining_duration_soap = (elapsed < remaining_duration_soap) ? (remaining_duration_soap - elapsed) : 0;
        digitalWrite(relay_pump_soap, LOW);
        pump_active_soap = false;
        sensor_paused_soap = true;
        allow_resume_soap = false;
        status = 3; // PAUSE
        char timeStr[6];
        int total_seconds = remaining_duration_soap / 1000;
        sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
        Serial.print("Soap Pump Paused: Object Cleared, Remaining: ");
        Serial.print(timeStr);
        Serial.println(" (MM:SS)");
      }
      if (pump_active_pewangi) {
        unsigned long elapsed = millis() - pump_start_time_pewangi;
        remaining_duration_pewangi = (elapsed < remaining_duration_pewangi) ? (remaining_duration_pewangi - elapsed) : 0;
        digitalWrite(relay_pump_pewangi, LOW);
        pump_active_pewangi = false;
        sensor_paused_pewangi = true;
        allow_resume_pewangi = false;
        status = 3; // PAUSE
        char timeStr[6];
        int total_seconds = remaining_duration_pewangi / 1000;
        sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
        Serial.print("Pewangi Pump Paused: Object Cleared, Remaining: ");
        Serial.print(timeStr);
        Serial.println(" (MM:SS)");
      }
    }
    delay(10);
  }
}

void sound() {
  if (!buzzerActive) {
    digitalWrite(buzzerPin, HIGH);
    buzzerStartTime = millis();
    buzzerActive = true;
  }
}

void display() {
  lcd.setCursor(0, 0);
  if (card_id == 0) lcd.print("Card UID: NO CARD  ");
  else if (card_id == 1) lcd.print("Card UID: FC5E9832");
  else if (card_id == 2) lcd.print("Card UID: B3A3CD4F");

  lcd.setCursor(0, 1);
  if (set == 0) {
    lcd.print("Set: NONE          ");
  } else if (set == 1) {
    char timeStr[6];
    int total_seconds = pump_on_duration_soap / 1000;
    sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
    lcd.print("Set: Soap> ");
    lcd.print(timeStr);
    lcd.print("   ");
  } else if (set == 2) {
    char timeStr[6];
    int total_seconds = pump_on_duration_pewangi / 1000;
    sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
    lcd.print("Set: Pewangi> ");
    lcd.print(timeStr);
    lcd.print("   ");
  }

  lcd.setCursor(0, 2);
  if (pump_active_soap || button_paused_soap || sensor_paused_soap) {
    unsigned long remaining_ms = (pump_active_soap) ? 
      (millis() - pump_start_time_soap < remaining_duration_soap ? 
       (remaining_duration_soap - (millis() - pump_start_time_soap)) : 0) : 
      remaining_duration_soap;
    int total_seconds = remaining_ms / 1000;
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
    lcd.print("Time: ");
    lcd.print(timeStr);
    lcd.print("    ");
  } else if (pump_active_pewangi || button_paused_pewangi || sensor_paused_pewangi) {
    unsigned long remaining_ms = (pump_active_pewangi) ? 
      (millis() - pump_start_time_pewangi < remaining_duration_pewangi ? 
       (remaining_duration_pewangi - (millis() - pump_start_time_pewangi)) : 0) : 
      remaining_duration_pewangi;
    int total_seconds = remaining_ms / 1000;
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
    lcd.print("Time: ");
    lcd.print(timeStr);
    lcd.print("    ");
  } 
  else {
    lcd.print("Time: 00:00       ");
  }

  lcd.setCursor(0, 3);
  if (status == 0) lcd.print("STATUS: WAITING  ");
  else if (status == 1) lcd.print("STATUS: FILLING  ");
  else if (status == 2) lcd.print("STATUS: FINISH   ");
  else if (status == 3) lcd.print("STATUS: PAUSE    ");
}

// Function to return the selected timer value
int timer() {
  if (set == 1 && soap_menu >= 1 && soap_menu <= 3) {
    return timer_set[soap_menu - 1];
  } else if (set == 2 && pewangi_menu >= 1 && pewangi_menu <= 3) {
    return timer_set[pewangi_menu + 2];
  } else {
    return 0;
  }
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial);
  Serial.println(F("Arduino Mega ready to receive RFID value."));

  // Initialize timer_set array
  timer_set[0] = timer_1 / 1000; // Soap timer 1 (41s)
  timer_set[1] = timer_2 / 1000; // Soap timer 2 (94s)
  timer_set[2] = timer_3 / 1000; // Soap timer 3 (160s)
  timer_set[3] = timer_4 / 1000; // Pewangi timer 1 (41s)
  timer_set[4] = timer_5 / 1000; // Pewangi timer 2 (94s)
  timer_set[5] = timer_6 / 1000; // Pewangi timer 3 (160s)

  lcd.init();
  lcd.backlight();

  pinMode(buttonPin_select_S, INPUT_PULLUP);
  pinMode(buttonPin_select_P, INPUT_PULLUP);
  pinMode(buttonPin_Start, INPUT_PULLUP);
  pinMode(buttonPin_Pos, INPUT_PULLUP);
  pinMode(led_select_S, OUTPUT);
  pinMode(led_select_P, OUTPUT);
  pinMode(led_Start, OUTPUT);
  pinMode(led_Pos, OUTPUT);
  pinMode(soap_indicator_G, OUTPUT);
  pinMode(soap_indicator_Y, OUTPUT);
  pinMode(soap_indicator_R, OUTPUT);
  pinMode(pewangi_indicator_G, OUTPUT);
  pinMode(pewangi_indicator_Y, OUTPUT);
  pinMode(pewangi_indicator_R, OUTPUT);
  pinMode(soap_select_button_indicator1, OUTPUT);
  pinMode(soap_select_button_indicator2, OUTPUT);
  pinMode(soap_select_button_indicator3, OUTPUT);
  pinMode(pewangi_select_button_indicator1, OUTPUT);
  pinMode(pewangi_select_button_indicator2, OUTPUT);
  pinMode(pewangi_select_button_indicator3, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relay_pump_soap, OUTPUT);
  pinMode(relay_pump_pewangi, OUTPUT);
  pinMode(e18_sensor, INPUT);

  digitalWrite(led_select_S, LOW);
  digitalWrite(led_select_P, LOW);
  digitalWrite(led_Start, LOW);
  digitalWrite(led_Pos, LOW);
  digitalWrite(soap_indicator_G, LOW);
  digitalWrite(soap_indicator_Y, LOW);
  digitalWrite(soap_indicator_R, LOW);
  digitalWrite(pewangi_indicator_G, LOW);
  digitalWrite(pewangi_indicator_Y, LOW);
  digitalWrite(pewangi_indicator_R, LOW);
  digitalWrite(soap_select_button_indicator1, LOW);
  digitalWrite(soap_select_button_indicator2, LOW);
  digitalWrite(soap_select_button_indicator3, LOW);
  digitalWrite(pewangi_select_button_indicator1, LOW);
  digitalWrite(pewangi_select_button_indicator2, LOW);
  digitalWrite(pewangi_select_button_indicator3, LOW);
  digitalWrite(relay_pump_soap, LOW);
  digitalWrite(relay_pump_pewangi, LOW);
}

void loop() {
  read_rfid_value();
  select_button();
  system_button();
  read_distance_e18();
  update_menu_indicators();
  sharp_sensor();

  // Display remaining time every second
  unsigned long currentMillis = millis();
  if (currentMillis - lastDisplayTime >= displayInterval) {
    if (pump_active_soap || button_paused_soap || sensor_paused_soap) {
      unsigned long remaining_ms;
      if (pump_active_soap) {
        unsigned long elapsed = currentMillis - pump_start_time_soap;
        remaining_ms = (elapsed < remaining_duration_soap) ? (remaining_duration_soap - elapsed) : 0;
      } else {
        remaining_ms = remaining_duration_soap;
      }
      int total_seconds = remaining_ms / 1000;
      char timeStr[6];
      sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
      Serial.print("Soap Remaining: ");
      Serial.print(timeStr);
      Serial.println(" (MM:SS)");
    }
    if (pump_active_pewangi || button_paused_pewangi || sensor_paused_pewangi) {
      unsigned long remaining_ms;
      if (pump_active_pewangi) {
        unsigned long elapsed = currentMillis - pump_start_time_pewangi;
        remaining_ms = (elapsed < remaining_duration_pewangi) ? (remaining_duration_pewangi - elapsed) : 0;
      } else {
        remaining_ms = remaining_duration_pewangi;
      }
      int total_seconds = remaining_ms / 1000;
      char timeStr[6];
      sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
      Serial.print("Pewangi Remaining: ");
      Serial.print(timeStr);
      Serial.println(" (MM:SS)");
    }
    lastDisplayTime = currentMillis;
  }

  // Turn off pumps after duration
  if (pump_active_soap && millis() - pump_start_time_soap >= remaining_duration_soap) {
    digitalWrite(relay_pump_soap, LOW);
    digitalWrite(buzzerPin, LOW);
    set = 0;
    status = 2; // FINISH
    card_id = 0;
    pump_active_soap = false;
    pump_completed = true;
    soap_menu = 0;
    lock_rfid_reading = 0;
    lock_Start = 0;
    button_paused_soap = false;
    sensor_paused_soap = false;
    allow_resume_soap = false;
    remaining_duration_soap = 0;
    digitalWrite(led_select_S, LOW);
    digitalWrite(led_select_P, LOW);
    digitalWrite(led_Start, LOW);
    digitalWrite(led_Pos, HIGH);
    digitalWrite(soap_select_button_indicator1, LOW);
    digitalWrite(soap_select_button_indicator2, LOW);
    digitalWrite(soap_select_button_indicator3, LOW);
    Serial.println("Soap Pump: OFF (Duration Reached)");
  }
  if (pump_active_pewangi && millis() - pump_start_time_pewangi >= remaining_duration_pewangi) {
    digitalWrite(relay_pump_pewangi, LOW);
    digitalWrite(buzzerPin, LOW);
    set = 0;
    status = 2; // FINISH
    card_id = 0;
    pump_active_pewangi = false;
    pump_completed = true;
    pewangi_menu = 0;
    lock_rfid_reading = 0;
    lock_Start = 0;
    button_paused_pewangi = false;
    sensor_paused_pewangi = false;
    allow_resume_pewangi = false;
    remaining_duration_pewangi = 0;
    digitalWrite(led_select_S, LOW);
    digitalWrite(led_select_P, LOW);
    digitalWrite(led_Start, LOW);
    digitalWrite(led_Pos, HIGH);
    digitalWrite(pewangi_select_button_indicator1, LOW);
    digitalWrite(pewangi_select_button_indicator2, LOW);
    digitalWrite(pewangi_select_button_indicator3, LOW);
    Serial.println("Pewangi Pump: OFF (Duration Reached)");
  }

  if (buzzerActive && millis() - buzzerStartTime >= 100) {
    digitalWrite(buzzerPin, LOW);
    buzzerActive = false;
  }

  display();

  delay(5);
}