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
#define CHAT_ID "1091852290"                                        // Your Telegram User ID

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

/*#######################################################*/
/* Replace with your RFID tag's UID (4 bytes) */
/*#######################################################*/
byte authorizedUID[4] = { 0x97, 0x35, 0x43, 0x02 };
int count_scan = 0;
int lastCountScan = -1;  // Track last count_scan state (-1 for initial state)

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
unsigned long lastRFIDScan = 0;               // Timestamp of last RFID scan
const unsigned long scanInterval = 1000;      // 1000ms interval between RFID scans
unsigned long buzzerStartTime = 0;            // Timestamp when buzzer/red LED starts
const unsigned long buzzerDuration = 5000;    // 5000ms for buzzer/red LED
bool buzzerActive = false;                    // Tracks if buzzer/red LED is active
unsigned long lastBotCheck = 0;               // Timestamp of last Telegram check
const unsigned long botCheckInterval = 1000;  // 1000ms for Telegram polling
unsigned long lastMessageSent = 0;            // Timestamp of last Telegram message
const unsigned long messageCooldown = 1000;   // 1000ms cooldown for messages
bool lastUnauthorizedSent = false;            // Tracks if unauthorized message was sent
bool lastNonMifareSent = false;               // Tracks if non-MIFARE message was sent

/*#######################################################*/
/* Function to reinitialize RFID module */
/*#######################################################*/
void resetRFID() {
  rfid.PCD_Init();                           // Reinitialize MFRC522
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);  // Maximize antenna gain
  Serial.println("RFID module reinitialized");
}

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

  // Reset message flags for new scan
  lastUnauthorizedSent = false;
  lastNonMifareSent = false;

  /******************************************************************/
  // Read card type
  /******************************************************************/
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.print("RFID/NFC Tag Type: ");
  Serial.println(rfid.PICC_GetTypeName(piccType));

  /******************************************************************/
  // Check if tag is MIFARE Classic
  /******************************************************************/
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && 
      piccType != MFRC522::PICC_TYPE_MIFARE_1K && 
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println("This tag is not MIFARE Classic.");
    // Send Telegram message after cooldown
    if (!lastNonMifareSent && millis() - lastMessageSent >= messageCooldown) {
      bot.sendMessage(CHAT_ID, "This tag is not MIFARE Classic.", "");
      lastMessageSent = millis();
      lastNonMifareSent = true;
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    lastRFIDScan = millis();
    resetRFID();  // Reinitialize RFID module
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
  String uidString = "";
  for (int i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
    uidString += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i], HEX);
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
      // Prioritize hardware actions
      servo.write(open_angle);
      digitalWrite(green_led_pin, HIGH);
      digitalWrite(red_led_pin, LOW);
      digitalWrite(buzzer_pin, LOW);
      buzzerActive = false;
      Serial.println("Servo : OPEN (180)");
    } else if (count_scan == 1) {
      count_scan = 2;  // Second scan
      // Prioritize hardware actions
      servo.write(close_angle);
      digitalWrite(green_led_pin, LOW);
      digitalWrite(red_led_pin, LOW);
      digitalWrite(buzzer_pin, LOW);
      buzzerActive = false;
      Serial.println("Servo : CLOSE (0)");
    }
    /******************************************************************/
    // Send Telegram message only on count_scan change and after cooldown
    /******************************************************************/
    if (count_scan != lastCountScan && millis() - lastMessageSent >= messageCooldown) {
      if (count_scan == 1) {
        bot.sendMessage(CHAT_ID, "Authorized Tag: " + uidString + "\nServo: OPEN (180)", "");
      } else if (count_scan == 2) {
        bot.sendMessage(CHAT_ID, "Authorized Tag: " + uidString + "\nServo: CLOSE (0)", "");
      }
      lastMessageSent = millis();
      lastCountScan = count_scan;
    }
  } else {
    Serial.println("Unauthorized Tag");
    // Prioritize hardware actions
    count_scan = 0;
    servo.write(close_angle);
    digitalWrite(green_led_pin, LOW);
    digitalWrite(red_led_pin, HIGH);
    digitalWrite(buzzer_pin, HIGH);
    buzzerActive = true;
    buzzerStartTime = millis();
    Serial.println("Servo : CLOSE (0)");

    /******************************************************************/
    // Send Telegram message after cooldown
    /******************************************************************/
    if (!lastUnauthorizedSent && millis() - lastMessageSent >= messageCooldown) {
      bot.sendMessage(CHAT_ID, "Unauthorized Tag Scanned: " + uidString, "");
      lastMessageSent = millis();
      lastUnauthorizedSent = true;
    }
    if (count_scan != lastCountScan && millis() - lastMessageSent >= messageCooldown) {
      bot.sendMessage(CHAT_ID, "Servo: CLOSE (0)", "");
      lastMessageSent = millis();
      lastCountScan = count_scan;
    }
  }

  /******************************************************************/
  // Clean up
  /******************************************************************/
  rfid.PICC_HaltA();       // Halt PICC
  rfid.PCD_StopCrypto1();  // Stop encryption
  lastRFIDScan = millis();
  resetRFID();  // Reinitialize RFID module
}

/*#######################################################*/
/* SETUP */
/*#######################################################*/
void setup() {
  Serial.begin(115200);
  SPI.begin();                               // Initialize SPI bus
  rfid.PCD_Init();                           // Initialize MFRC522
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);  // Maximize antenna gain

  // Initialize Wi-Fi
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
  // Update servo and LEDs based on current state
  /******************************************************************/
  if (count_scan == 1) {
    servo.write(open_angle);
    digitalWrite(green_led_pin, HIGH);
    digitalWrite(red_led_pin, LOW);
    Serial.println("Servo : OPEN (180)");
  } else if (count_scan == 2 || count_scan == 0) {
    servo.write(close_angle);
    digitalWrite(green_led_pin, LOW);
    digitalWrite(red_led_pin, LOW);
    Serial.println("Servo : CLOSE (0)");
    if (count_scan == 2) {
      count_scan = 0;  // Reset after close
    }
  }

  /******************************************************************/
  // Handle buzzer and red LED timeout for unauthorized tag
  /******************************************************************/
  if (buzzerActive && (millis() - buzzerStartTime >= buzzerDuration)) {
    digitalWrite(buzzer_pin, LOW);
    digitalWrite(red_led_pin, LOW);
    buzzerActive = false;  // Reset buzzer state
  }

  /******************************************************************/
  // Check for Telegram messages
  /******************************************************************/
  if (millis() - lastBotCheck > botCheckInterval) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      for (int i = 0; i < numNewMessages; i++) {
        String chat_id = String(bot.messages[i].chat_id);
        if (chat_id != CHAT_ID) {
          if (millis() - lastMessageSent >= messageCooldown) {
            bot.sendMessage(chat_id, "Unauthorized user", "");
            lastMessageSent = millis();
          }
          continue;
        }
        String text = bot.messages[i].text;
        Serial.println("Received Telegram message: " + text);
        if (text == "/start" && millis() - lastMessageSent >= messageCooldown) {
          String welcome = "Welcome! System monitors RFID tags and controls servo.\n";
          welcome += "Status updates sent automatically.";
          bot.sendMessage(chat_id, welcome, "");
          lastMessageSent = millis();
        }
      }
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastBotCheck = millis();
  }
}