#define BLYNK_PRINT Serial

#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <BlynkSimpleEsp32.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Preferences preferences;

#define MOD 27    // Green
#define NEXT 26   // Yellow
#define HOLD 25   // Red
#define CLK 34    // Encoder CLK
#define DT 35     // Encoder DT
#define SW 4      // Encoder Switch
#define TRIG_PIN 32
#define ECHO_PIN 33

char auth[] = "RfX6B_kVE8_zhTCCwblgNW2rbDEzb4ry";
char ssid[] = "abcd";
char pass[] = "123456789";
char server[] = "prakitblog.com";
int port = 8181;

const int buzzerPin = 2;

int counter = 0;
int currentStateCLK;
int lastStateCLK = 0;
unsigned long lastButtonPress = 0;
int lockPoint;

int modMenu = 0, holdMenu = 0, nextMenu = 0;
int flagMod = 0, flagHold = 0, flagNext = 0;

// Tank A data
float weightDataA[] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000};
float heightDataA[] = {0, 7.3, 14.5, 21.1, 27.2, 32.5, 37.4, 42.4, 47.5, 52.8, 58, 62.9, 68.3, 73.5, 79.2, 85, 91.3, 97.3, 101.3};
int dataSizeA = sizeof(weightDataA) / sizeof(weightDataA[0]);

// Tank B data
float weightDataB[] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 8700, 9000};
float heightDataB[] = {0, 3.8, 8.2, 11.5, 14, 16.3, 18.5, 20.7, 23, 25.2, 27.4, 29.8, 32, 34.6, 36.9, 39.5, 42.3, 45.5, 47.1, 50.2};
int dataSizeB = sizeof(weightDataB) / sizeof(weightDataB[0]);

// Tank C data
float weightDataC[] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 7900, 8000};
float heightDataC[] = {0, 5.6, 10.2, 14.7, 18.8, 23, 26.7, 30.8, 34.9, 38.9, 43, 47.1, 51.8, 55.5, 59.6, 63.8, 67.8, 70.2};
int dataSizeC = sizeof(weightDataC) / sizeof(weightDataC[0]);

float estimatedWeightA, estimatedHeightA;
float estimatedWeightB, estimatedHeightB;
float estimatedWeightC, estimatedHeightC;
float estimatedLitre[3] = {0, 0, 0};

int tank_HeightRead[3] = {0, 0, 0};
float tank_LbsRead[3] = {0, 0, 0};
float tank_LitreRead[3] = {0, 0, 0};

int set_Height[3] = {0, 0, 0};
float set_Lbs[3] = {0, 0, 0};
float set_Litre[3] = {0, 0, 0};

float distance[3] = {0, 0, 0}; // Store distance for each tank
float lastValidDistance[3] = {0, 0, 0};
int heightTank[3] = {0, 0, 0};
int percentageTank[3] = {0, 0, 0};
float lbsTank[3] = {0, 0, 0};
float litreTank[3] = {0, 0, 0};
float percentage[3] = {0, 0, 0};

void saveToNVS() {
  preferences.begin("tankSet", false);
  preferences.putFloat("heightA", tank_HeightRead[0]);
  preferences.putFloat("lbsA", tank_LbsRead[0]);
  preferences.putFloat("litreA", tank_LitreRead[0]);
  preferences.putFloat("heightB", tank_HeightRead[1]);
  preferences.putFloat("lbsB", tank_LbsRead[1]);
  preferences.putFloat("litreB", tank_LitreRead[1]);
  preferences.putFloat("heightC", tank_HeightRead[2]);
  preferences.putFloat("lbsC", tank_LbsRead[2]);
  preferences.putFloat("litreC", tank_LitreRead[2]);
  preferences.end();
}

void readFromNVS() {
  preferences.begin("tankSet", true);
  tank_HeightRead[0] = preferences.getFloat("heightA", 0.0);
  tank_LbsRead[0] = preferences.getFloat("lbsA", 0.0);
  tank_LitreRead[0] = preferences.getFloat("litreA", 0.0);
  tank_HeightRead[1] = preferences.getFloat("heightB", 0.0);
  tank_LbsRead[1] = preferences.getFloat("lbsB", 0.0);
  tank_LitreRead[1] = preferences.getFloat("litreB", 0.0);
  tank_HeightRead[2] = preferences.getFloat("heightC", 0.0);
  tank_LbsRead[2] = preferences.getFloat("lbsC", 0.0);
  tank_LitreRead[2] = preferences.getFloat("litreC", 0.0);
  preferences.end();
}

void encoder() {
  currentStateCLK = digitalRead(CLK);
  if (currentStateCLK == 1 && lastStateCLK == 0) {
    if (digitalRead(DT) == 1 && lockPoint == 0) counter++;
    else if (digitalRead(DT) == 1 && lockPoint == 1) counter--;
    updateValues();
  }
  lastStateCLK = currentStateCLK;

  int btnState = digitalRead(SW);
  if (btnState == LOW && millis() - lastButtonPress > 50) {
    Serial.println("Button pressed!");
    lockPoint = (lockPoint + 1) % 2;
    lastButtonPress = millis();
  }
  delay(1);
}

void updateValues() {
  if (flagHold == 0 && flagMod == 1) {
    if (flagNext == 1) { set_Height[0] = counter; tank_HeightRead[0] = set_Height[0]; }
    else if (flagNext == 2) { set_Lbs[0] = counter; tank_LbsRead[0] = set_Lbs[0]; }
    else if (flagNext == 3) { set_Litre[0] = counter; tank_LitreRead[0] = set_Litre[0]; }
  } else if (flagHold == 1 && flagMod == 1) {
    if (flagNext == 1) { set_Height[1] = counter; tank_HeightRead[1] = set_Height[1]; }
    else if (flagNext == 2) { set_Lbs[1] = counter; tank_LbsRead[1] = set_Lbs[1]; }
    else if (flagNext == 3) { set_Litre[1] = counter; tank_LitreRead[1] = set_Litre[1]; }
  } else if (flagHold == 2 && flagMod == 1) {
    if (flagNext == 1) { set_Height[2] = counter; tank_HeightRead[2] = set_Height[2]; }
    else if (flagNext == 2) { set_Lbs[2] = counter; tank_LbsRead[2] = set_Lbs[2]; }
    else if (flagNext == 3) { set_Litre[2] = counter; tank_LitreRead[2] = set_Litre[2]; }
  }
}

void ultrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 40000);
  float rawDistance = (duration * 0.0343) / 2; // Distance in cm with decimals

  // Update only the selected tank based on flagHold
  int tankIndex = -1;
  if (flagMod == 1 && flagNext == 0) tankIndex = 0; // Tank A
  else if (flagMod == 1 && flagNext == 1) tankIndex = 1; // Tank B
  else if (flagMod == 1 && flagNext == 2) tankIndex = 2; // Tank C
  else if (flagMod == 1 && flagNext == 3) tankIndex = 0; // Default to Tank A

  if (tankIndex != -1) {
    if (rawDistance <= 0 || rawDistance > tank_HeightRead[tankIndex]) {
      distance[tankIndex] = lastValidDistance[tankIndex];
    } else {
      distance[tankIndex] = rawDistance;
      lastValidDistance[tankIndex] = rawDistance;
    }

    if (distance[tankIndex] <= 20) {
      heightTank[tankIndex] = tank_HeightRead[tankIndex]; // Tank full
    } else {
      heightTank[tankIndex] = tank_HeightRead[tankIndex] - distance[tankIndex]; // Water level
    }

    // Update estimated values for the selected tank
    if (tankIndex == 0) {
      estimatedWeightA = interpolateHeightToWeightA(heightTank[0]);
      estimatedHeightA = interpolateWeightToHeightA(estimatedWeightA);
      estimatedLitre[0] = calculateLitersFromWeight(estimatedWeightA);
      percentage[0] = calculateTankPercentage(heightTank[0], tank_HeightRead[0]);
    } else if (tankIndex == 1) {
      estimatedWeightB = interpolateHeightToWeightB(heightTank[1]);
      estimatedHeightB = interpolateWeightToHeightB(estimatedWeightB);
      estimatedLitre[1] = calculateLitersFromWeight(estimatedWeightB);
      percentage[1] = calculateTankPercentage(heightTank[1], tank_HeightRead[1]);
    } else if (tankIndex == 2) {
      estimatedWeightC = interpolateHeightToWeightC(heightTank[2]);
      estimatedHeightC = interpolateWeightToHeightC(estimatedWeightC);
      estimatedLitre[2] = calculateLitersFromWeight(estimatedWeightC);
      percentage[2] = calculateTankPercentage(heightTank[2], tank_HeightRead[2]);
    } 
  }
    
  // Debug output
  Serial.print("Distance Tank A: "); Serial.println(distance[0], 2);
  Serial.print("Height Tank A: "); Serial.println(heightTank[0], 2);
  Serial.print("Weight Tank A: "); Serial.println(estimatedWeightA, 2);
  Serial.print("Liters Tank A: "); Serial.println(estimatedLitre[0], 2);
  Serial.print("Distance Tank B: "); Serial.println(distance[1], 2);
  Serial.print("Distance Tank C: "); Serial.println(distance[2], 2);
}

void modButton() {
  if (digitalRead(MOD) == LOW) {
    delay(100);
    modMenu = (modMenu + 1) % 2;
    sound();
    while (digitalRead(MOD) == LOW);
    flagMod = modMenu;
  }
}

void holdButton() {
  static unsigned long pressStartTime = 0;
  const unsigned long longPressDuration = 1000;

  if (digitalRead(HOLD) == LOW && pressStartTime == 0) {
    pressStartTime = millis();
    sound();
  }
  if (digitalRead(HOLD) == HIGH && pressStartTime != 0) {
    unsigned long pressDuration = millis() - pressStartTime;
    if (pressDuration < longPressDuration) {
      holdMenu = (holdMenu + 1) % 5;
      Serial.print("Short press, holdMenu: "); Serial.println(holdMenu);
      if (holdMenu == 0) flagHold = 0;
      else if (holdMenu == 1) flagHold = 1;
      else if (holdMenu == 2) flagHold = 2;
      else if (holdMenu == 3) flagHold = 3;
      else if (holdMenu == 4) flagHold = 4;
    } else {
      Serial.println("Long press detected!");
      flagHold = 5;
    }
    pressStartTime = 0;
  }
}

void nextButton() {
  if (digitalRead(NEXT) == LOW) {
    delay(100);
    nextMenu = (nextMenu + 1) % 4;
    sound();
    saveToNVS();
    while (digitalRead(NEXT) == LOW);
    if (nextMenu == 0) flagNext = 0;
    else if (nextMenu == 1) flagNext = 1;
    else if (nextMenu == 2) flagNext = 2;
    else if (nextMenu == 3) flagNext = 3;
  }
}

void sound() {
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
}

void connectToBlynk() {
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("WiFi Connected");
    display.display();
    pinMode(buzzerPin, OUTPUT);
    for (int i = 0; i < 2; i++) sound();
  } else {
    Serial.println("WiFi connection failed");
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
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("WiFi Connecting...");
  display.display();

  Blynk.begin(auth, ssid, pass, server, port);
  connectToBlynk();

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  pinMode(MOD, INPUT_PULLUP);
  pinMode(HOLD, INPUT_PULLUP);
  pinMode(NEXT, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lastStateCLK = digitalRead(CLK);
  readFromNVS();
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
  encoder();

  // Trigger ultrasonic when in set mode or on long press
  if (flagHold == 5) {
    ultrasonic();
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  if (modMenu == 0) {
    display.setCursor(0, 0);
    display.print("Mode Set: ");
    display.setCursor(0, 16);
    if (holdMenu == 0) display.print(" ");
    display.print("1. Tank A");
    display.setCursor(0, 32);
    if (holdMenu == 1) display.print(" ");
    display.print("2. Tank B");
    display.setCursor(0, 48);
    if (holdMenu == 2) display.print(" ");
    display.print("3. Tank C");
    if (holdMenu == 3 || holdMenu == 4) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.print("Mode Set: ");
      display.setCursor(0, 16);
      if (holdMenu == 3) display.print(" ");
      display.print("4. Data Variable");
      display.setCursor(0, 32);
      if (holdMenu == 4) display.print(" ");
      display.print("5. Tank Display");
    }
  } else if (modMenu == 1 && holdMenu == 4) {
    if (nextMenu == 0) {
      display.setCursor(0, 0);
      display.print("Tank A: "); display.print(percentage[0], 2); display.print(" %");
      Blynk.virtualWrite(V0, percentage[0], 2);
      display.setCursor(0, 16);
      display.print("Height: "); display.print(estimatedHeightA, 2); display.print(" cm");
      Blynk.virtualWrite(V3, estimatedHeightA, 2);
      display.setCursor(0, 32);
      display.print("Lbs: "); display.print(estimatedWeightA, 2); display.print(" lbs");
      Blynk.virtualWrite(V4, estimatedWeightA, 2);
      display.setCursor(0, 48);
      display.print("Litre: "); display.print(estimatedLitre[0], 2); display.print(" L");
      Blynk.virtualWrite(V5, estimatedLitre[0], 2);
    } else if (nextMenu == 1) {
      display.setCursor(0, 0);
      display.print("Tank B: "); display.print(percentage[1], 2); display.print(" %");
      Blynk.virtualWrite(V1, percentage[1], 2);
      display.setCursor(0, 16);
      display.print("Height: "); display.print(estimatedHeightB, 2); display.print(" cm");
      Blynk.virtualWrite(V6, estimatedHeightB, 2);
      display.setCursor(0, 32);
      display.print("Lbs: "); display.print(estimatedWeightB, 2); display.print(" lbs");
      Blynk.virtualWrite(V7, estimatedWeightB, 2);
      display.setCursor(0, 48);
      display.print("Litre: "); display.print(estimatedLitre[1], 2); display.print(" L");
      Blynk.virtualWrite(V8, estimatedLitre[1], 2);
    } else if (nextMenu == 2) {
      display.setCursor(0, 0);
      display.print("Tank C: "); display.print(percentage[2], 2); display.print(" %");
      Blynk.virtualWrite(V2, percentage[2], 2);
      display.setCursor(0, 16);
      display.print("Height: "); display.print(estimatedHeightC, 2); display.print(" cm");
      Blynk.virtualWrite(V9, estimatedHeightC, 2);
      display.setCursor(0, 32);
      display.print("Lbs: "); display.print(estimatedWeightC, 2); display.print(" lbs");
      Blynk.virtualWrite(V10, estimatedWeightC, 2);
      display.setCursor(0, 48);
      display.print("Litre: "); display.print(estimatedLitre[2], 2); display.print(" L");
      Blynk.virtualWrite(V11, estimatedLitre[2], 2);
    } else if (nextMenu == 3) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.print("<--");
    }
  } else if (flagHold == 0 && flagMod == 1) {
    display.setCursor(0, 0);
    display.print("Set Tank A: ");
    display.setCursor(0, 16);
    display.print("Height: "); display.print(tank_HeightRead[0]);
    display.setCursor(0, 32);
    display.print("Lbs: "); display.print(tank_LbsRead[0]);
    display.setCursor(0, 48);
    display.print("Litre: "); display.print(tank_LitreRead[0]);
  } else if (holdMenu == 1 && flagMod == 1) {
    display.setCursor(0, 0);
    display.print("Set Tank B:");
    display.setCursor(0, 16);
    display.print("Height: "); display.print(tank_HeightRead[1]);
    display.setCursor(0, 32);
    display.print("Lbs: "); display.print(tank_LbsRead[1]);
    display.setCursor(0, 48);
    display.print("Litre: "); display.print(tank_LitreRead[1]);
  } else if (holdMenu == 2 && flagMod == 1) {
    display.setCursor(0, 0);
    display.print("Set Tank C:");
    display.setCursor(0, 16);
    display.print("Height: "); display.print(tank_HeightRead[2]);
    display.setCursor(0, 32);
    display.print("Lbs: "); display.print(tank_LbsRead[2]);
    display.setCursor(0, 48);
    display.print("Litre: "); display.print(tank_LitreRead[2]);
  } else if (holdMenu == 3 && flagMod == 1) {
    display.setCursor(0, 0);
    display.print("Use variable data set?");
    display.setCursor(0, 16);
    if (nextMenu == 0) display.print(" ");
    display.print("YES");
    display.setCursor(0, 32);
    if (nextMenu == 1) display.print(" ");
    display.print("NO");
    display.setCursor(0, 48);
    if (nextMenu == 2) display.print(" ");
    display.print("SET");
  }

  display.display();

  Serial.print("MOD: "); Serial.print(modMenu);
  Serial.print(" | HOLD: "); Serial.print(holdMenu);
  Serial.print(" | NEXT: "); Serial.println(nextMenu);
  Serial.println("Loaded Tanks Values:");
  Serial.print("A: Height: "); Serial.print(tank_HeightRead[0]);
  Serial.print(" | Lbs: "); Serial.print(tank_LbsRead[0]);
  Serial.print(" | Litre: "); Serial.println(tank_LitreRead[0]);
  Serial.print("B: Height: "); Serial.print(tank_HeightRead[1]);
  Serial.print(" | Lbs: "); Serial.print(tank_LbsRead[1]);
  Serial.print(" | Litre: "); Serial.println(tank_LitreRead[1]);
  Serial.print("C: Height: "); Serial.print(tank_HeightRead[2]);
  Serial.print(" | Lbs: "); Serial.print(tank_LbsRead[2]);
  Serial.print(" | Litre: "); Serial.println(tank_LitreRead[2]);
  Serial.println();

  delay(10);
}

float interpolateHeightToWeightA(float heightTankA) {
  if (heightTankA <= heightDataA[0]) return weightDataA[0];
  if (heightTankA >= heightDataA[dataSizeA - 1]) return weightDataA[dataSizeA - 1];
  for (int i = 0; i < dataSizeA - 1; i++) {
    if (heightTankA >= heightDataA[i] && heightTankA <= heightDataA[i + 1]) {
      return weightDataA[i] + (heightTankA - heightDataA[i]) * (weightDataA[i + 1] - weightDataA[i]) / (heightDataA[i + 1] - heightDataA[i]);
    }
  }
  return 0;
}

float interpolateWeightToHeightA(float weightTankA) {
  if (weightTankA <= weightDataA[0]) return heightDataA[0];
  if (weightTankA >= weightDataA[dataSizeA - 1]) return heightDataA[dataSizeA - 1];
  for (int i = 0; i < dataSizeA - 1; i++) {
    if (weightTankA >= weightDataA[i] && weightTankA <= weightDataA[i + 1]) {
      return heightDataA[i] + (weightTankA - weightDataA[i]) * (heightDataA[i + 1] - heightDataA[i]) / (weightDataA[i + 1] - heightDataA[i]);
    }
  }
  return 0;
}

float interpolateHeightToWeightB(float heightTankB) {
  if (heightTankB <= heightDataB[0]) return weightDataB[0];
  if (heightTankB >= heightDataB[dataSizeB - 1]) return weightDataB[dataSizeB - 1];
  for (int i = 0; i < dataSizeB - 1; i++) {
    if (heightTankB >= heightDataB[i] && heightTankB <= heightDataB[i + 1]) {
      return weightDataB[i] + (heightTankB - heightDataB[i]) * (weightDataB[i + 1] - weightDataB[i]) / (heightDataB[i + 1] - heightDataB[i]);
    }
  }
  return 0;
}

float interpolateWeightToHeightB(float weightTankB) {
  if (weightTankB <= weightDataB[0]) return heightDataB[0];
  if (weightTankB >= weightDataB[dataSizeB - 1]) return heightDataB[dataSizeB - 1];
  for (int i = 0; i < dataSizeB - 1; i++) {
    if (weightTankB >= weightDataB[i] && weightTankB <= weightDataB[i + 1]) {
      return heightDataB[i] + (weightTankB - weightDataB[i]) * (heightDataB[i + 1] - heightDataB[i]) / (weightDataB[i + 1] - heightDataB[i]);
    }
  }
  return 0;
}

float interpolateHeightToWeightC(float heightTankC) {
  if (heightTankC <= heightDataC[0]) return weightDataC[0];
  if (heightTankC >= heightDataC[dataSizeC - 1]) return weightDataC[dataSizeC - 1];
  for (int i = 0; i < dataSizeC - 1; i++) {
    if (heightTankC >= heightDataC[i] && heightTankC <= heightDataC[i + 1]) {
      return weightDataC[i] + (heightTankC - heightDataC[i]) * (weightDataC[i + 1] - weightDataC[i]) / (heightDataC[i + 1] - heightDataC[i]);
    }
  }
  return 0;
}

float interpolateWeightToHeightC(float weightTankC) {
  if (weightTankC <= weightDataC[0]) return heightDataC[0];
  if (weightTankC >= weightDataC[dataSizeC - 1]) return heightDataC[dataSizeC - 1];
  for (int i = 0; i < dataSizeC - 1; i++) {
    if (weightTankC >= weightDataC[i] && weightTankC <= weightDataC[i + 1]) {
      return heightDataC[i] + (weightTankC - weightDataC[i]) * (heightDataC[i + 1] - heightDataC[i]) / (weightDataC[i + 1] - heightDataC[i]);
    }
  }
  return 0;
}

//Litre calculation
float calculateLitersFromWeight(float weight_lbs) {
  const float density_lbs_per_liter = 2.2;
  return weight_lbs / density_lbs_per_liter;
}

//Percentage calculation
float calculateTankPercentage(float currentHeight, float totalHeight) {
  if (totalHeight <= 0) return 0.0; // Avoid division by zero
  float percentage = (currentHeight / totalHeight) * 100.0;
  if (percentage < 0) return 0.0;   // Clamp to 0% if negative
  if (percentage > 100) return 100.0; // Clamp to 100% if overfull
  return percentage;
}
