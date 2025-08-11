#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#include <Servo.h>
#define SERVO_PIN 12
Servo servo;

#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN 5    // ESP32 pin GPIO5 for MFRC522 SDA
#define RST_PIN 27  // ESP32 pin GPIO27 for MFRC522 RST

MFRC522 rfid(SS_PIN, RST_PIN);

/*#######################################################*/
/* Wi-Fi and Telegram Credentials */
/*#######################################################*/
const char* ssid = "abcd";                                          // Your Wi-Fi SSID
const char* password = "123456789";                                 // Your Wi-Fi password
#define BOT_TOKEN "8236085233:AAHx2T4iyKA78MGEhOBEiY0waVf3eSs-HQw"  // Your BotFather token
#define CHAT_ID "1619383256"                                        // Your Telegram User ID

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

int lock_send = 0;

/*#######################################################*/
/* Replace with your RFID tag's UID (4 bytes) */
/*#######################################################*/
byte authorizedUID[4] = { 0x97, 0x35, 0x43, 0x02 };
int count_scan = 0;
int lastCountScan = 0;  // Track previous count_scan to detect changes

/*#######################################################*/
/* Pinout */
/*#######################################################*/
const int green_led_pin = 25;
const int red_led_pin = 26;
const int buzzer_pin = 2;

/*#######################################################*/
/* Servo Variable */
/*#######################################################*/
int open_angle = 180;
int close_angle = 0;

/*#######################################################*/
/* Timing Variables for millis() */
/*#######################################################*/
//RFID
unsigned long lastRFIDScan = 0;           // Timestamp of last RFID scan
const unsigned long scanInterval = 1000;  // 1000ms interval between RFID scans

//Buzzer
unsigned long buzzerStartTime = 0;          // Timestamp when buzzer/red LED starts
const unsigned long buzzerDuration = 5000;  // 5000ms for buzzer/red LED
bool buzzerActive = false;                  // Tracks if buzzer/red LED is active

//Telegram
unsigned long lastMessageSent = 0;           // Timestamp of last Telegram message
const unsigned long messageCooldown = 1000;  // 1000ms cooldown for messages

/*#######################################################*/
/* Function for read RFID card UID */
/*#######################################################*/
void readRFID() {
  // Only read RFID if enough time has passed since last scan
  if (millis() - lastRFIDScan < scanInterval) {
    return;  // Not enough time has passed
  }

  // Check for new RFID card
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;  // No card present, exit function
  }

  /******************************************************************/
  // Read card type
  /******************************************************************/
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.print("RFID/NFC Tag Type: ");
  Serial.println(rfid.PICC_GetTypeName(piccType));

  /******************************************************************/
  // Check if tag is MIFARE Classic
  /******************************************************************/
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println("This tag is not MIFARE Classic.");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    lastRFIDScan = millis();  // Update scan timestamp
    return;
  }

  /******************************************************************/
  // Check if UID matches authorized UID
  /******************************************************************/
  bool authorized = true;
  for (int i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != authorizedUID[i]) {
      authorized = false;
      break;
    }
  }

  /******************************************************************/
  // Print UID
  /******************************************************************/
  Serial.print("UID: ");
  for (int i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  /******************************************************************/
  // Handle authorized/unauthorized tag
  /* @ count_scan = 1 : First valid card scanned (OPEN servo 180 degree)
     @ count_scan = 2 : Second valid card scanned (CLOSE servo 0 degree) */
  /******************************************************************/
  if (authorized) {
    Serial.println("Authorized Tag");
    if (count_scan == 0) {
      count_scan = 1;  // First scan
      digitalWrite(buzzer_pin, HIGH);
      delay(100);
      digitalWrite(buzzer_pin, LOW);
    } else if (count_scan == 1) {
      count_scan = 2;  // Second scan
      digitalWrite(buzzer_pin, HIGH);
      delay(100);
      digitalWrite(buzzer_pin, LOW);
    }
    buzzerActive = false;  // Ensure buzzer is off for authorized tag
    digitalWrite(buzzer_pin, LOW);
    digitalWrite(red_led_pin, LOW);
  } else {
    Serial.println("Unauthorized Tag");
    count_scan = 0;
    buzzerActive = true;         // Activate buzzer and red LED
    buzzerStartTime = millis();  // Record start time
    digitalWrite(buzzer_pin, HIGH);
    digitalWrite(green_led_pin, LOW);
    digitalWrite(red_led_pin, HIGH);
    servo.write(close_angle);

    if (millis() - lastMessageSent >= messageCooldown) {
      bot.sendMessage(CHAT_ID, "Unauthorized Tag");
      lastMessageSent = millis();
    }
  }

  /******************************************************************/
  // Clean up
  /******************************************************************/
  rfid.PICC_HaltA();        // Halt PICC
  rfid.PCD_StopCrypto1();   // Stop encryption
  lastRFIDScan = millis();  // Update scan timestamp
}

/*#######################################################*/
/* SETUP */
/*#######################################################*/
void setup() {
  Serial.begin(115200);
  SPI.begin();      // Initialize SPI bus
  rfid.PCD_Init();  // Initialize MFRC522

  /******************************************************************/
  // Initialize Wi-Fi
  /******************************************************************/
  WiFi.begin(ssid, password);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(green_led_pin, OUTPUT);
  pinMode(red_led_pin, OUTPUT);
  pinMode(buzzer_pin, OUTPUT);

  servo.attach(SERVO_PIN);   // Attach servo to pin
  servo.write(close_angle);  // Set initial servo position to 0°
  digitalWrite(green_led_pin, LOW);
  digitalWrite(red_led_pin, LOW);
  digitalWrite(buzzer_pin, LOW);

  Serial.println("Tap an RFID/NFC tag on the reader");

  if (millis() - lastMessageSent >= messageCooldown) {
    bot.sendMessage(CHAT_ID, "System Initialized. Tap an RFID tag.", "");
    lastMessageSent = millis();
  }
}

/*#######################################################*/
/* LOOP */
/*#######################################################*/
void loop() {
  readRFID();

  /******************************************************************/
  // Rotate servo based on scan
  /******************************************************************/
  if (count_scan != lastCountScan) {
    if (count_scan == 1) {
      lock_send = 1;
    } else if (count_scan == 2) {
      lock_send = 2;
    }
    lastCountScan = count_scan;
  }

  if (count_scan == 1) {
    Serial.println("Servo : OPEN (180)");
    servo.write(open_angle);
    digitalWrite(green_led_pin, HIGH);
    digitalWrite(red_led_pin, LOW);
  } else if (count_scan == 2) {
    Serial.println("Servo : CLOSE (0)");
    servo.write(close_angle);
    digitalWrite(green_led_pin, LOW);
    digitalWrite(red_led_pin, LOW);
    count_scan = 0;
  } else if (count_scan == 0) {
    Serial.println("Servo : CLOSE (0)");
    servo.write(close_angle);
  }

  /******************************************************************/
  // Rotate servo based on scan
  /******************************************************************/
  if (lock_send == 1 && (millis() - lastMessageSent >= messageCooldown)) {
    bot.sendMessage(CHAT_ID, "On - Rotated 180");
    lastMessageSent = millis();
    lock_send = 0;
  } else if (lock_send == 2 && (millis() - lastMessageSent >= messageCooldown)) {
    bot.sendMessage(CHAT_ID, "Off - Rotated 0");
    lastMessageSent = millis();
    lock_send = 0;
  }

  /******************************************************************/
  // Handle buzzer and red LED timeout for unauthorized tag
  /******************************************************************/
  if (buzzerActive && (millis() - buzzerStartTime >= buzzerDuration)) {
    digitalWrite(buzzer_pin, LOW);
    digitalWrite(red_led_pin, LOW);
    buzzerActive = false;  // Reset buzzer state
  }
}