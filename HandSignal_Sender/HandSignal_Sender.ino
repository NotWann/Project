#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char auth[] = "KS6ziCk7gh95b1d699xDnv8MdTh8pf6K";
char ssid[] = "abcd";
char pass[] = "123456789";
char server[] = "prakitblog.com";
int port = 8181;

//const int push_button = 25;
//int lastbutton_state = HIGH;

const int led_pin = 2;

//flex sensor pin
const int flex[5] = {33, 32, 35, 34, 39};
int flexValue[5];

int hand_signal = 0;
int detected_signal = 0;
int last_detected_signal = 0;
unsigned long signalStartTime = 0;
const unsigned long stableTime = 1000;  // 1.5 seconds to confirm gesture

WidgetBridge bridge1(V1);
WidgetBridge bridge2(V10);

BLYNK_CONNECTED() {
  bridge1.setAuthToken("KS6ziCk7gh95b1d699xDnv8MdTh8pf6K");
  bridge2.setAuthToken("KS6ziCk7gh95b1d699xDnv8MdTh8pf6K"); 
}

void blinking() {
  digitalWrite(led_pin, HIGH);
  delay(100);
  digitalWrite(led_pin, LOW);
}

/*void read_button() {
  int currentbutton_state = digitalRead(push_button);
  if (currentbutton_state == LOW && lastbutton_state == HIGH) {
    bridge1.virtualWrite(V5, "99");
    Serial.println("SENT 99");
  } else if (currentbutton_state != lastbutton_state) {
    bridge1.virtualWrite(V5, "10");
    Serial.println("SENT 10");
  }
  lastbutton_state = currentbutton_state;
}*/

void read_flex() {
  flexValue[0] = analogRead(flex[0]);
  flexValue[1] = analogRead(flex[1]);
  flexValue[2] = analogRead(flex[2]);
  flexValue[3] = analogRead(flex[3]);
  flexValue[4] = analogRead(flex[4]);

  Serial.print(flexValue[4]); Serial.print(" | "); 
  Serial.print(flexValue[3]); Serial.print(" | "); 
  Serial.print(flexValue[2]); Serial.print(" | ");
  Serial.print(flexValue[1]); Serial.print(" | ");
  Serial.print(flexValue[0]); Serial.println(" ");
  
  delay(100); // Delay for readability
}

void process_signal() {
  // Detect signal based on flex pattern
  if (flexValue[0] > 1500 && flexValue[1] < 100 && flexValue[2] < 100 && flexValue[3] < 100 && flexValue[4] < 100) detected_signal = 1;
  else if (flexValue[0] > 1500 && flexValue[1] > 1500 && flexValue[2] < 100 && flexValue[3] < 100 && flexValue[4] < 100) detected_signal = 2;
  else if (flexValue[0] > 1500 && flexValue[1] > 1500 && flexValue[2] > 1500 && flexValue[3] < 100 && flexValue[4] < 100) detected_signal = 3;
  else if (flexValue[0] > 1500 && flexValue[1] > 1500 && flexValue[2] > 1500 && flexValue[3] > 1500 && flexValue[4] < 100) detected_signal = 4;
  else if (flexValue[0] > 1500 && flexValue[1] > 1500 && flexValue[2] > 1500 && flexValue[3] > 1500 && flexValue[4] > 1500) detected_signal = 5;
  else if (flexValue[0] < 100 && flexValue[1] > 1500 && flexValue[2] > 1500 && flexValue[3] > 1500 && flexValue[4] > 1500) detected_signal = 6;
  else if (flexValue[0] < 100 && flexValue[1] < 100 && flexValue[2] > 1500 && flexValue[3] > 1500 && flexValue[4] > 1500) detected_signal = 7;
  else if (flexValue[0] < 100 && flexValue[1] < 100 && flexValue[2] < 100 && flexValue[3] > 1500 && flexValue[4] > 1500) detected_signal = 8;
  else if (flexValue[0] > 1500 && flexValue[1] < 100 && flexValue[2] > 1500 && flexValue[3] < 100 && flexValue[4] > 1500) detected_signal = 9;
  else if (flexValue[0] < 100 && flexValue[1] < 100 && flexValue[2] > 1500 && flexValue[3] > 1500 && flexValue[4] < 100) detected_signal = 10;
  else detected_signal = 0;

  // Check if signal stays stable long enough
  if (detected_signal != last_detected_signal) {
    // Gesture changed — reset timer
    last_detected_signal = detected_signal;
    signalStartTime = millis();
  }

  // Confirm signal only after holding for stableTime duration
  if ((millis() - signalStartTime > stableTime) && detected_signal != hand_signal) {
    hand_signal = detected_signal; // Final confirmed signal
    bridge2.virtualWrite(V6, String(hand_signal));
    blinking();
    Serial.print("Confirmed Signal: ");
    Serial.println(hand_signal);
  }
}

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
  Blynk.begin(auth, ssid, pass, server, port);
  analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)v
  //pinMode(push_button, INPUT_PULLUP);
  pinMode(led_pin, OUTPUT);
}

void loop() {
  if (!Blynk.connected()) {
    Serial.println("Blynk disconnected! Reconnecting...");
    connectToBlynk();
  }
  Blynk.run();
  //read_button();
  read_flex();
  process_signal();
  delay(10);
}