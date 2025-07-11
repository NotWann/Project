#include "HX710B.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#include <Servo.h>
static const int servoPin = 12;
Servo servo1;
int lock_servo;
int count_servo;

const int DOUT = 26;   //sensor data pin
const int SCLK  = 25;   //sensor clock pin

HX710B pressure_sensor; 

float PSI_Cal = 0;
float KPI_1psi = 6.89476; 
float KPI_Cal = 0;

int maintain_pressure = 12; // SETTING PRESSURRE (MAX 40)

const int trigPin = 32; 
const int echoPin = 27; 
long duration;
int distance;

const int buttonPin = 23;
int buttonState = 0; 
int flag_1 = 0;

const int pump =  13; 

int buzzer = 2;



void setup() 
{
  Serial.begin(9600);
  servo1.attach(servoPin);
  lcd.init();    
  lcd.backlight(); 
  pressure_sensor.begin(DOUT, SCLK);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pump, OUTPUT);
  pinMode(buzzer, OUTPUT);
}

void loop() 
{
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && flag_1 == 0) 
  {
    digitalWrite(pump, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    flag_1 = 1;
  }
  else if (buttonState == HIGH && flag_1 == 1) 
  {
    digitalWrite(pump, LOW);
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    flag_1 = 0;
  }

  servo_rotated();
  pressure_read();
  jarak_atas();
  delay(100);
}

void pressure_read()
{
  //if (pressure_sensor.is_ready()) 
  //{
    //Serial.print("Pascal: ");
    //Serial.print(pressure_sensor.pascal());
   // Serial.print("  ATM: ");
    //Serial.print(pressure_sensor.atm());
   // Serial.print("  mmHg: ");
   // Serial.print(pressure_sensor.mmHg()-3.85);
   // Serial.print("  PSI: ");
   // Serial.println(pressure_sensor.psi());

    PSI_Cal = (pressure_sensor.mmHg()-3.85);

    if(PSI_Cal < 0.05)
    {
      PSI_Cal = 0;
      KPI_Cal = 0;
    }
    else
    {
      PSI_Cal = PSI_Cal + 0.5;
      KPI_Cal = PSI_Cal * KPI_1psi;
    }

    Serial.print("PSI: ");
    Serial.println(PSI_Cal);
    Serial.print("  Kilopascal: ");
    Serial.println(KPI_Cal);

    lcd.setCursor(0, 1);
    lcd.print("P.sure:");  
    lcd.print(KPI_Cal);  
    lcd.print(" Kpa ");  
}

void jarak_atas()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculating the distance
  distance = duration*0.034/2;
  
  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.println(distance);

    lcd.setCursor(0, 0);
    lcd.print("Distance:");  
    lcd.print(distance);  
    lcd.print(" cm ");  
}

void servo_rotated()
{
  if(lock_servo == 0)
  {
    count_servo = count_servo + 10;
    if(count_servo >= 180)
    {
      lock_servo = 1;
    }
  }
  if(lock_servo == 1)
  {
    count_servo = count_servo - 10;
    if(count_servo <= 0)
    {
      lock_servo = 0;
    }
  }
  servo1.write(count_servo);
}
