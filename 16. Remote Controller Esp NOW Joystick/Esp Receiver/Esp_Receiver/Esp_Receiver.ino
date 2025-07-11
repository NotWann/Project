#include <esp_now.h>
#include <WiFi.h>

const int motor_A1_pin = 18;
const int motor_A2_pin = 19;
const int motor_B1_pin = 21;
const int motor_B2_pin = 22;

const int trig_pin = 16;
const int echo_pin = 17;
int distance;

const int buzzer_pin = 2;

const int green_pin = 25;
const int yellow_pin = 26;
const int red_pin = 23;

// Structure to match sender's data
typedef struct struct_message {
  int xValue; 
  int yValue; 
  bool buttonPressed;
} struct_message;

struct_message joystickData;

// Buzzer state tracking
enum BlinkMode { OFF, FORWARD, REVERSE, RIGHT, LEFT }; // Modes
BlinkMode mode = OFF;    // Current mode

// Callback function for received data
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  memcpy(&joystickData, incomingData, sizeof(joystickData));
  Serial.print("X:");
  Serial.print(joystickData.xValue);
  Serial.print(",Y:");
  Serial.print(joystickData.yValue);
  Serial.print(",Button:");
  Serial.println(joystickData.buttonPressed ? 1 : 0);
}

void move_forward() {
  Serial.println("FORWARD");
  analogWrite(motor_A1_pin, 255);
  analogWrite(motor_A2_pin, 0);
  analogWrite(motor_B1_pin, 0);
  analogWrite(motor_B2_pin, 255);
}

void reverse() {
  Serial.println("BACKWARD");
  analogWrite(motor_A1_pin, 0);
  analogWrite(motor_A2_pin, 255);
  analogWrite(motor_B1_pin, 255);
  analogWrite(motor_B2_pin, 0);
}

void turn_right() {
  Serial.println("RIGHT");
  analogWrite(motor_A1_pin, 0);
  analogWrite(motor_A2_pin, 255);
  analogWrite(motor_B1_pin, 0);
  analogWrite(motor_B2_pin, 255);
}

void turn_left() {
  Serial.println("LEFT");
  analogWrite(motor_A1_pin, 255);
  analogWrite(motor_A2_pin, 0);
  analogWrite(motor_B1_pin, 255);
  analogWrite(motor_B2_pin, 0);
}

void stop() {
  Serial.println("STOP");
  analogWrite(motor_A1_pin, 0);
  analogWrite(motor_A2_pin, 0);
  analogWrite(motor_B1_pin, 0);
  analogWrite(motor_B2_pin, 0);
}

void ultrasonic() {
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);

  unsigned long duration = pulseIn(echo_pin, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Distance: "); Serial.print(distance); Serial.println(" cm");
}

void setup() {
  Serial.begin(115200); // Start Serial for output
  WiFi.mode(WIFI_STA);  // Set device as a Wi-Fi Station
  pinMode(buzzer_pin, OUTPUT);
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(yellow_pin, OUTPUT);
  pinMode(red_pin, OUTPUT);

  pinMode(motor_A1_pin, OUTPUT);
  pinMode(motor_A2_pin, OUTPUT);
  pinMode(motor_B1_pin, OUTPUT);
  pinMode(motor_B2_pin, OUTPUT);
  analogWrite(motor_A1_pin, 0);
  analogWrite(motor_A2_pin, 0);
  analogWrite(motor_B1_pin, 0);
  analogWrite(motor_B2_pin, 0);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback for received data
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  int value_x = joystickData.xValue;
  int value_y = joystickData.yValue;

  if (value_x > 500) {
    mode = FORWARD;
  } else if (value_x < 10) {
    mode = REVERSE;
  } else if (value_y > 500) {
    mode = RIGHT;
  } else if (value_y < 10) {
    mode = LEFT;
  } else if (value_x < 300 && value_y < 300) {
    mode = OFF;
  }

  // Handle motor control, LEDs, and buzzer
  switch (mode) {
    case FORWARD:
      move_forward();
      break;

    case REVERSE:
      reverse();
      ultrasonic();
      if (distance < 5) {
        digitalWrite(green_pin, LOW);
        digitalWrite(yellow_pin, LOW);
        digitalWrite(red_pin, HIGH);
        digitalWrite(buzzer_pin, HIGH);
      } else if (distance < 10) {
        digitalWrite(green_pin, LOW);
        digitalWrite(yellow_pin, HIGH);
        digitalWrite(red_pin, LOW);
        digitalWrite(buzzer_pin, LOW);
      } else if (distance >= 10) {
        digitalWrite(green_pin, HIGH);
        digitalWrite(yellow_pin, LOW);
        digitalWrite(red_pin, LOW);
        digitalWrite(buzzer_pin, LOW);
      }
      break;

    case RIGHT:
      turn_right();
      break;

    case LEFT:
      turn_left();
      break;

    case OFF:
      stop();
      digitalWrite(buzzer_pin, LOW);
      digitalWrite(green_pin, HIGH);
      digitalWrite(yellow_pin, LOW);
      digitalWrite(red_pin, LOW);
      break;
  }
}