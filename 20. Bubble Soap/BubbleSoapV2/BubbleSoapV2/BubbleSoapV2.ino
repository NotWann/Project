#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Sensor and sampling constants
#define SENSOR1_PIN A12 // Analog pin for first sensor Vo (Soap)
#define SENSOR2_PIN A13 // Analog pin for second sensor Vo (Pewangi)
#define SAMPLES 10      // Number of samples for averaging

// Lookup table: {voltage, distance in cm} for low-reflectivity objects
float lookupTable[][2] = {
  {0.19, 80}, {0.39, 60}, {0.52, 40}, {0.77, 30}, {1.23, 20}, {2.3, 10}
};

// Timer constants for pumps (in milliseconds)
const unsigned long timer_1 = 41000UL;  // 41 seconds (Soap)
const unsigned long timer_2 = 94000UL;  // 1 minute 34 seconds
const unsigned long timer_3 = 160000UL; // 2 minutes 40 seconds
const unsigned long timer_4 = 41000UL;  // 41 seconds (Pewangi)
const unsigned long timer_5 = 94000UL;  // 1 minute 34 seconds
const unsigned long timer_6 = 160000UL; // 2 minutes 40 seconds

// Tank level thresholds
const int soap_level_low = 15;   // Lowest level (cm)
const int soap_level_high = 30;  // Highest level (cm)
const int pewangi_level_low = 15;  // Lowest level (cm)
const int pewangi_level_high = 30; // Highest level (cm)

// Pin assignments for push buttons
const int buttonPin_select_S = 30; // Soap selection
const int buttonPin_select_P = 31; // Pewangi selection
const int buttonPin_Start = 32;    // Start system
const int buttonPin_Pos = 33;      // Pause/Stop system

// Pin assignments for LED indicators
const int led_select_S = 34; // Soap selection LED
const int led_select_P = 35; // Pewangi selection LED
const int led_Start = 36;    // Start LED
const int led_Pos = 37;      // Pause/Stop LED

// Pin assignments for traffic light indicators
const int soap_indicator_G = 5;  // Soap green
const int soap_indicator_Y = 6;  // Soap yellow
const int soap_indicator_R = 7;  // Soap red
const int pewangi_indicator_G = 9;  // Pewangi green
const int pewangi_indicator_Y = 10; // Pewangi yellow
const int pewangi_indicator_R = 11; // Pewangi red

// Pin assignments for timer selection indicators
const int soap_select_button_indicator1 = 38; // Soap timer 1
const int soap_select_button_indicator2 = 39; // Soap timer 2
const int soap_select_button_indicator3 = 40; // Soap timer 3
const int pewangi_select_button_indicator1 = 42; // Pewangi timer 1
const int pewangi_select_button_indicator2 = 43; // Pewangi timer 2
const int pewangi_select_button_indicator3 = 44; // Pewangi timer 3

// Pin assignments for buzzer and relays
const int buzzerPin = 13;         // Buzzer
const int e18_sensor = 26;        // E18-D80NK sensor
const int relay_pump_soap = 22;   // Soap pump relay
const int relay_pump_pewangi = 23; // Pewangi pump relay

// State variables
int soap_menu = 0; // Soap timer selection (0-3)
int pewangi_menu = 0; // Pewangi timer selection (0-3)
int lock_Start = 0;   // System lock state
int lock_rfid_reading = 0; // RFID reading lock
int lock_distance = 0;     // Object detection lock
bool pump_completed = false; // Pump cycle completion flag
bool object_detected_first = false; // Object detected before start

// Debouncing variables
unsigned long lastDebounceTime_S = 0; // Soap button debounce
unsigned long lastDebounceTime_P = 0; // Pewangi button debounce
unsigned long lastDebounceTime_System = 0; // System button debounce
const unsigned long debounceDelay = 50; // Debounce delay (ms)

// Pump control variables
unsigned long pump_on_duration_soap = 0;    // Soap pump duration
unsigned long pump_on_duration_pewangi = 0; // Pewangi pump duration
unsigned long pump_start_time_soap = 0;     // Soap pump start time
unsigned long pump_start_time_pewangi = 0;  // Pewangi pump start time
bool pump_active_soap = false;              // Soap pump active
bool pump_active_pewangi = false;           // Pewangi pump active

// Button states
int lastState_S = HIGH;     // Last soap button state
int lastState_P = HIGH;     // Last pewangi button state
int lastState_Start = HIGH; // Last start button state
int lastState_Pos = HIGH;   // Last pause/stop button state

// Buzzer control
unsigned long buzzerStartTime = 0; // Buzzer activation time
bool buzzerActive = false;         // Buzzer state

// Pump pause/resume states
bool button_paused_soap = false;      // Soap paused by button
bool button_paused_pewangi = false;   // Pewangi paused by button
bool sensor_paused_soap = false;      // Soap paused by sensor
bool sensor_paused_pewangi = false;   // Pewangi paused by sensor
bool allow_resume_soap = false;       // Soap resume allowed
bool allow_resume_pewangi = false;    // Pewangi resume allowed
unsigned long remaining_duration_soap = 0;    // Soap remaining time
unsigned long remaining_duration_pewangi = 0; // Pewangi remaining time

// LCD display variables
int card_id = 0; // RFID card ID
int set = 0;     // Selected pump (0: none, 1: soap, 2: pewangi)
int status = 0;  // System status (0: waiting, 1: filling, 2: finish, 3: pause)
int timer_set[6]; // Timer values for soap and pewangi
unsigned long lastDisplayTime = 0; // Last display update time
const unsigned long displayInterval = 1000; // Display update interval (ms)

// Interpolates voltage to distance using lookup table
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

// Reads and processes sensor data for tank levels
void updateTankLevels() {
  long sum1 = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum1 += analogRead(SENSOR1_PIN);
    delay(5);
  }
  float analogValue1 = sum1 / (float)SAMPLES;
  float voltage1 = analogValue1 * (5.0 / 1023.0);
  float distance1 = interpolate(voltage1);
  distance1 = constrain(distance1, 10, 80);

  long sum2 = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum2 += analogRead(SENSOR2_PIN);
    delay(5);
  }
  float analogValue2 = sum2 / (float)SAMPLES;
  float voltage2 = analogValue2 * (5.0 / 1023.0);
  float distance2 = interpolate(voltage2);
  distance2 = constrain(distance2, 10, 80);

  digitalWrite(soap_indicator_G, distance1 < soap_level_low ? HIGH : LOW);
  digitalWrite(soap_indicator_Y, (distance1 > soap_level_low && distance1 < soap_level_high) ? HIGH : LOW);
  digitalWrite(soap_indicator_R, distance1 > soap_level_high ? HIGH : LOW);

  digitalWrite(pewangi_indicator_G, distance2 < pewangi_level_low ? HIGH : LOW);
  digitalWrite(pewangi_indicator_Y, (distance2 > pewangi_level_low && distance2 < pewangi_level_high) ? HIGH : LOW);
  digitalWrite(pewangi_indicator_R, distance2 > pewangi_level_high ? HIGH : LOW);

  delay(25);
}

// Handles soap selection button
void handleSoapSelection() {
  if (lock_rfid_reading != 1 || pump_active_soap || pump_active_pewangi || 
      button_paused_soap || sensor_paused_soap || button_paused_pewangi || sensor_paused_pewangi) {
    Serial.println("Soap selection locked: Pump active or paused");
    return;
  }

  if (lock_rfid_reading != 1) return;
  unsigned long currentMillis = millis();
  if (currentMillis - lastDebounceTime_S < debounceDelay) return;

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
    pump_on_duration_soap = (soap_menu == 0) ? 0 : (soap_menu == 1) ? timer_1 : (soap_menu == 2) ? timer_2 : timer_3;
    
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

// Handles pewangi selection button
void handlePewangiSelection() {
  if (lock_rfid_reading != 1 || pump_active_soap || pump_active_pewangi || 
      button_paused_soap || sensor_paused_soap || button_paused_pewangi || sensor_paused_pewangi) {
    Serial.println("Pewangi selection locked: Pump active or paused");
    return;
  }

  if (lock_rfid_reading != 1) return;
  unsigned long currentMillis = millis();
  if (currentMillis - lastDebounceTime_P < debounceDelay) return;

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
    pump_on_duration_pewangi = (pewangi_menu == 0) ? 0 : (pewangi_menu == 1) ? timer_4 : (pewangi_menu == 2) ? timer_5 : timer_6;
    
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

// Updates menu selection LEDs
void updateMenuIndicators() {
  digitalWrite(soap_select_button_indicator1, soap_menu == 1 ? HIGH : LOW);
  digitalWrite(soap_select_button_indicator2, soap_menu == 2 ? HIGH : LOW);
  digitalWrite(soap_select_button_indicator3, soap_menu == 3 ? HIGH : LOW);
  
  digitalWrite(pewangi_select_button_indicator1, pewangi_menu == 1 ? HIGH : LOW);
  digitalWrite(pewangi_select_button_indicator2, pewangi_menu == 2 ? HIGH : LOW);
  digitalWrite(pewangi_select_button_indicator3, pewangi_menu == 3 ? HIGH : LOW);
}

// Handles start button
void handleStartButton() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastDebounceTime_System < debounceDelay) return;

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
      // Check for pre-detected object to start new cycle
      if (object_detected_first && lock_rfid_reading == 1 && !pump_active_soap && !pump_active_pewangi) {
        if ((soap_menu >= 1 && soap_menu <= 3)) {
          remaining_duration_soap = pump_on_duration_soap;
          startSoapPump();
          lock_distance = 1; // Lock distance to prevent re-trigger
          object_detected_first = false; // Reset flag
          Serial.println("Soap Pump Started: Object Already Detected");
        } else if ((pewangi_menu >= 1 && pewangi_menu <= 3)) {
          remaining_duration_pewangi = pump_on_duration_pewangi;
          startPewangiPump();
          lock_distance = 1; // Lock distance to prevent re-trigger
          object_detected_first = false; // Reset flag
          Serial.println("Pewangi Pump Started: Object Already Detected");
        } else {
          Serial.println("Pump not started: Invalid menu selection");
          Serial.print("soap_menu: ");
          Serial.print(soap_menu);
          Serial.print(", pewangi_menu: ");
          Serial.println(pewangi_menu);
        }
      }
    } else if (button_paused_soap || sensor_paused_soap || button_paused_pewangi || sensor_paused_pewangi) {
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
        if ((digitalRead(e18_sensor) == LOW) && (remaining_duration_soap > 0 && allow_resume_soap) && !pump_active_soap && !pump_active_pewangi) {
          startSoapPump();
        }
      } else if (button_paused_pewangi || sensor_paused_pewangi) {
        char timeStr[6];
        int total_seconds = remaining_duration_pewangi / 1000;
        sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
        Serial.print("Pewangi Ready to Resume, Waiting for Object, Remaining: ");
        Serial.print(timeStr);
        Serial.println(" (MM:SS)");
        if ((digitalRead(e18_sensor) == LOW) && (remaining_duration_pewangi > 0 && allow_resume_pewangi) && !pump_active_soap && !pump_active_pewangi) {
          startPewangiPump();
        }
      }
    }
    lastDebounceTime_System = currentMillis;
  }
  lastState_Start = currentState_Start;
}

// Handles pause/stop button
void handlePauseButton() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastDebounceTime_System < debounceDelay) return;

  int currentState_Pos = digitalRead(buttonPin_Pos);
  if (currentState_Pos == LOW && lastState_Pos == HIGH && lock_Start == 1) {
    Serial.println("Pause Button Pressed");
    if (pump_active_soap || pump_active_pewangi) {
      Serial.println("Pause Pump");
      digitalWrite(led_Start, LOW);
      digitalWrite(led_Pos, HIGH);
      status = 3; // PAUSE
      lock_Start = 0;
      sound();
      if (pump_active_soap) {
        pauseSoapPump();
      } else if (pump_active_pewangi) {
        pausePewangiPump();
      }
    } else if (button_paused_soap || sensor_paused_soap || button_paused_pewangi || sensor_paused_pewangi) {
      Serial.println("Pump Already Paused");
      digitalWrite(led_Start, LOW);
      digitalWrite(led_Pos, HIGH);
      status = 3; // PAUSE
      lock_Start = 0;
      sound();
      printRemainingTimes();
    } else {
      resetSystem();
    }
    lastDebounceTime_System = currentMillis;
  }
  lastState_Pos = currentState_Pos;
}

// Starts soap pump
void startSoapPump() {
  digitalWrite(relay_pump_soap, HIGH);
  digitalWrite(relay_pump_pewangi, LOW);
  pump_start_time_soap = millis();
  pump_active_soap = true;
  button_paused_soap = false;
  sensor_paused_soap = false;
  allow_resume_soap = false;
  status = 1; // FILLING
  char timeStr[6];
  int total_seconds = remaining_duration_soap / 1000;
  sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
  Serial.print("Soap Pump Started: ");
  Serial.print(timeStr);
  Serial.println(" (MM:SS)");
}

// Starts pewangi pump
void startPewangiPump() {
  digitalWrite(relay_pump_soap, LOW);
  digitalWrite(relay_pump_pewangi, HIGH);
  pump_start_time_pewangi = millis();
  pump_active_pewangi = true;
  button_paused_pewangi = false;
  sensor_paused_pewangi = false;
  allow_resume_pewangi = false;
  status = 1; // FILLING
  char timeStr[6];
  int total_seconds = remaining_duration_pewangi / 1000;
  sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
  Serial.print("Pewangi Pump Started: ");
  Serial.print(timeStr);
  Serial.println(" (MM:SS)");
}

// Pauses soap pump
void pauseSoapPump() {
  unsigned long elapsed = millis() - pump_start_time_soap;
  remaining_duration_soap = (elapsed < remaining_duration_soap) ? (remaining_duration_soap - elapsed) : 0;
  digitalWrite(relay_pump_soap, LOW);
  pump_active_soap = false;
  button_paused_soap = true;
  sensor_paused_soap = false;
  allow_resume_soap = true;
  char timeStr[6];
  int total_seconds = remaining_duration_soap / 1000;
  sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
  Serial.print("Soap Pump Paused by Button, Remaining: ");
  Serial.print(timeStr);
  Serial.println(" (MM:SS)");
}

// Pauses pewangi pump
void pausePewangiPump() {
  unsigned long elapsed = millis() - pump_start_time_pewangi;
  remaining_duration_pewangi = (elapsed < remaining_duration_pewangi) ? (remaining_duration_pewangi - elapsed) : 0;
  digitalWrite(relay_pump_pewangi, LOW);
  pump_active_pewangi = false;
  button_paused_pewangi = true;
  sensor_paused_pewangi = false;
  allow_resume_pewangi = true;
  char timeStr[6];
  int total_seconds = remaining_duration_pewangi / 1000;
  sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
  Serial.print("Pewangi Pump Paused by Button, Remaining: ");
  Serial.print(timeStr);
  Serial.println(" (MM:SS)");
}

// Resets the system to initial state
void resetSystem() {
  Serial.println("Stop System");
  digitalWrite(led_Start, LOW);
  digitalWrite(led_Pos, HIGH);
  status = 0; // WAITING
  sound();
  lock_Start = 0;
  lock_rfid_reading = 0;
  lock_distance = 0;
  object_detected_first = false;
  digitalWrite(led_select_S, LOW);
  digitalWrite(led_select_P, LOW);
  digitalWrite(relay_pump_soap, LOW);
  digitalWrite(relay_pump_pewangi, LOW);
  button_paused_soap = false;
  button_paused_pewangi = false;
  sensor_paused_soap = false;
  sensor_paused_pewangi = false;
  allow_resume_soap = false;
  allow_resume_pewangi = false;
  remaining_duration_soap = 0;
  remaining_duration_pewangi = 0;
  soap_menu = 0;
  pewangi_menu = 0;
  pump_on_duration_soap = 0;
  pump_on_duration_pewangi = 0;
  pump_active_soap = false;
  pump_active_pewangi = false;
  pump_completed = false;
  set = 0;
  card_id = 0;
}

// Prints remaining times for paused pumps
void printRemainingTimes() {
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
}

// Reads RFID card via Serial1
void readRFID() {
  if (Serial1.available() <= 0) return;
  
  byte receivedValue = Serial1.read();
  Serial.print(F("Received value: "));
  Serial.println(receivedValue);
  
  if (receivedValue == 0) {
    sound();
    Serial.println(F("Card Detected! (UID: FC5E9832)"));
    lcd.clear();
    card_id = 1;
    lock_rfid_reading = 1;
    status = 0; // WAITING
  } else if (receivedValue == 1) {
    sound();
    Serial.println(F("Card Detected! (UID: B3A3CD4F)"));
    lcd.clear();
    card_id = 2;
    lock_rfid_reading = 1;
    status = 0; // WAITING
  } else {
    Serial.println(F("Unknown value received, ignoring."));
    card_id = 0;
    lock_rfid_reading = 0;
    status = 0; // WAITING
  }
  delay(100);
}

// Handles E18-D80NK sensor for object detection
void handleObjectDetection() {
  if (lock_rfid_reading != 1) {
    Serial.println("Object detection skipped: RFID not locked");
    return;
  }
  
  int state = digitalRead(e18_sensor);
  Serial.print("E18 Sensor State: ");
  Serial.println(state == LOW ? "LOW (Object Detected)" : "HIGH (No Object)");
  
  if (state == LOW && lock_distance == 0) {
    Serial.println("Object Detected");
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    lock_distance = 1;
    if (!lock_Start) {
      object_detected_first = true; // Set flag if object detected before start
      Serial.println("Object detected before Start button");
    }
    if (pump_completed) {
      pump_completed = false;
    }
    if (((soap_menu >= 1 && soap_menu <= 3) || (remaining_duration_soap > 0 && allow_resume_soap)) && !pump_active_soap && !pump_active_pewangi && lock_Start == 1) {
      digitalWrite(relay_pump_soap, HIGH);
      digitalWrite(relay_pump_pewangi, LOW);
      pump_start_time_soap = millis();
      pump_active_soap = true;
      button_paused_soap = false;
      sensor_paused_soap = false;
      allow_resume_soap = false;
      status = 1; // FILLING
      if (remaining_duration_soap == 0) {
        remaining_duration_soap = pump_on_duration_soap;
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
    } else if (((pewangi_menu >= 1 && pewangi_menu <= 3) || (remaining_duration_pewangi > 0 && allow_resume_pewangi)) && !pump_active_soap && !pump_active_pewangi && lock_Start == 1) {
      digitalWrite(relay_pump_soap, LOW);
      digitalWrite(relay_pump_pewangi, HIGH);
      pump_start_time_pewangi = millis();
      pump_active_pewangi = true;
      button_paused_pewangi = false;
      sensor_paused_pewangi = false;
      allow_resume_pewangi = false;
      status = 1; // FILLING
      if (remaining_duration_pewangi == 0) {
        remaining_duration_pewangi = pump_on_duration_pewangi;
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
    } else {
      Serial.println("Pump not started: Invalid menu or resume conditions not met");
      Serial.print("soap_menu: ");
      Serial.print(soap_menu);
      Serial.print(", remaining_duration_soap: ");
      Serial.print(remaining_duration_soap);
      Serial.print(", allow_resume_soap: ");
      Serial.println(allow_resume_soap);
      Serial.print("pewangi_menu: ");
      Serial.print(pewangi_menu);
      Serial.print(", remaining_duration_pewangi: ");
      Serial.print(remaining_duration_pewangi);
      Serial.print(", allow_resume_pewangi: ");
      Serial.println(allow_resume_pewangi);
      Serial.print("lock_Start: ");
      Serial.println(lock_Start);
    }
  } else if (state == HIGH && lock_distance == 1) {
    Serial.println("All Clear");
    digitalWrite(buzzerPin, LOW);
    lock_distance = 0;
    object_detected_first = false; // Reset flag when object is cleared
    if (pump_active_soap) {
      unsigned long elapsed = millis() - pump_start_time_soap;
      remaining_duration_soap = (elapsed < remaining_duration_soap) ? (remaining_duration_soap - elapsed) : 0;
      digitalWrite(relay_pump_soap, LOW);
      pump_active_soap = false;
      sensor_paused_soap = true;
      allow_resume_soap = true;
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
      allow_resume_pewangi = true;
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

// Activates buzzer for 100ms
void sound() {
  if (!buzzerActive) {
    digitalWrite(buzzerPin, HIGH);
    buzzerStartTime = millis();
    buzzerActive = true;
  }
}

// Updates LCD display
void updateDisplay() {
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
  } else {
    lcd.print("Time: 00:00       ");
  }

  lcd.setCursor(0, 3);
  if (status == 0) lcd.print("STATUS: WAITING  ");
  else if (status == 1) lcd.print("STATUS: FILLING  ");
  else if (status == 2) lcd.print("STATUS: FINISH   ");
  else if (status == 3) lcd.print("STATUS: PAUSE    ");
}

// Returns selected timer value
int timer() {
  if (set == 1 && soap_menu >= 1 && soap_menu <= 3) {
    return timer_set[soap_menu - 1];
  } else if (set == 2 && pewangi_menu >= 1 && pewangi_menu <= 3) {
    return timer_set[pewangi_menu + 2];
  }
  return 0;
}

// Checks and stops pumps when duration is reached
void checkPumpTimers() {
  unsigned long currentMillis = millis();
  if (pump_active_soap && currentMillis - pump_start_time_soap >= remaining_duration_soap) {
    digitalWrite(relay_pump_soap, LOW);
    digitalWrite(buzzerPin, LOW);
    set = 0;
    status = 2; // FINISH
    pump_active_soap = false;
    pump_completed = true;
    lock_Start = 0;
    lock_distance = 0;
    object_detected_first = false;
    button_paused_soap = false;
    sensor_paused_soap = false;
    allow_resume_soap = false;
    remaining_duration_soap = 0;
    soap_menu = 0;
    digitalWrite(led_select_S, LOW);
    digitalWrite(led_select_P, LOW);
    digitalWrite(led_Start, LOW);
    digitalWrite(led_Pos, HIGH);
    digitalWrite(soap_select_button_indicator1, LOW);
    digitalWrite(soap_select_button_indicator2, LOW);
    digitalWrite(soap_select_button_indicator3, LOW);
    Serial.println("Soap Pump: OFF (Duration Reached)");
  }
  if (pump_active_pewangi && currentMillis - pump_start_time_pewangi >= remaining_duration_pewangi) {
    digitalWrite(relay_pump_pewangi, LOW);
    digitalWrite(buzzerPin, LOW);
    set = 0;
    status = 2; // FINISH
    pump_active_pewangi = false;
    pump_completed = true;
    lock_Start = 0;
    lock_distance = 0;
    object_detected_first = false;
    button_paused_pewangi = false;
    sensor_paused_pewangi = false;
    allow_resume_pewangi = false;
    remaining_duration_pewangi = 0;
    pewangi_menu = 0;
    digitalWrite(led_select_S, LOW);
    digitalWrite(led_select_P, LOW);
    digitalWrite(led_Start, LOW);
    digitalWrite(led_Pos, HIGH);
    digitalWrite(pewangi_select_button_indicator1, LOW);
    digitalWrite(pewangi_select_button_indicator2, LOW);
    digitalWrite(pewangi_select_button_indicator3, LOW);
    Serial.println("Pewangi Pump: OFF (Duration Reached)");
  }
}

// Updates remaining time display
void updateRemainingTimeDisplay() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastDisplayTime < displayInterval) return;

  if (pump_active_soap || button_paused_soap || sensor_paused_soap) {
    unsigned long remaining_ms = (pump_active_soap) ? 
      (currentMillis - pump_start_time_soap < remaining_duration_soap ? 
       (remaining_duration_soap - (currentMillis - pump_start_time_soap)) : 0) : 
      remaining_duration_soap;
    int total_seconds = remaining_ms / 1000;
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
    Serial.print("Soap Remaining: ");
    Serial.print(timeStr);
    Serial.println(" (MM:SS)");
  }
  if (pump_active_pewangi || button_paused_pewangi || sensor_paused_pewangi) {
    unsigned long remaining_ms = (pump_active_pewangi) ? 
      (currentMillis - pump_start_time_pewangi < remaining_duration_pewangi ? 
       (remaining_duration_pewangi - (currentMillis - pump_start_time_pewangi)) : 0) : 
      remaining_duration_pewangi;
    int total_seconds = remaining_ms / 1000;
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", total_seconds / 60, total_seconds % 60);
    Serial.print("Pewangi Remaining: ");
    Serial.print(timeStr);
    Serial.println(" (MM:SS)");
  }
  lastDisplayTime = currentMillis;
}

// Manages buzzer timeout
void manageBuzzer() {
  if (buzzerActive && millis() - buzzerStartTime >= 100) {
    digitalWrite(buzzerPin, LOW);
    buzzerActive = false;
  }
}

// Initializes pins and system
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial);
  Serial.println(F("Arduino Mega ready to receive RFID value."));

  timer_set[0] = timer_1 / 1000;
  timer_set[1] = timer_2 / 1000;
  timer_set[2] = timer_3 / 1000;
  timer_set[3] = timer_4 / 1000;
  timer_set[4] = timer_5 / 1000;
  timer_set[5] = timer_6 / 1000;

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

// Main loop
void loop() {
  readRFID();
  handleSoapSelection();
  handlePewangiSelection();
  handleStartButton();
  handlePauseButton();
  handleObjectDetection();
  updateMenuIndicators();
  updateTankLevels();
  checkPumpTimers();
  updateRemainingTimeDisplay();
  manageBuzzer();
  updateDisplay();
  delay(5);
}