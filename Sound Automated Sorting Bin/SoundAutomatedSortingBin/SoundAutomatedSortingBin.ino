const int ledPin = 13;
unsigned int receivedValue = 0;

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  //while (!Serial); // wait for serial port to connect. Needed for native USB port only, this port is for displaying data Grove Sensor sends

  // Serial.println("USB Serial Port connection Established!");

  // Set the data rate for the SoftwareSerial port - Grove Offline Voice Recognition Module runs at 9600 baud rate
}

void loop() {

  if (Serial.available()) {
    Serial.println("Data From VC-02");
    // Read the incoming bytes
    byte highByte = Serial.read();
    byte lowByte = Serial.read();

    // Combine the two bytes into a single 16-bit value
    receivedValue = (highByte << 8) | lowByte;

    // Print the received value in HEX format
    Serial.print("Received HEX value: 0x");
    Serial.println(receivedValue, HEX);
  }

  if (receivedValue == 0xAA11)  // Check if the value is A2 in HEX
  {
    digitalWrite(ledPin, HIGH);
    delay(10);
  } 
  if (receivedValue == 0xAA00) {
    digitalWrite(ledPin, LOW);
    delay(10);
  } else {
  }
  delay(10);
  receivedValue = 0;
}