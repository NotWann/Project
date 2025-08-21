#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

#define SS_PIN 10
#define RST_PIN 9
SoftwareSerial mySerial(2, 3); // RX, TX (Pin 2 as RX, Pin 3 as TX)

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
byte nuidPICC[4];

unsigned long lastScanTime = 0;
const unsigned long scanInterval = 3000; // 3 seconds

void setup() { 
  Serial.begin(9600); // For debugging to Serial Monitor
  mySerial.begin(9600); // For UART to Mega
  while (!Serial);   // Wait for serial port to connect
  SPI.begin();       // Init SPI bus
  rfid.PCD_Init();   // Init MFRC522 

  Serial.println(F("This code scans the MIFARE Classic NUID and sends a value via UART."));
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent())
    return;

  if (!rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  // Check if enough time has passed or if it's a new card
  if (millis() - lastScanTime > scanInterval || 
      rfid.uid.uidByte[0] != nuidPICC[0] || 
      rfid.uid.uidByte[1] != nuidPICC[1] || 
      rfid.uid.uidByte[2] != nuidPICC[2] || 
      rfid.uid.uidByte[3] != nuidPICC[3]) {
    Serial.println(F("A new card has been detected or timeout expired."));

    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
   
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();

    byte valueToSend = 255; // Default value (invalid)
    if (rfid.uid.uidByte[0] == 0x03 && rfid.uid.uidByte[1] == 0xEC && 
        rfid.uid.uidByte[2] == 0x5C && rfid.uid.uidByte[3] == 0xFF) {
      valueToSend = 2; // UID: FC5E9832
    } else if (rfid.uid.uidByte[0] == 0x31 && rfid.uid.uidByte[1] == 0x1C && 
               rfid.uid.uidByte[2] == 0xB1 && rfid.uid.uidByte[3] == 0x26) {
      valueToSend = 1; // UID: B3A3CD4F
    }

    if (valueToSend != 255) {
      Serial.print(F("Sending value: "));
      Serial.println(valueToSend);
      mySerial.write(valueToSend); // Send via SoftwareSerial
      delay(100); // Ensure Mega receives it
    } else {
      Serial.println(F("Unknown UID, no value sent."));
    }

    lastScanTime = millis(); // Update the last scan time
  } else {
    Serial.println(F("Card read recently."));
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
     Serial.print(buffer[i] < 0x10 ? " 0" : " ");
     Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(' ');
    Serial.print(buffer[i], DEC);
  }
}