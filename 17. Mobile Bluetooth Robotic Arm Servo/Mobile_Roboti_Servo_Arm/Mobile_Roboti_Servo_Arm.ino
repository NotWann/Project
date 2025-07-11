#include <BluetoothSerial.h>
#include <ESP32Servo.h>

BluetoothSerial SerialBT;
Servo myservo1;
Servo myservo2;
Servo myservo3;
Servo myservo4;

const int motor_A1_pin = 18;
const int motor_A2_pin = 19;
const int motor_B1_pin = 21;
const int motor_B2_pin = 22;

//
const int servo1_pin = 12;
const int servo2_pin = 13;
const int servo3_pin = 14;
const int servo4_pin = 15;
int angle_servo1 = 0;
int angle_servo2 = 0;
int angle_servo3 = 0;
int angle_servo4 = 0;


char incomingChar;

void move_forward() {
  Serial.println("FORWARD");
  analogWrite(motor_A1_pin, 0);
  analogWrite(motor_A2_pin, 255);
  analogWrite(motor_B1_pin, 255);
  analogWrite(motor_B2_pin, 0);
}

void reverse() {
  Serial.println("REVERSE");
  analogWrite(motor_A1_pin, 255);
  analogWrite(motor_A2_pin, 0);
  analogWrite(motor_B1_pin, 0);
  analogWrite(motor_B2_pin, 255);
}

void turn_right() {
  Serial.println("TURN RIGHT");
  analogWrite(motor_A1_pin, 0);
  analogWrite(motor_A2_pin, 255);
  analogWrite(motor_B1_pin, 0);
  analogWrite(motor_B2_pin, 255);
}

void turn_left() {
  Serial.println("TURN LEFT");
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

void reset_servo() {
  myservo1.write(0);
  myservo2.write(0);
  myservo3.write(0);
  myservo4.write(0);
}

void read_bluetooth() {
  // Read from Bluetooth and send to Serial Monitor
  if (SerialBT.available()) 
  {
    incomingChar = SerialBT.read();
    Serial.print("Received: ");
    Serial.println(incomingChar);
  }
}

void write_servo() {
  if (incomingChar == 'A')
  {
    angle_servo1 += 1;
    delay(20);
    myservo1.write(angle_servo1);
    Serial.println("Servo1 angle: " + String(angle_servo1));
  } 
  else if (incomingChar == 'B') 
  {
    angle_servo1 -= 1;
    delay(20);
    myservo1.write(angle_servo1);
    Serial.println("Servo1 angle: " + String(angle_servo1));
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(motor_A1_pin, OUTPUT);
  pinMode(motor_A2_pin, OUTPUT);
  pinMode(motor_B1_pin, OUTPUT);
  pinMode(motor_B2_pin, OUTPUT);

  myservo1.setPeriodHertz(50);
	myservo2.setPeriodHertz(50);
  myservo3.setPeriodHertz(50);
	myservo4.setPeriodHertz(50);
  myservo1.attach(servo1_pin, 1000, 2000);
  myservo2.attach(servo2_pin, 1000, 2000);
  myservo3.attach(servo3_pin, 1000, 2000);
	myservo4.attach(servo4_pin, 1000, 2000);

  SerialBT.begin("ESP32_Bluetooth"); // Bluetooth device name
  Serial.println("Bluetooth Started! Pair with ESP32_Bluetooth");
  stop();
}

void loop() {
  read_bluetooth();
  if (incomingChar == '0') stop();
  else if (incomingChar == 'w') move_forward();
  else if (incomingChar == 's') reverse();
  else if (incomingChar == 'a') turn_left();
  else if (incomingChar == 'd') turn_right();
  write_servo();
}