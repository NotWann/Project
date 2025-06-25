#include <BluetoothSerial.h>
#include <Servo.h>

BluetoothSerial SerialBT;
Servo myservo1;
Servo myservo2;
Servo myservo3;
Servo myservo4;

const int motor_A1_pin = 18;
const int motor_A2_pin = 19;
const int motor_B1_pin = 21;
const int motor_B2_pin = 22;
int motorSpeed = 200;  // initial motor speed

//
static const int servo1_pin = 12;
static const int servo2_pin = 13;
static const int servo3_pin = 14;
static const int servo4_pin = 15;
int angle_servo1 = 60;
int angle_servo2 = 0;
int angle_servo3 = 0;
int angle_servo4 = 100;


char incomingChar;

void move_forward() {
  Serial.println("FORWARD");
  analogWrite(motor_A1_pin, 0);
  analogWrite(motor_A2_pin, motorSpeed);
  analogWrite(motor_B1_pin, motorSpeed);
  analogWrite(motor_B2_pin, 0);
}

void reverse() {
  Serial.println("REVERSE");
  analogWrite(motor_A1_pin, motorSpeed);
  analogWrite(motor_A2_pin, 0);
  analogWrite(motor_B1_pin, 0);
  analogWrite(motor_B2_pin, motorSpeed);
}

void turn_right() {
  Serial.println("TURN RIGHT");
  analogWrite(motor_A1_pin, 0);
  analogWrite(motor_A2_pin, motorSpeed);
  analogWrite(motor_B1_pin, 0);
  analogWrite(motor_B2_pin, motorSpeed);
}

void turn_left() {
  Serial.println("TURN LEFT");
  analogWrite(motor_A1_pin, motorSpeed);
  analogWrite(motor_A2_pin, 0);
  analogWrite(motor_B1_pin, motorSpeed);
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
  myservo1.write(95);
  myservo2.write(90);
  myservo3.write(90);
  myservo4.write(100);
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
  //12
  if (incomingChar == 'A') {
    if (angle_servo1 < 180) angle_servo1 += 5;
    if (angle_servo1 > 180) angle_servo1 = 180;
    delay(20);
    myservo1.write(angle_servo1);
    Serial.println("Servo1 angle: " + String(angle_servo1));
  } 
  else if (incomingChar == 'B') {
    if (angle_servo1 > 0) angle_servo1 -= 5;
    if (angle_servo1 < 0) angle_servo1 = 0;
    delay(20);
    myservo1.write(angle_servo1);
    Serial.println("Servo1 angle: " + String(angle_servo1));
  }

  //13
  if (incomingChar == 'C') {
    if (angle_servo2 < 70) angle_servo2 += 5;
    if (angle_servo2 > 70) angle_servo2 = 70;
    delay(20);
    myservo2.write(angle_servo2);
    Serial.println("Servo2 angle: " + String(angle_servo2));
  } 
  else if (incomingChar == 'D') {
    if (angle_servo2 > 0) angle_servo2 -= 5;
    if (angle_servo2 < 0) angle_servo2 = 0;
    delay(20);
    myservo2.write(angle_servo2);
    Serial.println("Servo2 angle: " + String(angle_servo2));
  }

  //14
  if (incomingChar == 'E') {
    if (angle_servo3 < 65) angle_servo3 += 5;
    if (angle_servo3 > 65) angle_servo3 = 65;
    delay(20);
    myservo3.write(angle_servo3);
    Serial.println("Servo3 angle: " + String(angle_servo3));
  } 
  else if (incomingChar == 'F') {
    if (angle_servo3 > 0) angle_servo3 -= 5;
    if (angle_servo3 < 0) angle_servo3 = 0;
    delay(20);
    myservo3.write(angle_servo3);
    Serial.println("Servo3 angle: " + String(angle_servo3));
  }

  //15
  if (incomingChar == 'G') {
    if (angle_servo4 < 180) angle_servo4 += 5;
    if (angle_servo4 > 180) angle_servo4 = 180;
    delay(20);
    myservo4.write(angle_servo4);
    Serial.println("Servo4 angle: " + String(angle_servo4));
  } 
  else if (incomingChar == 'H') {
    if (angle_servo4 > 100) angle_servo4 -= 5;
    if (angle_servo4 < 100) angle_servo4 = 100;
    delay(20);
    myservo4.write(angle_servo4);
    Serial.println("Servo4 angle: " + String(angle_servo4));
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(motor_A1_pin, OUTPUT);
  pinMode(motor_A2_pin, OUTPUT);
  pinMode(motor_B1_pin, OUTPUT);
  pinMode(motor_B2_pin, OUTPUT);

  myservo1.attach(servo1_pin);
  myservo2.attach(servo2_pin);
  myservo3.attach(servo3_pin);
	myservo4.attach(servo4_pin);

  SerialBT.begin("Robot KK Jasin 2"); // Bluetooth device name
  Serial.println("Bluetooth Started! Pair with ESP32_Bluetooth");
  stop();
  reset_servo();
}

void loop() {
  read_bluetooth();

  if (incomingChar == '0') stop();
  else if (incomingChar == 'w') move_forward();
  else if (incomingChar == 's') reverse();
  else if (incomingChar == 'a') turn_left();
  else if (incomingChar == 'd') turn_right();

  else if (incomingChar == 'S') {
    motorSpeed += 5;
    if (motorSpeed > 255) motorSpeed = 255;
    Serial.println("Speed Increased: " + String(motorSpeed));
  }
  else if (incomingChar == 'L') {
    motorSpeed -= 5;
    if (motorSpeed < 100) motorSpeed = 100;
    Serial.println("Speed Decreased: " + String(motorSpeed));
  }

  write_servo();
}
