#include "Arduino.h"
#include "LoRa_E32.h"

// LoRa
LoRa_E32 e32ttl(2, 3);

void printParameters(struct Configuration configuration);
void printModuleInformation(struct ModuleInformation moduleInformation);

// Float sensor
const int float_level_sensor_pin = 7;
int last_float_state = HIGH;
int last_sent_state = HIGH;  // Track last sent state

// Led, buzzer
const int led_pin = 13;
const int buzzer_pin = 12;
 
// LED blinking variables
unsigned long previousMillis = 0;
const long blinkInterval = 100;  // 100ms blink interval
bool ledState = false;

// Send delay variables
unsigned long lastSendMillis = 0;
const long sendDelay = 2000; // 2-second delay between sends
const long timeSlotOffset = 0; // Flood A: 0ms offset (first 2s of 4s cycle)

char message[3];                    
char last_processed_message[3] = "";
int lock_status = 0;

void handle_coms() {
  if (e32ttl.available() > 0) {
    ResponseContainer rs = e32ttl.receiveMessage();
    if (rs.status.code == 1) { // Success
      strncpy(message, rs.data.c_str(), sizeof(message) - 1);
      message[sizeof(message) - 1] = '\0';
      Serial.println(rs.status.getResponseDescription());
      Serial.print("Received message: ");
      Serial.println(message);

      // Handle status request
      if (message[0] == 'S' && strcmp(message, "ST") == 0 && strcmp(message, last_processed_message) != 0) {
        unsigned long currentMillis = millis();
        if (currentMillis % 4000 < 2000 && currentMillis - lastSendMillis >= sendDelay) { // Flood A time slot
          String response = (lock_status == 1) ? "A1" : "A0";
          Serial.print("Sending delayed status response: ");
          Serial.println(response);
          ResponseStatus rs1 = e32ttl.sendFixedMessage(0, 1, 0x04, response);
          Serial.println(rs1.getResponseDescription());
          lastSendMillis = currentMillis;
          // Update last processed message
          strncpy(last_processed_message, message, sizeof(last_processed_message) - 1);
          last_processed_message[sizeof(last_processed_message) - 1] = '\0';
        }
      }
    } else {
      Serial.println("Failed to receive valid message");
    }
    // Clear LoRa buffer
    while (e32ttl.available() > 0) {
      e32ttl.receiveMessage(); // Discard any remaining data
    }
  }
}

// Read float sensor
void read_float() {
  int current_float_state = digitalRead(float_level_sensor_pin);
  unsigned long currentMillis = millis();

  // Handle LED blinking
  if (current_float_state == HIGH && last_sent_state == HIGH) {
    if (currentMillis - previousMillis >= blinkInterval) {
      ledState = !ledState;
      digitalWrite(led_pin, ledState ? HIGH : LOW);
      previousMillis = currentMillis;
    }
  }

  // Check for state transition, send delay, and time slot
  if (current_float_state != last_float_state && currentMillis - lastSendMillis >= sendDelay) {
    if (currentMillis % 4000 < 2000) { // Flood A sends in first 2 seconds of every 4-second cycle
      if (current_float_state == HIGH && last_sent_state != HIGH) {
        Serial.println("FLOAT: HIGH - Liquid Detected");
        if (currentMillis - previousMillis >= blinkInterval) {
          ledState = !ledState;
          digitalWrite(led_pin, ledState ? HIGH : LOW);
          previousMillis = currentMillis;
        }
        digitalWrite(buzzer_pin, HIGH);
        Serial.println("Sending A1 to Receiver");
        ResponseStatus rs1 = e32ttl.sendFixedMessage(0, 1, 0x04, "A1");
        Serial.println(rs1.getResponseDescription());
        lock_status = 1;
        last_sent_state = HIGH;
        lastSendMillis = currentMillis;
      } else if (current_float_state == LOW && last_sent_state != LOW) {
        Serial.println("FLOAT: LOW - No Liquid");
        digitalWrite(led_pin, LOW);
        ledState = false;
        digitalWrite(buzzer_pin, LOW);
        Serial.println("Sending A0 to Receiver");
        ResponseStatus rs1 = e32ttl.sendFixedMessage(0, 1, 0x04, "A0");
        Serial.println(rs1.getResponseDescription());
        lock_status = 0;
        last_sent_state = LOW;
        lastSendMillis = currentMillis;
      }
      last_float_state = current_float_state;
    }
  }
}

void setup() {
  Serial.begin(9600);
  e32ttl.begin();
  ResponseStructContainer c;
  c = e32ttl.getConfiguration();
  Configuration configuration = *(Configuration*)c.data;
  configuration.ADDL = 0x03; // Flood A address
  configuration.ADDH = 0x00;
  configuration.CHAN = 0x04;
  configuration.OPTION.fixedTransmission = FT_FIXED_TRANSMISSION;
  e32ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  printParameters(configuration);

  pinMode(float_level_sensor_pin, INPUT_PULLUP);
  pinMode(led_pin, OUTPUT);
  pinMode(buzzer_pin, OUTPUT);

  Serial.println();
  Serial.println("Start listening!");
}

void loop() {
  handle_coms();
  read_float();
  Serial.print("Lock Status: ");
  Serial.println(lock_status);
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