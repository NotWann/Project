#define SENSOR1_PIN A12 // Analog pin for first sensor Vo
#define SENSOR2_PIN A13 // Analog pin for second sensor Vo
#define SAMPLES 10      // Number of samples for averaging

// Lookup table: {voltage, distance in cm} for low-reflectivity objects
/*
  ### This is original lookup table for sharp analog sensor for detect distance @ tank level (dah calibrate) ###
*/
float lookupTable[][2] = {
  {0.19, 80}, {0.39, 60}, {0.52, 40}, {0.77, 30}, {1.23, 20}, {2.3, 10}
};
/*###############################################################################################*/

/* ### EDIT SECTION ### 
    Nak edit timer untuk pump boleh edit kat sini*/
/*Soap pump*/
const int timer_1 = 5000;  // 5 saat
const int timer_2 = 10000; // 10 saat
const int timer_3 = 15000; // 15 saat

/*Pewangi pump*/
const int timer_4 = 5000;  // 5 saat
const int timer_5 = 10000; // 10 saat
const int timer_6 = 15000; // 15 saat

//Indicator Tanki Soap
const int soap_level_low = 15;   //Paling rendah
const int soap_level_high = 30;  //Paling tinggi

//Indicator Tanki pewangi
const int pewangi_level_low = 15;  //Paling rendah
const int pewangi_level_high = 30;  //Paling tinggi

/*###############################################################################################*/

//pinout push button
const int buttonPin_select_S = 30;
const int buttonPin_select_P = 31;
const int buttonPin_Start = 32;
const int buttonPin_Pos = 33;

//pinout led push button
const int led_select_S = 34;
const int led_select_P = 35;
const int led_Start = 36;
const int led_Pos = 37;

//pinout led traffic light for soap indicator tank
const int soap_indicator_G = 5;
const int soap_indicator_Y = 6;
const int soap_indicator_R = 7;

//pinout led traffic light for pewangi indicator tank
const int pewangi_indicator_G = 9;
const int pewangi_indicator_Y = 10;
const int pewangi_indicator_R = 11;

//pinout for led display timer indicator (soap pump)
const int soap_select_button_indicator1 = 38;
const int soap_select_button_indicator2 = 39;
const int soap_select_button_indicator3 = 40;

//pinout for led display timer indicator (pewangi pump)
const int pewangi_select_button_indicator1 = 42;
const int pewangi_select_button_indicator2 = 43;
const int pewangi_select_button_indicator3 = 44;

//buzzer pinout
const int buzzerPin = 13;

//pinout sensor e18 ir distance sensor
const int e18_sensor = 26;

//pinout relay for pump
const int relay_pump_soap = 22;
const int relay_pump_pewangi = 23;

int soap_menu = 0;
int pewangi_menu = 0;
int lock_Start = 0;
int lock_rfid_reading = 0;
int lock_distance = 0;
bool pump_completed = false; // Flag to track pump cycle completion

unsigned long lastDebounceTime_S = 0; // Debounce timer for soap button
unsigned long lastDebounceTime_P = 0; // Debounce timer for pewangi button
unsigned long lastDebounceTime_System = 0; // Debounce timer for system buttons
const unsigned long debounceDelay = 100;

unsigned long pump_on_duration_soap = 0;
unsigned long pump_on_duration_pewangi = 0;
unsigned long pump_start_time_soap = 0;
unsigned long pump_start_time_pewangi = 0;
bool pump_active_soap = false;
bool pump_active_pewangi = false;

int lastState_S = HIGH; // Previous state of soap button
int lastState_P = HIGH; // Previous state of pewangi button
int lastState_Start = HIGH; // Previous state of start button
int lastState_Pos = HIGH; // Previous state of pos button

unsigned long buzzerStartTime = 0;
bool buzzerActive = false;

// *** New variables for pause/resume functionality ***
bool pump_paused_soap = false;
bool pump_paused_pewangi = false;
unsigned long remaining_duration_soap = 0;
unsigned long remaining_duration_pewangi = 0;

// Distance measurement for sharp analog sensor based on lookup table
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

// Function sharp analog sensor
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

  // Soap indicator
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

    // Pewangi indicator
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

  Serial.println("=== Sensor Readings ===");
  Serial.print("Soap: ");
  Serial.print(distance1);
  Serial.println(" cm");
  Serial.print("Pewangi: ");
  Serial.print(distance2);
  Serial.println(" cm");
  Serial.println();
  delay(25);
}

//Function push button select
void select_button() {
  unsigned long currentMillis = millis();
  if (lock_rfid_reading == 1) {
    // Soap button handling
    if (currentMillis - lastDebounceTime_S >= debounceDelay) {
      int currentState_S = digitalRead(buttonPin_select_S);
      if (lastState_S == LOW && currentState_S == HIGH) {
        soap_menu = (soap_menu + 1) % 4; // Cycle through 0, 1, 2, 3
        pewangi_menu = 0; // Reset pewangi menu
        pump_on_duration_pewangi = 0; // Reset pewangi duration
        pump_active_pewangi = false; // Reset pewangi pump state
        if (soap_menu == 0) pump_on_duration_soap = 0;
        else if (soap_menu == 1) pump_on_duration_soap = timer_1;
        else if (soap_menu == 2) pump_on_duration_soap = timer_2;
        else if (soap_menu == 3) pump_on_duration_soap = timer_3;
        Serial.print("Soap Menu: ");
        Serial.println(soap_menu);
        Serial.print("Soap Pump Duration: ");
        Serial.print(pump_on_duration_soap);
        Serial.println(" ms");
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

    // Pewangi button handling
    if (currentMillis - lastDebounceTime_P >= debounceDelay) {
      int currentState_P = digitalRead(buttonPin_select_P);
      if (lastState_P == LOW && currentState_P == HIGH) {
        pewangi_menu = (pewangi_menu + 1) % 4; // Cycle through 0, 1, 2, 3
        soap_menu = 0; // Reset soap menu
        pump_on_duration_soap = 0; // Reset soap duration
        pump_active_soap = false; // Reset soap pump state
        if (pewangi_menu == 0) pump_on_duration_pewangi = 0;
        else if (pewangi_menu == 1) pump_on_duration_pewangi = timer_4;
        else if (pewangi_menu == 2) pump_on_duration_pewangi = timer_5;
        else if (pewangi_menu == 3) pump_on_duration_pewangi = timer_6;
        Serial.print("Pewangi Menu: ");
        Serial.println(pewangi_menu);
        Serial.print("Pewangi Pump Duration: ");
        Serial.print(pump_on_duration_pewangi);
        Serial.println(" ms");
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

//Function led timer pump indicator
void update_menu_indicators() {
  // Soap indicators
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
    } else if (soap_menu == 0) {
      digitalWrite(soap_select_button_indicator1, LOW);
      digitalWrite(soap_select_button_indicator2, LOW);
      digitalWrite(soap_select_button_indicator3, LOW);
    }

    // Pewangi indicators
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
    } else if (pewangi_menu == 0) {
      digitalWrite(pewangi_select_button_indicator1, LOW);
      digitalWrite(pewangi_select_button_indicator2, LOW);
      digitalWrite(pewangi_select_button_indicator3, LOW);
    }
}

//Function push button for system ON and PAUSE
void system_button() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastDebounceTime_System >= debounceDelay) {
    int currentState_Start = digitalRead(buttonPin_Start);
    if (currentState_Start == LOW && lastState_Start == HIGH) {
      if (lock_Start == 0) {
        // Start system
        Serial.println("Start System");
        digitalWrite(led_Start, HIGH);
        digitalWrite(led_Pos, LOW);
        sound();
        lock_Start = 1;
      } else if ((pump_paused_soap || pump_paused_pewangi) && lock_Start == 1) {
        // Resume paused pump
        Serial.println("Resume Pump");
        digitalWrite(led_Start, HIGH);
        digitalWrite(led_Pos, LOW);
        sound();
        if (pump_paused_soap && remaining_duration_soap > 0) {
          pump_start_time_soap = millis();
          pump_active_soap = true;
          pump_paused_soap = false;
          Serial.print("Resuming Soap Pump for: ");
          Serial.print(remaining_duration_soap);
          Serial.println(" ms");
        } else if (pump_paused_pewangi && remaining_duration_pewangi > 0) {
          pump_start_time_pewangi = millis();
          pump_active_pewangi = true;
          pump_paused_pewangi = false;
          Serial.print("Resuming Pewangi Pump for: ");
          Serial.print(remaining_duration_pewangi);
          Serial.println(" ms");
        }
      }
      lastDebounceTime_System = currentMillis;
    }
    lastState_Start = currentState_Start;

    int currentState_Pos = digitalRead(buttonPin_Pos);
    if (currentState_Pos == LOW && lastState_Pos == HIGH && lock_Start == 1) {
      if (pump_active_soap || pump_active_pewangi) {
        // Pause active pump
        Serial.println("Pause Pump");
        digitalWrite(led_Start, LOW);
        digitalWrite(led_Pos, HIGH);
        sound();
        if (pump_active_soap) {
          unsigned long elapsed = millis() - pump_start_time_soap;
          remaining_duration_soap = (elapsed < pump_on_duration_soap) ? (pump_on_duration_soap - elapsed) : 0;
          digitalWrite(relay_pump_soap, LOW);
          pump_active_soap = false;
          pump_paused_soap = true;
          Serial.print("Soap Pump Paused, Remaining: ");
          Serial.print(remaining_duration_soap);
          Serial.println(" ms");
        } else if (pump_active_pewangi) {
          unsigned long elapsed = millis() - pump_start_time_pewangi;
          remaining_duration_pewangi = (elapsed < pump_on_duration_pewangi) ? (pump_on_duration_pewangi - elapsed) : 0;
          digitalWrite(relay_pump_pewangi, LOW);
          pump_active_pewangi = false;
          pump_paused_pewangi = true;
          Serial.print("Pewangi Pump Paused, Remaining: ");
          Serial.print(remaining_duration_pewangi);
          Serial.println(" ms");
        }
      } else {
        // Stop system if no pump is active
        Serial.println("Stop System");
        digitalWrite(led_Start, LOW);
        digitalWrite(led_Pos, HIGH);
        sound();
        lock_Start = 0;
        digitalWrite(led_select_S, LOW);
        digitalWrite(led_select_P, LOW);
        digitalWrite(relay_pump_soap, LOW);
        digitalWrite(relay_pump_pewangi, LOW);
        pump_paused_soap = false;
        pump_paused_pewangi = false;
        remaining_duration_soap = 0;
        remaining_duration_pewangi = 0;
      }
      lastDebounceTime_System = currentMillis;
    }
    lastState_Pos = currentState_Pos;
  }
}

//Function read rfid for Nano via uart
void read_rfid_value() {
  if (Serial1.available() > 0) {
    byte receivedValue = Serial1.read();
    Serial.print(F("Received value: "));
    Serial.println(receivedValue);
    if (receivedValue == 0) {
      sound();
      Serial.println(F("Card Detected! (UID: FC5E9832)"));
      lock_rfid_reading = 1;
    } else if (receivedValue == 1) {
      sound();
      Serial.println(F("Card Detected! (UID: B3A3CD4F)"));
      lock_rfid_reading = 1;
    } else {
      Serial.println(F("Unknown value received, ignoring."));
      lock_rfid_reading = 0;
    }
    delay(100);
  }
}

//Function detect object sensor e18 ir sensor
void read_distance_e18() {
  if (lock_rfid_reading == 1)
  {
    if (lock_Start == 1) 
    {
      int state = digitalRead(e18_sensor);
      if (state == LOW && lock_distance == 0) 
      {
        Serial.println("Object Detected");
        digitalWrite(buzzerPin, HIGH);
        lock_distance = 1;
        if (pump_completed) 
        {
          pump_completed = false; // Allow new pump cycle after clear
        }
      } 
      else if (state == HIGH && lock_distance == 1) 
      {
        Serial.println("All Clear");
        digitalWrite(buzzerPin, LOW);
        lock_distance = 0;
        digitalWrite(relay_pump_soap, LOW);
        digitalWrite(relay_pump_pewangi, LOW);
        pump_active_soap = false;
        pump_active_pewangi = false;
        pump_completed = false; // Reset to allow new cycle
      }
      // Stop pumps if object clears during operation
      if (state == HIGH && (pump_active_soap || pump_active_pewangi)) 
      {
        digitalWrite(relay_pump_soap, LOW);
        digitalWrite(relay_pump_pewangi, LOW);
        pump_active_soap = false;
        pump_active_pewangi = false;
        pump_completed = false;
        Serial.println("Pump Stopped: Object Cleared");
      }
      delay(10);
    }
  }
}

//Buzzer sound function (millis)
void sound() {
  if (!buzzerActive) {
    digitalWrite(buzzerPin, HIGH);
    buzzerStartTime = millis();
    buzzerActive = true;
  }
}

///
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial);
  Serial.println(F("Arduino Mega ready to receive RFID value."));

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

///
void loop() {
  read_rfid_value();
  select_button();
  system_button();
  read_distance_e18();
  update_menu_indicators();
  sharp_sensor();

  // Pump control
  if (lock_Start == 1 && lock_distance == 1 && !pump_completed && !pump_paused_soap && !pump_paused_pewangi) {
    if (soap_menu >= 1 && soap_menu <= 3 && !pump_active_soap && pump_on_duration_soap > 0) {
      digitalWrite(relay_pump_soap, HIGH);
      digitalWrite(relay_pump_pewangi, LOW);
      pump_start_time_soap = millis();
      pump_active_soap = true;
      remaining_duration_soap = pump_on_duration_soap; // Initialize remaining duration
      Serial.println("Soap Pump: ON");
    } else if (pewangi_menu >= 1 && pewangi_menu <= 3 && !pump_active_pewangi && pump_on_duration_pewangi > 0) {
      digitalWrite(relay_pump_soap, LOW);
      digitalWrite(relay_pump_pewangi, HIGH);
      pump_start_time_pewangi = millis();
      pump_active_pewangi = true;
      remaining_duration_pewangi = pump_on_duration_pewangi; // Initialize remaining duration
      Serial.println("Pewangi Pump: ON");
    }
  }

  // Turn off pumps after duration
  if (pump_active_soap && millis() - pump_start_time_soap >= remaining_duration_soap) {
    digitalWrite(relay_pump_soap, LOW);
    digitalWrite(buzzerPin, LOW);
    pump_active_soap = false;
    pump_completed = true; // Mark cycle as completed
    soap_menu = 0;
    lock_rfid_reading = 0;
    remaining_duration_soap = 0;
    digitalWrite(led_select_S, LOW);
    digitalWrite(led_select_P, LOW);
    digitalWrite(soap_select_button_indicator1, LOW);
    digitalWrite(soap_select_button_indicator2, LOW);
    digitalWrite(soap_select_button_indicator3, LOW);
    Serial.println("Soap Pump: OFF (Duration Reached)");
  }
  if (pump_active_pewangi && millis() - pump_start_time_pewangi >= remaining_duration_pewangi) {
    digitalWrite(relay_pump_pewangi, LOW);
    digitalWrite(buzzerPin, LOW);
    pump_active_pewangi = false;
    pump_completed = true; // Mark cycle as completed
    pewangi_menu = 0;
    lock_rfid_reading = 0;
    remaining_duration_pewangi = 0;
    digitalWrite(led_select_S, LOW);
    digitalWrite(led_select_P, LOW);
    digitalWrite(pewangi_select_button_indicator1, LOW);
    digitalWrite(pewangi_select_button_indicator2, LOW);
    digitalWrite(pewangi_select_button_indicator3, LOW);
    Serial.println("Pewangi Pump: OFF (Duration Reached)");
  }

  // Non-blocking buzzer
  if (buzzerActive && millis() - buzzerStartTime >= 150) {
    digitalWrite(buzzerPin, LOW);
    buzzerActive = false;
  }

  delay(5);
}


