#define BLYNK_PRINT Serial

#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <BlynkSimpleEsp32.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create SSD1306 display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define MOD 27    // Hijau
#define NEXT 26   //Kuning
#define HOLD 25   //Merah


#define TRIG_PIN 32
#define ECHO_PIN 33

#define EEPROM_SIZE 12

char auth[] = "RfX6B_kVE8_zhTCCwblgNW2rbDEzb4ry";
char ssid[] = "abcd";
char pass[] = "123456789";

char server[] = "prakitblog.com";
int port = 8181;

const int potPin = 35;
const int buzzerPin = 2;

int modMenu = 0, holdMenu = 0, nextMenu = 0;
int flagMod = 0, flagHold = 0, flagNext = 0;

int waterLevel, percentage;
float LBS, Litre;

int setTankHeight;
float setLBS, setLitre;

float distance, lastValidDistance;

int smoothWaterLevel = 0;
float smoothLBS = 0;
float smoothLitre = 0;
float alpha = 0.1;  // Smoothing factor (lower = smoother)

float readSmoothPot(int pin) {
  return analogRead(pin) / 4095.0;  // Normalize ADC value (0 to 1)
}

void updateValues() {
  float potValue = readSmoothPot(potPin);

  if (flagNext == 1) {
    smoothWaterLevel = (alpha * (potValue * 1000)) + ((1 - alpha) * smoothWaterLevel);
    setTankHeight = smoothWaterLevel;
  } else if (flagNext == 2) {
    smoothLBS = (alpha * (potValue * 20)) + ((1 - alpha) * smoothLBS);
    setLBS = smoothLBS;
  } else if (flagNext == 3) {
    smoothLitre = (alpha * (potValue * 20)) + ((1 - alpha) * smoothLitre);
    setLitre = smoothLitre;
  }
}

void ultrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 40000);
  distance = (duration * 0.0343) / 2;

  if (distance <= 0 || distance > setTankHeight) {
    distance = lastValidDistance;
  } else {
    lastValidDistance = distance;
  }

  // Blind area adjustment
  if (distance <= 20) {
    waterLevel = setTankHeight;  // Assume the tank is full
  } else {
    waterLevel = setTankHeight - distance;  // Adjusted water level
  }

  // Ensure water level is within valid range
  if (waterLevel > setTankHeight) waterLevel = setTankHeight;
  if (waterLevel < 0) waterLevel = 0;

  // Calculate LBS and Litre based on water level percentage
  LBS = ((float)waterLevel / setTankHeight) * setLBS;
  Litre = ((float)waterLevel / setTankHeight) * setLitre;
  percentage = ((float)waterLevel / setTankHeight) * 100;
}

void modButton() {
  if (digitalRead(MOD) == LOW) {
    delay(100);
    modMenu = (modMenu + 1) % 2;
    sound();
    while (digitalRead(MOD) == LOW);
  }

  if (modMenu == 1) {
    flagMod = 1;
  } else if (modMenu == 0) {
    flagMod = 0;
  }
}

void holdButton() {
  if (digitalRead(HOLD) == LOW) {
    delay(100);
    holdMenu = (holdMenu + 1) % 2;
    sound();
    while (digitalRead(HOLD) == LOW)
      ;
  }

  if (holdMenu == 1) {
    flagHold = 1;
  } else if (holdMenu == 0) {
    flagHold = 0;
  }
}

void nextButton() {
  if (digitalRead(NEXT) == LOW) {
    delay(100);
    nextMenu = (nextMenu + 1) % 4;
    saveToEEPROM();
    sound();
    while (digitalRead(NEXT) == LOW)
      ;
  }

  if (nextMenu == 1) {
    flagNext = 1;
  } else if (nextMenu == 2) {
    flagNext = 2;
  } else if (nextMenu == 3) {
    flagNext = 3;
  } else if (nextMenu == 0) {
    flagNext = 0;
  }
}

void sound() {
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
}

void saveToEEPROM() {
  EEPROM.put(0, setLBS);
  EEPROM.put(4, setLitre);
  EEPROM.put(8, setTankHeight);
  EEPROM.commit();
}

void loadFromEEPROM() {
  EEPROM.get(0, setLBS);
  EEPROM.get(4, setLitre);
  EEPROM.get(8, setTankHeight);
}

void connectToBlynk() {
  unsigned long start = millis();
  while (!Blynk.connected() != WL_CONNECTED && millis() - start < 10000) {  // Try for 10 seconds
    Blynk.run();                                                            // Attempt connection
    delay(1000);
  }

  if (Blynk.connected()) {
    Serial.println("Connected!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("WiFi Connected");
    display.display();

    pinMode(buzzerPin, OUTPUT);

    for (int i = 0; i < 2; i++) {
      sound();
    }
  } else {
    Serial.println("Blynk connection failed");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("WiFi Failed");
    display.display();
  }
}

void setup() {
  Serial.begin(115200);

  /*WiFi.disconnect(true, true);
  delay(1000);*/

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (1)
      ;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("WiFi Connecting...");
  display.display();

  Blynk.begin(auth, ssid, pass, server, port);

  connectToBlynk();

  delay(1000);

  // Initialize EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM");
    while (1)
      ;
  }

  pinMode(MOD, INPUT_PULLUP);
  pinMode(HOLD, INPUT_PULLUP);
  pinMode(NEXT, INPUT_PULLUP);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  loadFromEEPROM();

  display.clearDisplay();
}

void loop() {
  if (!Blynk.connected()) {
    Serial.println("Blynk disconnected! Reconnecting...");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("WiFi Connecting...");
    display.display();
    connectToBlynk();
  }
  Blynk.run();

  modButton();
  holdButton();
  nextButton();

  updateValues();

  display.clearDisplay();

  if (flagHold == 1) {
    ultrasonic();
  } else if (flagHold == 0) {
    holdMenu = 0;
  }

  //  Main display
  if (modMenu == 0) {
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.print("Water Level: ");
    display.print(waterLevel);
    display.print(" cm ");

    display.setCursor(0, 16);
    display.print("LBS: ");
    display.print(LBS);

    display.setCursor(0, 32);
    display.print("Litre: ");
    display.print(Litre);
    display.print(" L ");

    display.setCursor(0, 48);
    display.print("Percentage: ");
    display.print(percentage);
    display.print(" % ");
  }

  //  Set Mode display
  if (modMenu == 1) {
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.print("Set Tank: ");
    display.print(setTankHeight);
    display.print(" cm ");

    display.setCursor(0, 16);
    display.print("Set LBS: ");
    display.print(setLBS);

    display.setCursor(0, 32);
    display.print("Set Litre: ");
    display.print(setLitre);
    display.print(" L ");
  }

  display.display();

  Blynk.virtualWrite(V1, waterLevel);
  Blynk.virtualWrite(V2, LBS);
  Blynk.virtualWrite(V3, Litre);
  Blynk.virtualWrite(V4, percentage);

  Serial.print("MOD: ");
  Serial.print(modMenu);
  Serial.print(" | HOLD: ");
  Serial.print(holdMenu);
  Serial.print(" | NEXT: ");
  Serial.println(nextMenu);
  Serial.println("");

  Serial.print("Water Level: ");
  Serial.print(waterLevel);
  Serial.print(" cm");
  Serial.print("  | LBS: ");
  Serial.print(LBS);
  Serial.print("  | Litre: ");
  Serial.print(Litre);
  Serial.print("  | Percentage: ");
  Serial.println(percentage);
  Serial.println("");

  Serial.print("Set Water: ");
  Serial.print(setTankHeight);
  Serial.print("  | Set LBS: ");
  Serial.print(setLBS);
  Serial.print("  | Set Litre: ");
  Serial.println(setLitre);
  Serial.println("");

  Serial.print("Distance: ");
  Serial.println(distance);
  Serial.println("");

  delay(100);
}
