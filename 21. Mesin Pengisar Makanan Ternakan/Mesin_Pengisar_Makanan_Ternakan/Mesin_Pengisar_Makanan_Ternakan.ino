#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <HX711_ADC.h>
#include <EEPROM.h>
 
LiquidCrystal_I2C lcd(0x27, 20, 4);
Servo servo;

const int HX711_dout = 4; 
const int HX711_sck = 5; 

HX711_ADC LoadCell(HX711_dout, HX711_sck);

unsigned long t = 0;

const int emergency_button = 10;
int lock_system = 0;

const int set_button = 8;
const int mode_button = 7;
int last_set_state = HIGH;
int last_mode_state = HIGH;
int flag_set = 0;
int flag_mode = 0;

const int led_pin = 13;

float weight = 0;     // Reading weight in kg
float set_weight = 0;; // Set weight in kg
int timer = 0;          // Countdown timer 
int set_timer = 0;    // Servo rotation time 
unsigned long countdownStartTime = 0; // Track when countdown starts

// Countdown timer
int hours = 0;
int minutes = 0;
int seconds = 0;
unsigned long lastTick = 0;

//EEPROM store address
int store_weight_addr = 0;
int store_timer_addr[3] = {4, 5, 6};
int store_servo_timer_addr[3] = {7, 8, 9};

static const int servo_pin = 3;

// Servo rotation time
int servo_rotated_hours = 0;
int servo_rotated_minutes = 0;
int servo_rotated_seconds = 0;
// Servo reset angle time
const unsigned long reset_servo_duration = 30000;
unsigned long last_servo_rotated_duration = 0; 
int lock_servo = 0;

// Potentiometer
const int pot_pin = A0; // Changed to analog pin A0
int map_pot_value = 0;
float smoothedPot = 0;
float alpha = 0.1;      // Smoothing factor

int smooth_pot_value() {
  int raw = analogRead(pot_pin);
  smoothedPot = alpha * raw + (1 - alpha) * smoothedPot;
  return (int)smoothedPot;
}

void read_potvalue() {
  int pot_value = smooth_pot_value();
  //Serial.print("Raw Pot: "); Serial.println(pot_value);
  if (flag_mode == 1) {
    if (flag_set == 1) {
      set_weight = constrain(map(pot_value, 0, 1000, 0, 10000) / 1000.0, 0.00, 10.00); // kg (float)
      save_eeprom(1);
      Serial.print("Set Weight: "); Serial.print(set_weight); Serial.println(" kg");
    }
    else if (flag_set == 2) {
      hours = constrain(map(pot_value, 0, 1000, 0, 24), 0, 24); // 0-23 hours
      timer = hours * 3600 + minutes * 60 + seconds; // Update timer
      save_eeprom(2);
      Serial.print("Set Hours: "); Serial.println(hours);
    }
    else if (flag_set == 3) {
      minutes = constrain(map(pot_value, 0, 1000, 0, 59), 0, 59);
      timer = hours * 3600 + minutes * 60 + seconds; // Update timer
      save_eeprom(3);
      Serial.print("Set Minutes: "); Serial.println(minutes);
    }
    else if (flag_set == 4) {
      seconds = constrain(map(pot_value, 0, 1000, 0, 59), 0, 59);
      timer = hours * 3600 + minutes * 60 + seconds; // Update timer
      save_eeprom(4);
      Serial.print("Set Seconds: "); Serial.println(seconds);
    }
    else if (flag_set == 5) {
      servo_rotated_hours = constrain(map(pot_value, 0, 1000, 0, 24), 0, 24);
      set_timer = servo_rotated_hours * 3600 + servo_rotated_minutes * 60 + servo_rotated_seconds; // Update set_timer
      save_eeprom(5);
      Serial.print("Set Servo Hours: "); Serial.println(servo_rotated_hours);
    }
    else if (flag_set == 6) {
      servo_rotated_minutes = constrain(map(pot_value, 0, 1000, 0, 59), 0, 59);
      set_timer = servo_rotated_hours * 3600 + servo_rotated_minutes * 60 + servo_rotated_seconds; // Update set_timer
      save_eeprom(6);
      Serial.print("Set Servo Minutes: "); Serial.println(servo_rotated_minutes);
    }
    else if (flag_set == 7) {
      servo_rotated_seconds = constrain(map(pot_value, 0, 1000, 0, 59), 0, 59);
      set_timer = servo_rotated_hours * 3600 + servo_rotated_minutes * 60 + servo_rotated_seconds; // Update set_timer
      save_eeprom(7);
      Serial.print("Set Servo Seconds: "); Serial.println(servo_rotated_seconds);
    }
  }
}

void emergency() {
  int current_emergency_state = digitalRead(emergency_button);
  if (current_emergency_state == LOW && lock_system == 0)
  {
    lcd.clear();
    lock_system = 1;
    servo.write(0);
    lock_servo = 0;
    flag_mode = 0;
    flag_set = 0;
  }
  else if (current_emergency_state == HIGH && lock_system == 1)
  {
    lock_system = 0;
  }
}

void select_mode() {
  int current_mode_state = digitalRead(mode_button);
  static unsigned long pressStartTime = 0; // Track when button is pressed
  const unsigned long longPressDuration = 1000; // 1 second for long press

  // Detect button press (HIGH to LOW transition)
  if (current_mode_state == LOW && last_mode_state == HIGH) {
    pressStartTime = millis(); // Record press start time
  }

  // Detect button hold (still LOW after transition)
  if (current_mode_state == LOW && last_mode_state == LOW) {
    if (millis() - pressStartTime >= longPressDuration) {
      // lcd.clear();
      flag_mode = 3; // Set to mode 3 for long press
      flag_set = 0;  // Reset set flag
      countdownStartTime = millis(); // Record countdown start time
      digitalWrite(led_pin, HIGH);
      delay(100); 
      digitalWrite(led_pin, LOW);
      Serial.println("Start TIME");
      delay(50); // Debounce
      // Wait for button release to avoid repeated triggers
      while (digitalRead(mode_button) == LOW) {}
      delay(50); // Debounce release
    }
  }

  // Detect button release (LOW to HIGH transition) for short press
  if (current_mode_state == HIGH && last_mode_state == LOW) {
    // Check if it was a short press (less than 1 second)
    if (millis() - pressStartTime < longPressDuration) {
      lcd.clear();
      digitalWrite(led_pin, HIGH);
      delay(100); 
      digitalWrite(led_pin, LOW);
      flag_mode = (flag_mode + 1) % 2; // Toggle between 0 and 1
      flag_set = 0; // Reset set flag
      Serial.println(flag_mode == 0 ? "Mode: Main Display" : "Mode: Set Mode");
      delay(50); // Debounce
    }
  }

  last_mode_state = current_mode_state;
}

void set_mode() {
  int current_set_state = digitalRead(set_button);
  if (current_set_state == LOW && last_set_state == HIGH && flag_mode == 1) {
    flag_set = (flag_set + 1) % 8;
    digitalWrite(led_pin, HIGH);
    delay(100); 
    digitalWrite(led_pin, LOW);
    //lcd.clear();
    delay(50); // Debounce
  }
  last_set_state = current_set_state;
}

// Helper function to format seconds into hours, minutes, seconds
void format_time(int total_seconds, int &h, int &m, int &s) {
  h = total_seconds / 3600;
  m = (total_seconds % 3600) / 60;
  s = total_seconds % 60;
}

void display() {
  lcd.setCursor(0, 0);
  if (flag_mode == 0 || flag_mode == 3) {
    lcd.print("Weight: "); lcd.print(weight, 2); lcd.print("kg  ");
    lcd.setCursor(0, 1);
    lcd.print("Set Weight: "); lcd.print(set_weight, 2); lcd.print("kg  ");
    lcd.setCursor(0, 2);
    lcd.print("Timer: ");
    int t_h, t_m, t_s;
    format_time(timer, t_h, t_m, t_s);
    lcd.print(t_h < 10 ? "0" : ""); lcd.print(t_h); lcd.print(":");
    lcd.print(t_m < 10 ? "0" : ""); lcd.print(t_m); lcd.print(":");
    lcd.print(t_s < 10 ? "0" : ""); lcd.print(t_s); lcd.print("  ");
    lcd.setCursor(0, 3);
    lcd.print("Servo: ");
    int st_h, st_m, st_s;
    format_time(set_timer, st_h, st_m, st_s);
    lcd.print(st_h < 10 ? "0" : ""); lcd.print(st_h); lcd.print(":");
    lcd.print(st_m < 10 ? "0" : ""); lcd.print(st_m); lcd.print(":");
    lcd.print(st_s < 10 ? "0" : ""); lcd.print(st_s); lcd.print("  ");
  }
  else if (flag_mode == 1) {
    lcd.print(flag_set == 1 ? ">Set Weight: " : "Set Weight: ");
    lcd.print(set_weight, 2); lcd.print("kg  ");
    lcd.setCursor(0, 1);
    lcd.print(flag_set == 2 || flag_set == 3 || flag_set == 4 ? ">Set Time: " : "Set Time: ");
    lcd.print(hours < 10 ? "0" : ""); lcd.print(hours); lcd.print(":");
    lcd.print(minutes < 10 ? "0" : ""); lcd.print(minutes); lcd.print(":");
    lcd.print(seconds < 10 ? "0" : ""); lcd.print(seconds); lcd.print("  ");
    lcd.setCursor(0, 2);
    lcd.print(flag_set == 5 || flag_set == 6 || flag_set == 7 ? ">Set Servo: " : "Set Servo: ");
    lcd.print(servo_rotated_hours < 10 ? "0" : ""); lcd.print(servo_rotated_hours); lcd.print(":");
    lcd.print(servo_rotated_minutes < 10 ? "0" : ""); lcd.print(servo_rotated_minutes); lcd.print(":");
    lcd.print(servo_rotated_seconds < 10 ? "0" : ""); lcd.print(servo_rotated_seconds); lcd.print("  ");
    
  }
}

void timer_countdown() {
  unsigned long now = millis();
  static unsigned long lastLedOnTime = 0; // Track LED on time for non-blocking blink
  static bool ledIsOn = false; // Track LED state
  const unsigned long ledBlinkDuration = 100; // LED blink duration (ms)

  // Handle non-blocking LED blinking
  if (ledIsOn && now - lastLedOnTime >= ledBlinkDuration) {
    digitalWrite(led_pin, LOW);
    ledIsOn = false;
  }

  // Timer countdown
  if (now - lastTick >= 1000 && flag_mode == 3) {
    lastTick = now;

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
      timer = hours * 3600 + minutes * 60 + seconds; // Sync timer
      Serial.print("Time left: ");
      Serial.print(hours); Serial.print("h ");
      Serial.print(minutes); Serial.print("m ");
      Serial.print(seconds); Serial.println("s");

      // Calculate elapsed time since countdown started (in seconds)
      unsigned long elapsedTime = (now - countdownStartTime) / 1000;

      // Rotate servo if elapsed time is a multiple of set_timer and not locked
      if (set_timer > 0 && elapsedTime % set_timer == 0 && elapsedTime > 0 && lock_servo == 0) {
        // Ensure we only trigger once per interval
        static unsigned long lastRotationTime = 0;
        if (elapsedTime != lastRotationTime) { // Prevent multiple triggers in the same second
          Serial.println("Servo Rotated!");
          digitalWrite(led_pin, HIGH);
          ledIsOn = true;
          lastLedOnTime = now;
          servo.write(100);
          lock_servo = 1;
          last_servo_rotated_duration = now; // Update rotation time
          lastRotationTime = elapsedTime; // Track the last rotation time
        }
      }
    } else {
      Serial.println("Time's up!");
      hours = timer / 3600;
      minutes = (timer % 3600) / 60;
      seconds = timer % 60;
      flag_mode = 0; // Return to main mode
      servo.write(0); // Ensure servo resets
    }
  }

  // Check for servo reset independently
  if (lock_servo == 1 && now - last_servo_rotated_duration >= reset_servo_duration) {
    lock_servo = 0;
    digitalWrite(led_pin, HIGH);
    ledIsOn = true;
    lastLedOnTime = now;
    servo.write(0);
    Serial.println("Servo Reset!");
  }
}

void read_eeprom() {
  set_weight = EEPROM.get(store_weight_addr, set_weight);
  hours = EEPROM.read(store_timer_addr[0]);
  minutes = EEPROM.read(store_timer_addr[1]);
  seconds = EEPROM.read(store_timer_addr[2]);
  servo_rotated_hours = EEPROM.read(store_servo_timer_addr[0]);
  servo_rotated_minutes = EEPROM.read(store_servo_timer_addr[1]);
  servo_rotated_seconds = EEPROM.read(store_servo_timer_addr[2]);

  // Recalculate timer and set_timer from their components
  timer = hours * 3600 + minutes * 60 + seconds;
  set_timer = servo_rotated_hours * 3600 + servo_rotated_minutes * 60 + servo_rotated_seconds;
}

void save_eeprom(int num) {
  if (num == 1) EEPROM.put(store_weight_addr, set_weight);
  else if (num == 2) EEPROM.update(store_timer_addr[0], hours);
  else if (num == 3) EEPROM.update(store_timer_addr[1], minutes);
  else if (num == 4) EEPROM.update(store_timer_addr[2], seconds);
  else if (num == 5) EEPROM.update(store_servo_timer_addr[0], servo_rotated_hours);
  else if (num == 6) EEPROM.update(store_servo_timer_addr[1], servo_rotated_minutes); // Fixed
  else if (num == 7) EEPROM.update(store_servo_timer_addr[2], servo_rotated_seconds); // Fixed
}

void debug_eeprom() {
  Serial.println("EEPROM Contents:");
  Serial.print("Weight: "); Serial.println(EEPROM.read(store_weight_addr));
  Serial.print("Timer Hours: "); Serial.println(EEPROM.read(store_timer_addr[0]));
  Serial.print("Timer Minutes: "); Serial.println(EEPROM.read(store_timer_addr[1]));
  Serial.print("Timer Seconds: "); Serial.println(EEPROM.read(store_timer_addr[2]));
  Serial.print("Servo Hours: "); Serial.println(EEPROM.read(store_servo_timer_addr[0]));
  Serial.print("Servo Minutes: "); Serial.println(EEPROM.read(store_servo_timer_addr[1]));
  Serial.print("Servo Seconds: "); Serial.println(EEPROM.read(store_servo_timer_addr[2]));
}

void read_weight() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; // Increase value to slow down serial print activity

  // Check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // Get smoothed value from the dataset:
  if (newDataReady) 
  {
    if (millis() > t + serialPrintInterval) 
    {
      float rawWeight = LoadCell.getData(); // Get weight in grams
      weight = rawWeight / 1000.0; // Convert to kilograms
      if (weight < 0.0) weight = 0.0; // Clamp negative values to zero
      Serial.print("Weight (kg): ");
      Serial.println(weight, 2); // Print with 1 decimal place
      newDataReady = 0;
      t = millis();
    }
  }

  if (weight > set_weight) 
  {
    digitalWrite(led_pin, HIGH);
    Serial.println("WEIGHT REACH!");
  } 
  else if (weight < 0.01)
  {
    digitalWrite(led_pin, LOW);
  }
  else 
  {
    digitalWrite(led_pin, LOW);
  }
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  pinMode(led_pin, OUTPUT);
  pinMode(set_button, INPUT_PULLUP);
  pinMode(mode_button, INPUT_PULLUP);
  pinMode(emergency_button, INPUT_PULLUP);
  pinMode(pot_pin, INPUT);
  servo.attach(servo_pin);

  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  float calibrationValue; 
  calibrationValue = 226.05; 

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
  servo.write(0);
  read_eeprom();
  debug_eeprom();
}

void loop() {
  emergency();
  display();
  if (lock_system == 0)
  {
    read_weight();
    select_mode();
    set_mode();
    read_potvalue();
    timer_countdown();

    //Serial.print("Mode: "); Serial.println(flag_mode);
    delay(20);
  }
  //Serial.print("Emergency: "); Serial.println(lock_system);
}