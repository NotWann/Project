#include <Wire.h>
#include <EEPROM.h>

#include <HX711_ADC.h>
#define HX711_DOUT 4
#define HX711_SCK 5
HX711_ADC LoadCell(HX711_DOUT, HX711_SCK);

#include <Servo.h>
Servo servo;

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

/*#######################################################*/
/* EEPROM store address */
/*#######################################################*/
int store_weight_addr = 0;
int store_timer_addr[3] = { 4, 5, 6 };
int store_servo_timer_addr[3] = { 7, 8, 9 };

/*#######################################################*/
/* Pinout */
/*#######################################################*/
const int set_button_pin = 8;         // set push button pin
const int mode_button_pin = 7;        // mode push button pin
const int emergency_button_pin = 10;  //emergency button pin
const int buzzer_pin = 13;            // buzzer pin
const int servo_pin = 3;              // servo pin
const int pot_pin = A0;               //potentiometer pin

/*#######################################################*/
/* Variable for smooth potentiometer analog read value */
/*#######################################################*/
int map_pot_value = 0;
float smoothedPot = 0;
float alpha = 0.1;  // Smoothing factor

/*#######################################################*/
/* Variable for set countdown timer */
/*#######################################################*/
int timer = 0;      // Countdown timer
int set_timer = 0;  // Servo ON time (in seconds)
int hours = 0;
int minutes = 0;
int seconds = 0;
unsigned long lastTick = 0;

/*#######################################################*/
/* Variable for set servo rotate (ON) timer */
/*#######################################################*/
int servo_rotated_hours = 0;
int servo_rotated_minutes = 0;
int servo_rotated_seconds = 0;

/*#######################################################*/
/* Variable for servo action */
/*#######################################################*/
const unsigned long off_duration = 5000;  // Fixed off duration in ms (reset time)
bool on_active = false;                   // Flag for on period active
bool off_active = false;                  // Flag for off period active
int current_on_seconds = 0;               // Current countdown for on period
int current_off_seconds = 0;              // Current countdown for off period
unsigned long last_on_tick = 0;           // Last tick for on timer
unsigned long last_off_tick = 0;          // Last tick for off timer

/*#######################################################*/
/* Variable for push button action */
/*#######################################################*/
int last_set_state = HIGH;   // Single press transition set button
int last_mode_state = HIGH;  // Single press transition mode button

int flag_set = 0;   // Menu cycle for set button
int flag_mode = 0;  // Menu cycle for mode button

/*#######################################################*/
/* Variable for emergency button action */
/*#######################################################*/
int lock_system = 0;

/*#######################################################*/
/* Variable for weight detection */
/*#######################################################*/
unsigned long t = 0;   // Time to read weight between reading
float weight = 0;      // Reading weight in kg
float set_weight = 0;  // Set weight in kg


/*#######################################################*/
/* Function for reading smooth potentiometer value */
/*#######################################################*/
int smooth_pot_value() {
  int raw = analogRead(pot_pin);
  smoothedPot = alpha * raw + (1 - alpha) * smoothedPot;
  return (int)smoothedPot;
}


/*#######################################################*/
/* Function for set value,
   @ set_weight             : targeted weight (kg)
   @ hours                  : total timer for hours
   @ minutes                : total timer for minutes
   @ seconds                : total timer for seconds
   @ servo_rotated_hours    : total servo (on) timer for hours
   @ servo_rotated_minutes  : total servo (on) timer for minutes
   @ servo_rotated_seconds  : total servo (on) timer for seconds*/
/*#######################################################*/
void read_potvalue() {
  int pot_value = smooth_pot_value();
  // Serial.print("Raw Pot: "); Serial.println(pot_value);

  // Set Targeted Weight:
  if (flag_mode == 1) {
    if (flag_set == 1) {
      /******************************************************************/
      set_weight = constrain(map(pot_value, 0, 1000, 0, 10000) / 1000.0, 0.00, 10.00);  // kg (float)
      save_eeprom(1);
      Serial.print("Set Weight: ");
      Serial.print(set_weight);
      Serial.println(" kg");
      /******************************************************************/
    }

    // Set Total Timer:
    else if (flag_set == 2) {
      /******************************************************************/
      hours = constrain(map(pot_value, 0, 1000, 0, 24), 0, 24);  // 0-23 hours
      timer = hours * 3600 + minutes * 60 + seconds;             // Update timer
      save_eeprom(2);
      Serial.print("Set Hours: ");
      Serial.println(hours);
      /******************************************************************/
    } else if (flag_set == 3) {
      /******************************************************************/
      minutes = constrain(map(pot_value, 0, 1000, 0, 59), 0, 59);  // 0-59 minutes
      timer = hours * 3600 + minutes * 60 + seconds;               // Update timer
      save_eeprom(3);
      Serial.print("Set Minutes: ");
      Serial.println(minutes);
      /******************************************************************/
    } else if (flag_set == 4) {
      /******************************************************************/
      seconds = constrain(map(pot_value, 0, 1000, 0, 59), 0, 59);  // 0-59 minutes
      timer = hours * 3600 + minutes * 60 + seconds;               // Update timer
      save_eeprom(4);
      Serial.print("Set Seconds: ");
      Serial.println(seconds);
      /******************************************************************/
    }

    // Set Servo ON:
    else if (flag_set == 5) {
      /******************************************************************/
      servo_rotated_hours = constrain(map(pot_value, 0, 1000, 0, 24), 0, 24);                       // 0-23 hours
      set_timer = servo_rotated_hours * 3600 + servo_rotated_minutes * 60 + servo_rotated_seconds;  // Update set_timer
      save_eeprom(5);
      Serial.print("Set Servo Hours: ");
      Serial.println(servo_rotated_hours);
      /******************************************************************/
    } else if (flag_set == 6) {
      /******************************************************************/
      servo_rotated_minutes = constrain(map(pot_value, 0, 1000, 0, 59), 0, 59);                     // 0-59 minutes
      set_timer = servo_rotated_hours * 3600 + servo_rotated_minutes * 60 + servo_rotated_seconds;  // Update set_timer
      save_eeprom(6);
      Serial.print("Set Servo Minutes: ");
      Serial.println(servo_rotated_minutes);
      /******************************************************************/
    } else if (flag_set == 7) {
      /******************************************************************/
      servo_rotated_seconds = constrain(map(pot_value, 0, 1000, 0, 59), 0, 59);                     // 0-59 minutes
      set_timer = servo_rotated_hours * 3600 + servo_rotated_minutes * 60 + servo_rotated_seconds;  // Update set_timer
      save_eeprom(7);
      Serial.print("Set Servo Seconds: ");
      Serial.println(servo_rotated_seconds);
      /******************************************************************/
    }
  }
}


/*#######################################################*/
/* Function for emergency button
   @ lock_system = 1 : emergency button is pressed
   @ lock_system = 0 : emergency button not pressed*/
/*#######################################################*/
void emergency() {
  int current_emergency_state = digitalRead(emergency_button_pin);

  if (current_emergency_state == LOW && lock_system == 0) {
    lock_system = 1;  //

    lcd.clear();
    servo.write(0);

    flag_mode = 0;   // Reset mode menu cycle
    flag_set = 0;    // Reset set menu cycle
    on_active = false;
    off_active = false;
  } else if (current_emergency_state == HIGH && lock_system == 1) {
    lock_system = 0;  // Reset emergency lock
  }
}

/*#######################################################*/
/* Function for push button set mode */
/*#######################################################*/
void set_mode() {
  int current_set_state = digitalRead(set_button_pin);
  /******************************************************************/
  if (current_set_state == LOW && last_set_state == HIGH && flag_mode == 1) {
    flag_set = (flag_set + 1) % 8;  // Set mode menu cycle
    digitalWrite(buzzer_pin, HIGH);
    delay(100);
    digitalWrite(buzzer_pin, LOW);
    //lcd.clear();
    delay(50);  // Debounce
  }
  /******************************************************************/
  last_set_state = current_set_state;  // Update state
}


/*#######################################################*/
/* Function for push button select mode */
/*#######################################################*/
void select_mode() {
  int current_mode_state = digitalRead(mode_button_pin);
  static unsigned long pressStartTime = 0;       // Track when button is pressed
  const unsigned long longPressDuration = 1000;  // 1 second for long press

  /******************************************************************/
  // Detect button press (HIGH to LOW transition)
  /******************************************************************/
  if (current_mode_state == LOW && last_mode_state == HIGH) {
    pressStartTime = millis();  // Record press start time
  }
  /******************************************************************/

  /******************************************************************/
  // Detect button hold (still LOW after transition)
  /******************************************************************/
  if (current_mode_state == LOW && last_mode_state == LOW) {
    if (millis() - pressStartTime >= longPressDuration) {
      // lcd.clear();
      flag_mode = 3;                  // Set to mode 3 for long press
      flag_set = 0;                   // Reset set flag
      lastTick = millis();            // Initialize lastTick
      digitalWrite(buzzer_pin, HIGH);
      delay(100);
      digitalWrite(buzzer_pin, LOW);
      Serial.println("Start TIME");
      delay(50);  // Debounce

      /******************************************************************/
      // Wait for button release to avoid repeated triggers
      /******************************************************************/
      while (digitalRead(mode_button_pin) == LOW) {}
      delay(50);  // Debounce release
    }
  }
  /******************************************************************/

  /******************************************************************/
  // Detect button release (LOW to HIGH transition) for short press
  /******************************************************************/
  if (current_mode_state == HIGH && last_mode_state == LOW) {
    // Check if it was a short press (less than 1 second)
    if (millis() - pressStartTime < longPressDuration) {
      lcd.clear();
      flag_mode = (flag_mode + 1) % 2;  // Toggle between 0 and 1
      flag_set = 0;                     // Reset set flag
      digitalWrite(buzzer_pin, HIGH);
      delay(100);
      digitalWrite(buzzer_pin, LOW);
      Serial.println(flag_mode == 0 ? "Mode: Main Display" : "Mode: Set Mode");
      delay(50);  // Debounce
    }
  }
  /******************************************************************/

  last_mode_state = current_mode_state;
}


/*#######################################################*/
/* Function for timer countdown */
/*#######################################################*/
void timer_countdown() {
  unsigned long currentTime = millis();
  static unsigned long lastBuzzerOnTime = 0;      // Track buzzer on time for non-blocking blink
  static bool buzzerIsOn = false;                 // Track Buzzer state
  const unsigned long buzzerBlinkDuration = 100;  // Buzzer blink duration (ms)

  /******************************************************************/
  // Handle non-blocking buzzer blinking
  /******************************************************************/
  if (buzzerIsOn && currentTime - lastBuzzerOnTime >= buzzerBlinkDuration) {
    digitalWrite(buzzer_pin, LOW);
    buzzerIsOn = false;
  }

  /******************************************************************/
  // Timer countdown
  /******************************************************************/
  if (currentTime - lastTick >= 1000 && flag_mode == 3) {
    lastTick = currentTime;

    if (seconds > 0 || minutes > 0 || hours > 0) {
      if (seconds > 0) {
        seconds--;
      } else {
        if (minutes > 0) {
          minutes--;
          seconds = 59;
        } else {
          hours--;
          minutes = 59;
          seconds = 59;
        }
      }

      timer = hours * 3600 + minutes * 60 + seconds;  // Sync timer

      Serial.print("Time left: ");
      Serial.print(hours);
      Serial.print("h ");
      Serial.print(minutes);
      Serial.print("m ");
      Serial.print(seconds);
      Serial.println("s");

    } else {
      Serial.println("Time's up!");
      servo.write(0);  // Ensure servo resets
      digitalWrite(buzzer_pin, HIGH);
      buzzerIsOn = true;
      lastBuzzerOnTime = currentTime;
      Serial.println("Servo Reset! (timer up)");
      on_active = false;
      off_active = false;
      flag_mode = 0;   // Return to main mode
      read_eeprom();   // Recall original timer values
    }
  }

  /******************************************************************/
  // Servo logic: separate timers for on and off periods
  /******************************************************************/
  if (flag_mode == 3) {
    if (!on_active && !off_active) {
      // Check if enough time left before starting ON
      if (timer >= set_timer) {
        // Start ON period
        Serial.println("Servo Rotated!");
        digitalWrite(buzzer_pin, HIGH);
        buzzerIsOn = true;
        lastBuzzerOnTime = currentTime;
        servo.write(100);
        on_active = true;
        current_on_seconds = set_timer;
        last_on_tick = currentTime;
      } else {
        // Not enough time, reset servo and skip
        servo.write(0);
        Serial.println("Not enough time for servo ON, skipping.");
      }
    }

    /******************************************************************/
    // Tick ON timer
    /******************************************************************/
    if (on_active && currentTime - last_on_tick >= 1000) {
      last_on_tick = currentTime;
      current_on_seconds--;
      if (current_on_seconds <= 0) {
        digitalWrite(buzzer_pin, HIGH);
        buzzerIsOn = true;
        lastBuzzerOnTime = currentTime;
        servo.write(0);
        on_active = false;
        off_active = true;
        current_off_seconds = off_duration / 1000;  // Convert ms to seconds
        last_off_tick = currentTime;
        Serial.println("Start off period");
      }
    }

    /******************************************************************/
    // Tick OFF timer
    /******************************************************************/
    if (off_active && currentTime - last_off_tick >= 1000) {
      last_off_tick = currentTime;
      current_off_seconds--;
      if (current_off_seconds <= 0) {
        off_active = false;
        digitalWrite(buzzer_pin, HIGH);
        buzzerIsOn = true;
        lastBuzzerOnTime = currentTime;
        Serial.println("Servo Reset!");
      }
    }
  }
}


/*#######################################################*/
/* Helper function to format seconds into hours, minutes, seconds */
/*#######################################################*/
void format_time(int total_seconds, int &h, int &m, int &s) {
  h = total_seconds / 3600;
  m = (total_seconds % 3600) / 60;
  s = total_seconds % 60;
}


/*#######################################################*/
/* Function for lcd display */
/*#######################################################*/
void display() {

  if (flag_mode == 0 || flag_mode == 3) {
    /******************************************************************/
    lcd.setCursor(0, 0);
    lcd.print("Weight: ");
    lcd.print(weight, 2);
    lcd.print("kg  ");
    /******************************************************************/

    /******************************************************************/
    lcd.setCursor(0, 1);
    lcd.print("Set Weight: ");
    lcd.print(set_weight, 2);
    lcd.print("kg  ");
    /******************************************************************/

    /******************************************************************/
    lcd.setCursor(0, 2);
    lcd.print("Timer: ");
    int t_h, t_m, t_s;
    format_time(timer, t_h, t_m, t_s);
    lcd.print(t_h < 10 ? "0" : "");
    lcd.print(t_h);
    lcd.print(":");
    lcd.print(t_m < 10 ? "0" : "");
    lcd.print(t_m);
    lcd.print(":");
    lcd.print(t_s < 10 ? "0" : "");
    lcd.print(t_s);
    lcd.print("  ");
    /******************************************************************/

    /******************************************************************/
    lcd.setCursor(0, 3);
    lcd.print("Servo: ");
    int st_h, st_m, st_s;
    format_time(set_timer, st_h, st_m, st_s);
    lcd.print(st_h < 10 ? "0" : "");
    lcd.print(st_h);
    lcd.print(":");
    lcd.print(st_m < 10 ? "0" : "");
    lcd.print(st_m);
    lcd.print(":");
    lcd.print(st_s < 10 ? "0" : "");
    lcd.print(st_s);
    lcd.print("  ");
    /******************************************************************/
  } 
  else if (flag_mode == 1) {
    /******************************************************************/
    lcd.setCursor(0, 0);
    lcd.print(flag_set == 1 ? ">Set Weight: " : "Set Weight: ");
    lcd.print(set_weight, 2);
    lcd.print("kg  ");
    /******************************************************************/

    /******************************************************************/
    lcd.setCursor(0, 1);
    lcd.print(flag_set == 2 || flag_set == 3 || flag_set == 4 ? ">Set Time: " : "Set Time: ");
    lcd.print(hours < 10 ? "0" : "");
    lcd.print(hours);
    lcd.print(":");
    lcd.print(minutes < 10 ? "0" : "");
    lcd.print(minutes);
    lcd.print(":");
    lcd.print(seconds < 10 ? "0" : "");
    lcd.print(seconds);
    lcd.print("  ");
    /******************************************************************/

    /******************************************************************/
    lcd.setCursor(0, 2);
    lcd.print(flag_set == 5 || flag_set == 6 || flag_set == 7 ? ">Set Servo: " : "Set Servo: ");
    lcd.print(servo_rotated_hours < 10 ? "0" : "");
    lcd.print(servo_rotated_hours);
    lcd.print(":");
    lcd.print(servo_rotated_minutes < 10 ? "0" : "");
    lcd.print(servo_rotated_minutes);
    lcd.print(":");
    lcd.print(servo_rotated_seconds < 10 ? "0" : "");
    lcd.print(servo_rotated_seconds);
    lcd.print("  ");
    /******************************************************************/
  }
}


/*#######################################################*/
/* Function to read value in EEPROM */
/*#######################################################*/
void read_eeprom() {
  set_weight = EEPROM.get(store_weight_addr, set_weight);          // Set weight
  hours = EEPROM.read(store_timer_addr[0]);                        // Timer countdown hours
  minutes = EEPROM.read(store_timer_addr[1]);                      // Timer countdown minutes
  seconds = EEPROM.read(store_timer_addr[2]);                      // Timer countdown seconds
  servo_rotated_hours = EEPROM.read(store_servo_timer_addr[0]);    // Servo on hours
  servo_rotated_minutes = EEPROM.read(store_servo_timer_addr[1]);  // Servo on minutes
  servo_rotated_seconds = EEPROM.read(store_servo_timer_addr[2]);  // Servo on seconds

  /******************************************************************/
  // Recalculate timer and set_timer from their components
  /******************************************************************/
  timer = hours * 3600 + minutes * 60 + seconds;
  set_timer = servo_rotated_hours * 3600 + servo_rotated_minutes * 60 + servo_rotated_seconds;
}


/*#######################################################*/
/* Function to save value in EEPROM */
/*#######################################################*/
void save_eeprom(int num) {
  if (num == 1) EEPROM.put(store_weight_addr, set_weight);                             // Save set weight
  else if (num == 2) EEPROM.update(store_timer_addr[0], hours);                        // Save set timer hours
  else if (num == 3) EEPROM.update(store_timer_addr[1], minutes);                      // Save set timer minutes
  else if (num == 4) EEPROM.update(store_timer_addr[2], seconds);                      // Save set timer seconds
  else if (num == 5) EEPROM.update(store_servo_timer_addr[0], servo_rotated_hours);    // Save servo on hours
  else if (num == 6) EEPROM.update(store_servo_timer_addr[1], servo_rotated_minutes);  // Save servo on minutes
  else if (num == 7) EEPROM.update(store_servo_timer_addr[2], servo_rotated_seconds);  // SAve servo on seconds
}


/*#######################################################*/
/* Function to debug EEPROM */
/*#######################################################*/
void debug_eeprom() {
  Serial.println("EEPROM Contents:");
  Serial.print("Weight: ");
  Serial.println(EEPROM.read(store_weight_addr));
  Serial.print("Timer Hours: ");
  Serial.println(EEPROM.read(store_timer_addr[0]));
  Serial.print("Timer Minutes: ");
  Serial.println(EEPROM.read(store_timer_addr[1]));
  Serial.print("Timer Seconds: ");
  Serial.println(EEPROM.read(store_timer_addr[2]));
  Serial.print("Servo Hours: ");
  Serial.println(EEPROM.read(store_servo_timer_addr[0]));
  Serial.print("Servo Minutes: ");
  Serial.println(EEPROM.read(store_servo_timer_addr[1]));
  Serial.print("Servo Seconds: ");
  Serial.println(EEPROM.read(store_servo_timer_addr[2]));
}

/*#######################################################*/
/* Function to read weight */
/*#######################################################*/
void read_weight() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0;  // Increase value to slow down serial print activity

  /******************************************************************/
  // Check for new data/start next conversion:
  /******************************************************************/
  if (LoadCell.update()) newDataReady = true;

  /******************************************************************/
  // Get smoothed value from the dataset:
  /******************************************************************/
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float rawWeight = LoadCell.getData();  // Get weight in grams
      weight = rawWeight / 1000.0;           // Convert to kilograms
      if (weight < 0.0) weight = 0.0;        // Clamp negative values to zero
      Serial.print("Weight (kg): ");
      Serial.println(weight, 2);  // Print with 1 decimal place
      newDataReady = 0;
      t = millis();
    }
  }
  /******************************************************************/

  /******************************************************************/
  // Compare read weight to set weight
  /******************************************************************/
  if (weight > set_weight) {
    digitalWrite(buzzer_pin, HIGH);
    Serial.println("WEIGHT REACH!");
  } else if (weight < 0.01)  /// Prevent false reading trigger at 0.00kg weight
  {
    digitalWrite(buzzer_pin, LOW);
  } else {
    digitalWrite(buzzer_pin, LOW);
  }
  /******************************************************************/
}

/*#######################################################*/
/* SETUP */
/*#######################################################*/
void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(set_button_pin, INPUT_PULLUP);
  pinMode(mode_button_pin, INPUT_PULLUP);
  pinMode(emergency_button_pin, INPUT_PULLUP);
  pinMode(pot_pin, INPUT);
  pinMode(buzzer_pin, OUTPUT);
  servo.attach(servo_pin);

  /******************************************************************/
  // Initialize loadcell HX711
  /******************************************************************/
  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  float calibrationValue;
  calibrationValue = 226.05;

  unsigned long stabilizingtime = 2000;  // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true;                  //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1)
      ;
  } else {
    LoadCell.setCalFactor(calibrationValue);  // set calibration value (float)
    Serial.println("Startup is complete");
  }
  /******************************************************************/

  servo.write(0);
  read_eeprom();
  debug_eeprom();
}


/*#######################################################*/
/* LOOP */
/*#######################################################*/
void loop() {
  emergency();
  display();

  /******************************************************************/
  if (lock_system == 0) {
    read_weight();
    select_mode();
    set_mode();
    read_potvalue();
    timer_countdown();

    //Serial.print("Mode: "); Serial.println(flag_mode);
    delay(20);
  }
  /******************************************************************/

  // Debug
  //Serial.print("Emergency: "); Serial.println(lock_system);
}