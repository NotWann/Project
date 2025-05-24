#include <SoftwareSerial.h>

SoftwareSerial BTSerial(10, 11); // RX, TX

const int ledGreenPin = 2;
const int ledRedPin = 4;
const int buzzerPin = 3;
int lockPir = 0;
unsigned long previousMillis = 0; // For non-blocking delay
const long interval = 200; // Blink interval in milliseconds (0.2 seconds)

void setup() {
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(115200);
  BTSerial.begin(115200); // HC-05 baud rate (match your AT+UART setting)

  Serial.println("Slave ready.");
  digitalWrite(ledRedPin, HIGH); // Default state: red LED on, no motion
  digitalWrite(ledGreenPin, LOW);
  digitalWrite(buzzerPin, LOW);
}

void loop() {
  unsigned long currentMillis = millis();

  // Handle Bluetooth commands
  if (BTSerial.available()) {
    char data = BTSerial.read(); // Read incoming data
    if (data == '2' && lockPir == 0) {
      lockPir = 1;
      digitalWrite(buzzerPin, HIGH);
      digitalWrite(ledRedPin, LOW);
      Serial.println("Motion detected, blinking started");
    } else if (data == '3' && lockPir == 1) {
      lockPir = 0;
      digitalWrite(ledRedPin, HIGH);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(buzzerPin, LOW);
      Serial.println("No motion detected, blinking stopped");
    }
  }

  // Continuous blinking while lockPir is 1
  if (lockPir == 1) {
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      digitalWrite(ledGreenPin, !digitalRead(ledGreenPin)); // Toggle LED state
    }
  } else {
    digitalWrite(ledGreenPin, LOW); // Ensure LED is off when lockPir is 0
  }

  delay(10); // Small delay to prevent overwhelming the loop
}