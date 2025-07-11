#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

HardwareSerial voice(2);  // UART2: GPIO17 (TX) and GPIO16 (RX)
SoftwareSerial mp3(22, 23); // RX. TX VC-02 AI-Thinker Module

DFRobotDFPlayerMini myDFPlayer;
//1: call
//2: drink
//3: eat
//4: help
//5: medicine
//6: okay
//7: sleep
//8: thanks
//9: toilet
//10: wake

char auth[] = "KS6ziCk7gh95b1d699xDnv8MdTh8pf6K";
char ssid[] = "abcd";
char pass[] = "123456789";
char server[] = "prakitblog.com";
int port = 8181;

const int led_pin = 2;
int ledState; // Variable to store the LED state (0 = OFF, 1 = ON)
int hand_signal = 0;
int last_hand_signal = 0;

//AI-Thinker incoming data
unsigned int receivedValue = 0;
unsigned int lastReceivedValue = 0;
unsigned long lastTriggerTime = 0;
const unsigned long cooldownDuration = 2000; // 2 seconds cooldown

//
/*BLYNK_WRITE(V5) {
  ledState = param.asInt(); // Get the value of V0 (0 = OFF, 1 = ON)
  Serial.print("Received V5 state: ");
  Serial.println(ledState);

  if (ledState == 99) {
    myDFPlayer.play(12);
    digitalWrite(led_pin, HIGH);
  }
  else if (ledState == 10) 
  {
    digitalWrite(led_pin, LOW);
  }
}*/

void blinking() {
  digitalWrite(led_pin, HIGH);
  delay(100);
  digitalWrite(led_pin, LOW);
}

BLYNK_WRITE(V6) {
  hand_signal = param.asInt(); // Get the value of hand signal
  Serial.print("Received hand state: ");
  Serial.println(hand_signal);

  // Only trigger if hand signal changed
  if (hand_signal != last_hand_signal) {
    last_hand_signal = hand_signal; // update the lock
    delay(1000);  // optional debounce or processing delay

    switch (hand_signal) {
      case 1:
        Serial.println("Receive Signal: Call");
        Blynk.virtualWrite(V3, "Call");
        myDFPlayer.play(1);
        blinking();
        break;
      case 2:
        Serial.println("Receive Signal: Drink");
        Blynk.virtualWrite(V3, "Drink");
        myDFPlayer.play(2);
        blinking();
        break;
      case 3:
        Serial.println("Receive Signal: Eat");
        Blynk.virtualWrite(V3, "Eat");
        myDFPlayer.play(3);
        blinking();
        break;
      case 4:
        Serial.println("Receive Signal: Help");
        Blynk.virtualWrite(V3, "Help");
        myDFPlayer.play(4);
        blinking();
        break;
      case 5:
        Serial.println("Receive Signal: Medicine");
        Blynk.virtualWrite(V3, "Medicine");
        myDFPlayer.play(5);
        blinking();
        break;
      case 6:
        Serial.println("Receive Signal: Okay");
        Blynk.virtualWrite(V3, "Okay");
        myDFPlayer.play(6);
        blinking();
        break;
      case 7:
        Serial.println("Receive Signal: Sleep");
        Blynk.virtualWrite(V3, "Sleep");
        myDFPlayer.play(7);
        blinking();
        break;
      case 8:
        Serial.println("Receive Signal: Thanks");
        Blynk.virtualWrite(V3, "Thanks");
        myDFPlayer.play(8);
        blinking();
        break;
      case 9:
        Serial.println("Receive Signal: Toilet");
        Blynk.virtualWrite(V3, "Toilet");
        myDFPlayer.play(9);
        blinking();
        break;
      case 10:
        Serial.println("Receive Signal: Wake");
        Blynk.virtualWrite(V3, "Wake");
        myDFPlayer.play(10);
        blinking();
        break;
      default:
        Serial.println("Unknown hand signal");
        break;
    }
  }
}

//
void read_incomingData() {
  if (voice.available() >= 2) {
    byte highByte = voice.read();
    byte lowByte = voice.read();
    receivedValue = (highByte << 8) | lowByte;
    // Only process if:
    // - received value is different, OR
    // - same value but cooldown passed
    if (receivedValue != lastReceivedValue || (millis() - lastTriggerTime >= cooldownDuration)) {
      lastReceivedValue = receivedValue;
      lastTriggerTime = millis();
      Serial.print("Received HEX value: 0x");
      Serial.println(receivedValue, HEX);

      switch (receivedValue) {
        case 0xAA01:
          delay(3000);
          Blynk.virtualWrite(V2, "Call");
          Serial.println("Call");
          myDFPlayer.play(1);
          blinking();
          break;
        case 0xAA02:
          delay(3000);
          Blynk.virtualWrite(V2, "Drink");
          Serial.println("Drink");
          myDFPlayer.play(2);
          blinking();
          break;
        case 0xAA03:
          delay(3000);
          Blynk.virtualWrite(V2, "Eat");
          Serial.println("Eat");
          myDFPlayer.play(3);
          blinking();
          break;
        case 0xAA04:
          delay(3000);
          Blynk.virtualWrite(V2, "Help");
          Serial.println("Help");
          myDFPlayer.play(4);
          blinking();
          break;
        case 0xAA05:
          delay(3000);
          Blynk.virtualWrite(V2, "Medicine");
          Serial.println("Medicine");
          myDFPlayer.play(5);
          blinking();
          break;
        case 0xAA06:
          delay(3000);
          Blynk.virtualWrite(V2, "Okay");
          Serial.println("Okay");
          myDFPlayer.play(6);
          blinking();
          break;
        case 0xAA07:
          delay(3000);
          Blynk.virtualWrite(V2, "Sleep");
          Serial.println("Sleep");
          myDFPlayer.play(7);
          blinking();
          break;
        case 0xAA08:
          delay(3000);
          Blynk.virtualWrite(V2, "Thanks");
          Serial.println("Thanks");
          myDFPlayer.play(8);
          blinking();
          break;
        case 0xAA09:
          delay(3000);
          Blynk.virtualWrite(V2, "Toilet");
          Serial.println("Toilet");
          myDFPlayer.play(9);
          blinking();
          break;
        case 0xAA10:
          delay(3000);
          Blynk.virtualWrite(V2, "Wake");
          Serial.println("Wake");
          myDFPlayer.play(10);
          blinking();
          break;
        default:
          Serial.println("Unknown command");
          break;
      }
    }
  }
}

//
void connectToBlynk() {
  unsigned long start = millis();
  while (!Blynk.connected() && millis() - start < 10000) { // 10 seconds timeout
    Blynk.run();
    delay(1000);
  }
  if (Blynk.connected()) {
    Serial.println("Blynk Connected!");
  } else {
    Serial.println("Blynk connection failed!");
  }
}

void setup() {
  Serial.begin(115200);
  voice.begin(9600, SERIAL_8N1, 16, 17);  // RX, TX
  mp3.begin(9600);
  Blynk.begin(auth, ssid, pass, server, port);
  pinMode(led_pin, OUTPUT);
  Serial.println("Initializing DFPlayer...");
  if (!myDFPlayer.begin(mp3)) {
    Serial.println("Unable to begin DFPlayer Mini:");
    Serial.println("1. Check wiring");
    Serial.println("2. Insert SD card with MP3 files");
    while (true);
  }
  Serial.println("DFPlayer Mini online.");
  myDFPlayer.volume(30);  // Volume: 0–30
  myDFPlayer.play(14);
}

void loop() {
  if (!Blynk.connected()) {
    Serial.println("Blynk disconnected! Reconnecting...");
    connectToBlynk();
  }
  Blynk.run();
  read_incomingData();
  delay(10);
}