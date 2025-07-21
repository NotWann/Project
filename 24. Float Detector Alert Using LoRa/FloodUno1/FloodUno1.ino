#include "Arduino.h"
#include "LoRa_E32.h"
#include "bitmap.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

LoRa_E32 e32ttl(10, 11);

const int buzzer_pin = 13;
const unsigned long interval = 1000;
unsigned long previous_millis = 0;
bool buzzer_state = false;
unsigned long last_display_update = 0;
const unsigned long display_interval = 1000;

unsigned long last_ask_duration_a = 0; // Timer for Flood A status requests
unsigned long last_ask_duration_b = 0; // Timer for Flood B status requests
const unsigned long wait_before_ask_duration = 6000; // 6 seconds to accommodate Flood B's 4s delay

int lock_alert_uno1 = 0;                // For Flood A
int lock_alert_uno2 = 0;                // For Flood B
char message[6];                        // Sufficient for "A1", "A0", "B1", "B0" + null terminator
char last_processed_message_a[6] = "";  // Track last processed message for Flood A
char last_processed_message_b[6] = "";  // Track last processed message for Flood B

void printParameters(struct Configuration configuration);
void printModuleInformation(struct ModuleInformation moduleInformation);

void handle_coms() {
  unsigned long current_millis = millis();

  if (e32ttl.available() > 0) {
    ResponseContainer rs = e32ttl.receiveMessage();
    if (rs.status.code == 1) { // Success
      strncpy(message, rs.data.c_str(), sizeof(message) - 1);
      message[sizeof(message) - 1] = '\0';
      Serial.println(rs.status.getResponseDescription());
      Serial.print("Received message: ");
      Serial.println(message);

      // Parse message based on prefix
      if (message[0] == 'A' && strcmp(message, last_processed_message_a) != 0) {  // Flood A
        if (strcmp(message, "A1") == 0) {
          lock_alert_uno1 = 1;
          Serial.println("Flood A: XXXXXXXXXXX");
        } else if (strcmp(message, "A0") == 0) {
          lock_alert_uno1 = 0;
          Serial.println("Flood A: YYYYYYYYYYYY");
          buzzer_state = false;
          digitalWrite(buzzer_pin, LOW);
        }
        strncpy(last_processed_message_a, message, sizeof(last_processed_message_a) - 1);
        last_processed_message_a[sizeof(last_processed_message_a) - 1] = '\0';
      } else if (message[0] == 'B' && strcmp(message, last_processed_message_b) != 0) {  // Flood B
        if (strcmp(message, "B1") == 0) {
          lock_alert_uno2 = 1;
          Serial.println("Flood B: XXXXXXXXXXX");
        } else if (strcmp(message, "B0") == 0) {
          lock_alert_uno2 = 0;
          Serial.println("Flood B: YYYYYYYYYYYY");
          buzzer_state = false;
          digitalWrite(buzzer_pin, LOW);
        }
        strncpy(last_processed_message_b, message, sizeof(last_processed_message_b) - 1);
        last_processed_message_b[sizeof(last_processed_message_b) - 1] = '\0';
      }
    } else {
      Serial.println("Failed to receive valid message");
    }
    // Clear LoRa buffer
    while (e32ttl.available() > 0) {
      e32ttl.receiveMessage(); // Discard any remaining data
    }
  }

  // Send status request to Flood A every 6 seconds if in alert state
  if (lock_alert_uno1 == 1 && current_millis - last_ask_duration_a >= wait_before_ask_duration) {
    Serial.println("Asking Status: Sending ST to Flood A");
    ResponseStatus rs1 = e32ttl.sendFixedMessage(0, 3, 0x04, "ST");
    Serial.println(rs1.getResponseDescription());
    last_ask_duration_a = current_millis;
  }

  // Send status request to Flood B every 6 seconds if in alert state
  if (lock_alert_uno2 == 1 && current_millis - last_ask_duration_b >= wait_before_ask_duration) {
    Serial.println("Asking Status: Sending ST to Flood B");
    ResponseStatus rs2 = e32ttl.sendFixedMessage(0, 5, 0x04, "ST");
    Serial.println(rs2.getResponseDescription());
    last_ask_duration_b = current_millis;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(buzzer_pin, OUTPUT);
  digitalWrite(buzzer_pin, LOW);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  Serial.println("OLED initialized successfully");
  e32ttl.begin();
  ResponseStructContainer c;
  c = e32ttl.getConfiguration();
  Configuration configuration = *(Configuration*)c.data;
  configuration.ADDL = 0x01;  // Receiver address
  configuration.ADDH = 0x00;
  configuration.CHAN = 0x04;
  configuration.OPTION.fixedTransmission = FT_FIXED_TRANSMISSION;
  e32ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  printParameters(configuration);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  start_up_menu();
}

void loop() {
  unsigned long current_millis = millis();

  handle_coms();
  Serial.print("Message: ");
  Serial.println(message);

  if (current_millis - last_display_update >= display_interval) {
    display_status_menu();
    last_display_update = current_millis;
  }
}

void start_up_menu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 17);
  display.print("Loading");
  display.drawBitmap(31, 29, loading_icon, 11, 16, 1);
  display.drawBitmap(66, 0, dolphin_loading_icon, 51, 64, 1);
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(23, 4);
  display.print("Flood Detector");
  display.setCursor(17, 15);
  display.print("With LoRa System");
  display.drawBitmap(46, 25, chip_icon, 37, 36, 1);
  display.display();
  delay(2000);
}

void display_status_menu() {
  display.clearDisplay();
  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(47, 4);
  display.print("STATUS");
  display.drawLine(47, 12, 81, 12, 1);
  display.setCursor(3, 18);
  display.print("Station A:");
  if (lock_alert_uno1 == 1) {
    unsigned long current_millis = millis();
    if (current_millis - previous_millis >= interval) {
      buzzer_state = !buzzer_state;
      digitalWrite(buzzer_pin, buzzer_state ? HIGH : LOW);
      previous_millis = current_millis;
    }
    if (buzzer_state == HIGH) {
      display.drawBitmap(62, 18, black_alert_icon, 29, 7, 1);
      display.drawBitmap(98, 18, on_speaker_icon, 20, 16, 1);
      display.setCursor(20, 47);
      display.print("Flood Detected!");
      display.drawBitmap(6, 43, water_icon, 11, 16, 1);
      display.drawBitmap(109, 43, water_icon, 11, 16, 1);
    } else {
      display.drawBitmap(62, 18, white_alert_icon, 29, 7, 1);
      display.drawBitmap(98, 18, off_speaker_icon, 20, 16, 1);
      display.drawBitmap(62, 43, warning_icon, 16, 14, 1);
    }
  } else {
    display.setCursor(62, 18);
    display.print("NORMAL");
  }

  display.setCursor(3, 27);
  display.print("Station B:");
  if (lock_alert_uno2 == 1) {
    unsigned long current_millis = millis();
    if (current_millis - previous_millis >= interval) {
      buzzer_state = !buzzer_state;
      digitalWrite(buzzer_pin, buzzer_state ? HIGH : LOW);
      previous_millis = current_millis;
    }
    if (buzzer_state == HIGH) {
      display.drawBitmap(62, 27, black_alert_icon, 29, 7, 1);
      display.drawBitmap(98, 18, on_speaker_icon, 20, 16, 1);
      display.setCursor(20, 47);
      display.print("Flood Detected!");
      display.drawBitmap(6, 43, water_icon, 11, 16, 1);
      display.drawBitmap(109, 43, water_icon, 11, 16, 1);
    } else {
      display.drawBitmap(62, 27, white_alert_icon, 29, 7, 1);
      display.drawBitmap(98, 18, off_speaker_icon, 20, 16, 1);
      display.drawBitmap(62, 43, warning_icon, 16, 14, 1);
    }
  } else {
    display.setCursor(62, 27);
    display.print("NORMAL");
  }
  display.display();
}

/*----Print AS32 Detail---*/
void printParameters(struct Configuration configuration) {
  Serial.println("----------------------------------------");
  Serial.print(F("HEAD : "));
  Serial.print(configuration.HEAD, BIN);
  Serial.print(" ");
  Serial.print(configuration.HEAD, DEC);
  Serial.print(" ");
  Serial.println(configuration.HEAD, HEX);
  Serial.println(F(" "));
  Serial.print(F("AddH : "));
  Serial.print(configuration.ADDH, DEC);
  Serial.println();
  Serial.print(F("AddL : "));
  Serial.print(configuration.ADDL, DEC);
  Serial.println();
  Serial.print(F("Chan : "));
  Serial.print(configuration.CHAN, DEC);
  Serial.print(" -> ");
  Serial.println(configuration.getChannelDescription());
  Serial.println(F(" "));
  Serial.print(F("SpeedParityBit     : "));
  Serial.print(configuration.SPED.uartParity, BIN);
  Serial.print(" -> ");
  Serial.println(configuration.SPED.getUARTParityDescription());
  Serial.print(F("SpeedUARTDatte  : "));
  Serial.print(configuration.SPED.uartBaudRate, BIN);
  Serial.print(" -> ");
  Serial.println(configuration.SPED.getUARTBaudRate());
  Serial.print(F("SpeedAirDataRate   : "));
  Serial.print(configuration.SPED.airDataRate, BIN);
  Serial.print(" -> ");
  Serial.println(configuration.SPED.getAirDataRate());
  Serial.print(F("OptionTrans        : "));
  Serial.print(configuration.OPTION.fixedTransmission, BIN);
  Serial.print(" -> ");
  Serial.println(configuration.OPTION.getFixedTransmissionDescription());
  Serial.print(F("OptionPullup       : "));
  Serial.print(configuration.OPTION.ioDriveMode, BIN);
  Serial.print(" -> ");
  Serial.println(configuration.OPTION.getIODroveModeDescription());
  Serial.print(F("OptionWakeup       : "));
  Serial.print(configuration.OPTION.wirelessWakeupTime, BIN);
  Serial.print(" -> ");
  Serial.println(configuration.OPTION.getWirelessWakeUPTimeDescription());
  Serial.print(F("OptionFEC          : "));
  Serial.print(configuration.OPTION.fec, BIN);
  Serial.print(" -> ");
  Serial.println(configuration.OPTION.getFECDescription());
  Serial.print(F("OptionPower        : "));
  Serial.print(configuration.OPTION.transmissionPower, BIN);
  Serial.print(" -> ");
  Serial.println(configuration.OPTION.getTransmissionPowerDescription());
  Serial.println("----------------------------------------");
}

void printModuleInformation(struct ModuleInformation moduleInformation) {
  Serial.println("----------------------------------------");
  Serial.print(F("HEAD BIN: "));
  Serial.print(moduleInformation.HEAD, BIN);
  Serial.print(" ");
  Serial.print(moduleInformation.HEAD, DEC);
  Serial.print(" ");
  Serial.println(moduleInformation.HEAD, HEX);
  Serial.print(F("Freq.: "));
  Serial.println(moduleInformation.frequency, HEX);
  Serial.print(F("Version  : "));
  Serial.println(moduleInformation.version, HEX);
  Serial.print(F("Features : "));
  Serial.println(moduleInformation.features, HEX);
  Serial.println("----------------------------------------");
}