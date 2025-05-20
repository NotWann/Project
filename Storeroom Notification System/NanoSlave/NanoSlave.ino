#include <SoftwareSerial.h>
SoftwareSerial BTSerial(10, 11); // RX, TX

const int ledPin = 2; // LED connected to D3
const int buzzerPin = 3;
int lockPir = 0;

void setup() {
  pinMode(ledPin, OUTPUT); // LED as output
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(115200); // For debugging
  BTSerial.begin(115200); // HC-05 baud rate (match your AT+UART setting)

  Serial.println("Slave ready.");
}

void loop() {
  if (BTSerial.available()) {
    char data = BTSerial.read(); // Read incoming data
    if (data == '1') {
      digitalWrite(ledPin, HIGH); // Turn LED on
      Serial.println("LED ON");
    } else if (data == '0') {
      digitalWrite(ledPin, LOW); // Turn LED off
      Serial.println("LED OFF");
    }

    if (data == '2' && lockPir == 0) {
      digitalWrite(buzzerPin, HIGH);
      for (int i = 0; i < 5; i++)
      {
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
        delay(100);
      }
      Serial.println("LED Blinking");
      
      lockPir = 1;
    } 
    else if (data == '3' && lockPir == 1) {
      digitalWrite(ledPin, LOW);
      digitalWrite(buzzerPin, LOW);
      Serial.println("No Motion detected, OFF");
      lockPir = 0;
    }
  }
 // Serial.print("Bluetooth read: "); Serial.println(data);
  delay(100);
}