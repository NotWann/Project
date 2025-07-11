#define BLYNK_PRINT Serial

#include <Wire.h>  
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>
#include <SPI.h>
#include <BlynkSimpleEsp32.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
File dataFile;

#define SD_CS_PIN 5

#define MOD 27   // Green
#define NEXT 26  // Yellow 1
#define HOLD 25  // Red
#define VAR 14   // Yellow 2

#define CLK 34
#define DT 35
#define SW 4

#define TRIG_PIN 32
#define ECHO_PIN 33

/////////////////////////////////////////////
char auth[] = "RfX6B_kVE8_zhTCCwblgNW2rbDEzb4ry";
char ssid[] = "abcd";
char pass[] = "123456789";

char server[] = "prakitblog.com";
int port = 8181;

/////////////////////////////////////////////
const int buzzerPin = 2;

int counter = 0;
int currentStateCLK;
int lastStateCLK = 0;
unsigned long lastButtonPress = 0;
int lockPoint;

/////////////////////////////////////////////
int modMenu = 0, holdMenu = 0, nextMenu = 0, varMenu = 0;
int flagMod = 0, flagHold = 0, flagNext = 0, flagVar = 0;

bool useVarData = false; // Default to hardcoded data
bool isFreeze = false;

/////////////////////////////////////////////
// Hardcoded data
float weightDataA[] = { 0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000 };
float heightDataA[] = { 0, 7.3, 14.5, 21.1, 27.2, 32.5, 37.4, 42.4, 47.5, 52.8, 58, 62.9, 68.3, 73.5, 79.2, 85, 91.3, 97.3, 101.3 };
int dataSizeA = sizeof(weightDataA) / sizeof(weightDataA[0]);

float weightDataB[] = { 0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 8700, 9000 };
float heightDataB[] = { 0, 3.8, 8.2, 11.5, 14, 16.3, 18.5, 20.7, 23, 25.2, 27.4, 29.8, 32, 34.6, 36.9, 39.5, 42.3, 45.5, 47.1, 50.2 };
int dataSizeB = sizeof(weightDataB) / sizeof(weightDataB[0]);

float weightDataC[] = { 0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 7900, 8000 };
float heightDataC[] = { 0, 5.6, 10.2, 14.7, 18.8, 23, 26.7, 30.8, 34.9, 38.9, 43, 47.1, 51.8, 55.5, 59.6, 63.8, 67.8, 70.2 };
int dataSizeC = sizeof(weightDataC) / sizeof(weightDataC[0]);

/////////////////////////////////////////////
float estimatedWeightA, estimatedHeightA;
float estimatedWeightB, estimatedHeightB;
float estimatedWeightC, estimatedHeightC;
float estimatedLitre[3] = { 0, 0, 0 };

float varSetWeightARead[20] = { 0 };
float varSetHeightARead[20] = { 0 };
float varSetWeightBRead[20] = { 0 };
float varSetHeightBRead[20] = { 0 };
float varSetWeightCRead[20] = { 0 };
float varSetHeightCRead[20] = { 0 };

int tank_HeightRead[3] = { 0, 0, 0 };
float tank_LbsRead[3] = { 0, 0, 0 };
float tank_LitreRead[3] = { 0, 0, 0 };

int set_Height[3] = { 0, 0, 0 };
float set_Lbs[3] = { 0, 0, 0 };
float set_Litre[3] = { 0, 0, 0 };

float distance[3] = { 0, 0, 0 };
float lastValidDistance[3] = { 0, 0, 0 };
int heightTank[3] = { 0, 0, 0 };
float lbsTank[3] = { 0, 0, 0 };
float litreTank[3] = { 0, 0, 0 };
float percentage[3] = { 0, 0, 0 };

/////////////////////////////////////////////
void saveToSD() {
  dataFile = SD.open("/tank_data.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println("UseVarData");
    dataFile.println(useVarData ? "1" : "0");

    dataFile.println("Tank,Height,Lbs,Litre");
    dataFile.print("A,"); dataFile.print(tank_HeightRead[0]); dataFile.print(",");
    dataFile.print(tank_LbsRead[0]); dataFile.print(","); dataFile.println(tank_LitreRead[0]);
    dataFile.print("B,"); dataFile.print(tank_HeightRead[1]); dataFile.print(",");
    dataFile.print(tank_LbsRead[1]); dataFile.print(","); dataFile.println(tank_LitreRead[1]);
    dataFile.print("C,"); dataFile.print(tank_HeightRead[2]); dataFile.print(",");
    dataFile.print(tank_LbsRead[2]); dataFile.print(","); dataFile.println(tank_LitreRead[2]);

    dataFile.println("TankA_VarIndex,Height,Weight");
    for (int i = 0; i < 20; i++) {
      dataFile.print(i + 1); dataFile.print(",");
      dataFile.print(varSetHeightARead[i]); dataFile.print(",");
      dataFile.println(varSetWeightARead[i]);
    }
    dataFile.println("TankB_VarIndex,Height,Weight");
    for (int i = 0; i < 20; i++) {
      dataFile.print(i + 1); dataFile.print(",");
      dataFile.print(varSetHeightBRead[i]); dataFile.print(",");
      dataFile.println(varSetWeightBRead[i]);
    }
    dataFile.println("TankC_VarIndex,Height,Weight");
    for (int i = 0; i < 20; i++) {
      dataFile.print(i + 1); dataFile.print(",");
      dataFile.print(varSetHeightCRead[i]); dataFile.print(",");
      dataFile.println(varSetWeightCRead[i]);
    }
    dataFile.close();
    Serial.println("Data saved to SD card in CSV format");
  } else {
    Serial.println("Error opening tank_data.csv for writing");
  }
}

/////////////////////////////////////////////
void readFromSD() {
  for (int i = 0; i < 3; i++) {
    tank_HeightRead[i] = 0;
    tank_LbsRead[i] = 0;
    tank_LitreRead[i] = 0;
  }
  for (int i = 0; i < 20; i++) {
    varSetHeightARead[i] = 0;
    varSetWeightARead[i] = 0;
    varSetHeightBRead[i] = 0;
    varSetWeightBRead[i] = 0;
    varSetHeightCRead[i] = 0;
    varSetWeightCRead[i] = 0;
  }

  dataFile = SD.open("/tank_data.csv", FILE_READ);
  if (dataFile) {
    if (dataFile.available()) {
      dataFile.readStringUntil('\n'); // Skip "UseVarData"
      String line = dataFile.readStringUntil('\n');
      useVarData = (line.toInt() == 1);
    }
    if (dataFile.available()) {
      dataFile.readStringUntil('\n'); // Skip "Tank,Height,Lbs,Litre"
    }
    for (int i = 0; i < 3; i++) {
      if (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');
        int comma1 = line.indexOf(',');
        int comma2 = line.indexOf(',', comma1 + 1);
        int comma3 = line.indexOf(',', comma2 + 1);
        if (comma1 != -1 && comma2 != -1 && comma3 != -1) {
          tank_HeightRead[i] = line.substring(comma1 + 1, comma2).toFloat();
          tank_LbsRead[i] = line.substring(comma2 + 1, comma3).toFloat();
          tank_LitreRead[i] = line.substring(comma3 + 1).toFloat();
        }
      }
    }
    if (dataFile.available()) {
      dataFile.readStringUntil('\n'); // Skip "TankA_VarIndex,Height,Weight"
    }
    for (int i = 0; i < 20; i++) {
      if (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');
        int comma1 = line.indexOf(',');
        int comma2 = line.indexOf(',', comma1 + 1);
        if (comma1 != -1 && comma2 != -1) {
          varSetHeightARead[i] = line.substring(comma1 + 1, comma2).toFloat();
          varSetWeightARead[i] = line.substring(comma2 + 1).toFloat();
        }
      }
    }
    if (dataFile.available()) {
      dataFile.readStringUntil('\n'); // Skip "TankB_VarIndex,Height,Weight"
    }
    for (int i = 0; i < 20; i++) {
      if (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');
        int comma1 = line.indexOf(',');
        int comma2 = line.indexOf(',', comma1 + 1);
        if (comma1 != -1 && comma2 != -1) {
          varSetHeightBRead[i] = line.substring(comma1 + 1, comma2).toFloat();
          varSetWeightBRead[i] = line.substring(comma2 + 1).toFloat();
        }
      }
    }
    if (dataFile.available()) {
      dataFile.readStringUntil('\n'); // Skip "TankC_VarIndex,Height,Weight"
    }
    for (int i = 0; i < 20; i++) {
      if (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');
        int comma1 = line.indexOf(',');
        int comma2 = line.indexOf(',', comma1 + 1);
        if (comma1 != -1 && comma2 != -1) {
          varSetHeightCRead[i] = line.substring(comma1 + 1, comma2).toFloat();
          varSetWeightCRead[i] = line.substring(comma2 + 1).toFloat();
        }
      }
    }
    dataFile.close();
    Serial.println("Data read from SD card in CSV format");
    Serial.print("Using variable data: "); Serial.println(useVarData ? "Yes" : "No");
  } else {
    Serial.println("Error opening tank_data.csv for reading");
  }
}

/////////////////////////////////////////////
void encoder() {
  currentStateCLK = digitalRead(CLK);
  if (currentStateCLK == 1 && lastStateCLK == 0) {
    if ((digitalRead(DT) == 1 && currentStateCLK == 1) && lockPoint == 0) {
      counter++;
    }
    if ((digitalRead(DT) == 1 && currentStateCLK == 1) && lockPoint == 1) {
      counter--;
    }
    Serial.print("Counter: "); Serial.println(counter);
    updateValues();
  }
  lastStateCLK = currentStateCLK;

  int btnState = digitalRead(SW);
  if (btnState == LOW) {
    if (millis() - lastButtonPress > 50) {
      Serial.println("Button pressed!");
      lockPoint = (lockPoint + 1) % 2;
    }
    lastButtonPress = millis();
  }
  delay(1);
}

/////////////////////////////////////////////
void updateValues() {
  if (flagHold == 1 && flagMod == 1) { // Set Height menu
    if (flagNext == 1) {
      set_Height[0] = counter;
      tank_HeightRead[0] = set_Height[0];
    } else if (flagNext == 2) {
      set_Height[1] = counter;
      tank_HeightRead[1] = set_Height[1];
    } else if (flagNext == 3) {
      set_Height[2] = counter;
      tank_HeightRead[2] = set_Height[2];
    }
  } else if (flagMod == 2 && flagHold == 2) { // Set Var menu
    if (flagVar >= 0 && flagVar <= 39) {
      int index = flagVar / 2;
      if (flagNext == 1) { // Tank A
        if (flagVar % 2 == 0) {
          varSetHeightARead[index] = counter;
        } else {
          varSetWeightARead[index] = counter;
        }
      } else if (flagNext == 2) { // Tank B
        if (flagVar % 2 == 0) {
          varSetHeightBRead[index] = counter;
        } else {
          varSetWeightBRead[index] = counter;
        }
      } else if (flagNext == 3) { // Tank C
        if (flagVar % 2 == 0) {
          varSetHeightCRead[index] = counter;
        } else {
          varSetWeightCRead[index] = counter;
        }
      }
    }
  }
}

/////////////////////////////////////////////
void ultrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 40000);
  float rawDistance = (duration * 0.0343) / 2;

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
      heightTank[tankIndex] = tank_HeightRead[tankIndex];
    } else {
      heightTank[tankIndex] = tank_HeightRead[tankIndex] - distance[tankIndex];
    }

    if (tankIndex == 0) {
      estimatedWeightA = interpolateVarHeightToWeightA(heightTank[0]);
      estimatedHeightA = interpolateVarWeightToHeightA(estimatedWeightA);
      estimatedLitre[0] = calculateLitersFromWeight(estimatedWeightA);
      percentage[0] = calculateTankPercentage(heightTank[0], tank_HeightRead[0]);
      Blynk.virtualWrite(V0, String(percentage[0], 2));
      Blynk.virtualWrite(V3, String(estimatedHeightA, 2));
      Blynk.virtualWrite(V4, String(estimatedWeightA, 2));
      Blynk.virtualWrite(V5, String(estimatedLitre[0], 2));
    } else if (tankIndex == 1) {
      estimatedWeightB = interpolateVarHeightToWeightB(heightTank[1]);
      estimatedHeightB = interpolateVarWeightToHeightB(estimatedWeightB);
      estimatedLitre[1] = calculateLitersFromWeight(estimatedWeightB);
      percentage[1] = calculateTankPercentage(heightTank[1], tank_HeightRead[1]);
      Blynk.virtualWrite(V1, String(percentage[1], 2));
      Blynk.virtualWrite(V6, String(estimatedHeightB, 2));
      Blynk.virtualWrite(V7, String(estimatedWeightB, 2));
      Blynk.virtualWrite(V8, String(estimatedLitre[1], 2));
    } else if (tankIndex == 2) {
      estimatedWeightC = interpolateVarHeightToWeightC(heightTank[2]);
      estimatedHeightC = interpolateVarWeightToHeightC(estimatedWeightC);
      estimatedLitre[2] = calculateLitersFromWeight(estimatedWeightC);
      percentage[2] = calculateTankPercentage(heightTank[2], tank_HeightRead[2]);
      Blynk.virtualWrite(V2, String(percentage[2], 2));
      Blynk.virtualWrite(V9, String(estimatedHeightC, 2));
      Blynk.virtualWrite(V10, String(estimatedWeightC, 2));
      Blynk.virtualWrite(V11, String(estimatedLitre[2], 2));
    }
  }

  Serial.print("Estimated Weight Tank A: "); Serial.print(estimatedWeightA); Serial.println(" lbs");
  Serial.print("Estimated Height Tank A: "); Serial.print(estimatedHeightA); Serial.println(" cm");
  Serial.print("Estimated Weight Tank B: "); Serial.print(estimatedWeightB); Serial.println(" lbs");
  Serial.print("Estimated Height Tank B: "); Serial.print(estimatedHeightB); Serial.println(" cm");
  Serial.print("Estimated Weight Tank C: "); Serial.print(estimatedWeightC); Serial.println(" lbs");
  Serial.print("Estimated Height Tank C: "); Serial.print(estimatedHeightC); Serial.println(" cm");
  Serial.println(" ");
  Serial.print("Height Tank A: "); Serial.print(heightTank[0]); Serial.println(" cm");
  Serial.print("Height Tank B: "); Serial.print(heightTank[1]); Serial.println(" cm");
  Serial.print("Height Tank C: "); Serial.print(heightTank[2]); Serial.println(" cm");
  Serial.println(" ");
}

/////////////////////////////////////////////
void modButton() {
  static unsigned long varDataSetpress = 0;
  const unsigned long longPressVarDuration = 1000;

  if (digitalRead(MOD) == LOW && varDataSetpress == 0) {
    varDataSetpress = millis();
    sound();
  }

  if (digitalRead(MOD) == HIGH && varDataSetpress != 0) {
    unsigned long pressVarDuration = millis() - varDataSetpress;
    if (pressVarDuration < longPressVarDuration) {
      modMenu = (modMenu + 1) % 2;
      flagMod = modMenu;
    } else {
      flagMod = 2; // Long press still triggers variable data setup mode
      Serial.println("Entering variable data setup mode");
    }
    varDataSetpress = 0;
  }
}

/////////////////////////////////////////////
void holdButton() {
  static unsigned long pressStartTime = 0;
  const unsigned long longPressDuration = 1000;
  const unsigned long holdDataDuration = 2000;

  if (digitalRead(HOLD) == LOW && pressStartTime == 0) {
    pressStartTime = millis();
    sound();
  }

  if (digitalRead(HOLD) == HIGH && pressStartTime != 0) {
    unsigned long pressDuration = millis() - pressStartTime;
    if (pressDuration < longPressDuration) {
      holdMenu = (holdMenu + 1) % 4;
      flagHold = holdMenu;
    } else if (pressDuration >= holdDataDuration) {
      isFreeze = !isFreeze; // Toggle freeze state
      Serial.println(isFreeze ? "Display Frozen!" : "Display Unfrozen!");
      if (isFreeze) {
        sound(); // Double beep for freeze
        delay(100);
        sound();
      }
    } else { // Between 1s and 2s
      flagHold = 4;
      isFreeze = false;
      Serial.println("Long press detected (less than 2s)!");
    }
    pressStartTime = 0;
  }
}

/////////////////////////////////////////////
void nextButton() {
  static unsigned long pressStartTime = 0;
  const unsigned long longPressDuration = 1000;

  if (digitalRead(NEXT) == LOW && pressStartTime == 0) {
    pressStartTime = millis();
    delay(100); // Debounce
    nextMenu = (nextMenu + 1) % 4;
    flagNext = nextMenu;
    sound();
  }

  if (digitalRead(NEXT) == HIGH && pressStartTime != 0) {
    unsigned long pressDuration = millis() - pressStartTime;

    // Handle "Use data set?" menu choice with long press
    if (flagMod == 1 && flagHold == 2) {
      if (flagNext == 1 && pressDuration >= longPressDuration) { // Long press on YES
        useVarData = true;
        Serial.println("Long press on YES: Set to use variable data");
        saveToSD();
      } else if (flagNext == 2 && pressDuration >= longPressDuration) { // Long press on NO
        useVarData = false;
        Serial.println("Long press on NO: Set to use hardcoded data");
        saveToSD();
      } else if (flagNext == 3 && pressDuration >= longPressDuration) { // Long press on SET (optional reset)
        Serial.println("Long press on SET: No change (already saved)");
      } else {
        Serial.println("Short press detected, no change to data choice");
      }
    } else {
      saveToSD(); // Save other data for short presses outside "Use data set?" menu
    }

    pressStartTime = 0;
  }
}

/////////////////////////////////////////////
void varSelectButton() {
  if (digitalRead(VAR) == LOW) {
    delay(100);
    varMenu = (varMenu + 1) % 40;
    flagVar = varMenu;
    sound();
    saveToSD();
    while (digitalRead(VAR) == LOW);
  }
}

/////////////////////////////////////////////
void sound() {
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
}

/////////////////////////////////////////////
void connectToBlynk() {
  unsigned long start = millis();
  while (!Blynk.connected() && millis() - start < 10000) {
    Blynk.run();
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
    for (int i = 0; i < 2; i++) sound();
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

/////////////////////////////////////////////
void updateDisplay() {
  bool mainMenu = (flagMod == 0);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Show freeze status in Tank Display mode
  if (flagMod == 1 && (flagHold == 3 || flagHold == 4)) {
    display.setCursor(100, 0); // Top right corner
    display.print(isFreeze ? "F" : "L"); // F for Frozen, L for Live
  }

  if (mainMenu) {
    display.setCursor(0, 0);
    display.print("Mode Set: ");
    display.setCursor(0, 16);
    if (flagHold == 1) display.print("> ");
    display.print("1. Set Height");
    display.setCursor(0, 32);
    if (flagHold == 2) display.print("> ");
    display.print("2. Set Data");
    display.setCursor(0, 48);
    if (flagHold == 3) display.print("> ");
    display.print("3. Tank Display");
  }

  if (mainMenu) {
    display.setCursor(0, 0);
    display.print("Mode Set: ");
    display.setCursor(0, 16);
    if (flagHold == 1) display.print("> ");
    display.print("1. Set Height");
    display.setCursor(0, 32);
    if (flagHold == 2) display.print("> ");
    display.print("2. Set Data");
    display.setCursor(0, 48);
    if (flagHold == 3) display.print("> ");
    display.print("3. Tank Display");
  }

  if (flagMod == 1 && flagHold == 1) { // Set Height Menu
    display.setCursor(0, 0);
    display.print("Set Height: ");
    display.setCursor(0, 16);
    if (flagNext == 1) display.print("> ");
    display.print("Tank A: "); display.print(tank_HeightRead[0]); display.print(" cm");
    display.setCursor(0, 32);
    if (flagNext == 2) display.print("> ");
    display.print("Tank B: "); display.print(tank_HeightRead[1]); display.print(" cm");
    display.setCursor(0, 48);
    if (flagNext == 3) display.print("> ");
    display.print("Tank C: "); display.print(tank_HeightRead[2]); display.print(" cm");
  }

  if (flagMod == 1 && flagHold == 2) { // Set Data Menu
    display.setCursor(0, 0);
    display.print("Use data set?");
    display.setCursor(0, 16);
    if (flagNext == 1) display.print("> ");
    display.print("YES");
    display.setCursor(0, 32);
    if (flagNext == 2) display.print("> ");
    display.print("NO");
    display.setCursor(0, 48);
    if (flagNext == 3) display.print("> ");
    display.print("SET");
  }

  if (flagMod == 1 && (flagHold == 3 || flagHold == 4)) { // Tank Display
    if (flagNext == 0) {
      display.setCursor(0, 0);
      display.print("Tank A: "); display.print(percentage[0], 2); display.print(" %");
      display.setCursor(0, 16);
      display.print("Height: "); display.print(estimatedHeightA, 2); display.print(" cm");
      display.setCursor(0, 32);
      display.print("Lbs: "); display.print(estimatedWeightA, 2); display.print(" lbs");
      display.setCursor(0, 48);
      display.print("Litre: "); display.print(estimatedLitre[0], 2); display.print(" L");
    } else if (flagNext == 1) {
      display.setCursor(0, 0);
      display.print("Tank B: "); display.print(percentage[1], 2); display.print(" %");
      display.setCursor(0, 16);
      display.print("Height: "); display.print(estimatedHeightB, 2); display.print(" cm");
      display.setCursor(0, 32);
      display.print("Lbs: "); display.print(estimatedWeightB, 2); display.print(" lbs");
      display.setCursor(0, 48);
      display.print("Litre: "); display.print(estimatedLitre[1], 2); display.print(" L");
    } else if (flagNext == 2) {
      display.setCursor(0, 0);
      display.print("Tank C: "); display.print(percentage[2], 2); display.print(" %");
      display.setCursor(0, 16);
      display.print("Height: "); display.print(estimatedHeightC, 2); display.print(" cm");
      display.setCursor(0, 32);
      display.print("Lbs: "); display.print(estimatedWeightC, 2); display.print(" lbs");
      display.setCursor(0, 48);
      display.print("Litre: "); display.print(estimatedLitre[2], 2); display.print(" L");
    } else if (flagNext == 3) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.print("<--");
    }
  }

  if (flagMod == 2 && flagHold == 2) {
    if (flagVar >= 0 && flagVar <= 39) {
      int index = flagVar / 2;
      if (flagNext == 1) { // Tank A
        display.setCursor(0, 0);
        display.print("Set Var Tank A: "); display.print(index + 1);
        display.setCursor(0, 16);
        display.print("Height: "); display.print(varSetHeightARead[index]);
        display.setCursor(0, 32);
        display.print("Lbs: "); display.print(varSetWeightARead[index]);
      } else if (flagNext == 2) { // Tank B
        display.setCursor(0, 0);
        display.print("Set Var Tank B: "); display.print(index + 1);
        display.setCursor(0, 16);
        display.print("Height: "); display.print(varSetHeightBRead[index]);
        display.setCursor(0, 32);
        display.print("Lbs: "); display.print(varSetWeightBRead[index]);
      } else if (flagNext == 3) { // Tank C
        display.setCursor(0, 0);
        display.print("Set Var Tank C: "); display.print(index + 1);
        display.setCursor(0, 16);
        display.print("Height: "); display.print(varSetHeightCRead[index]);
        display.setCursor(0, 32);
        display.print("Lbs: "); display.print(varSetWeightCRead[index]);
      }
    }
  }
  display.display();
}

/////////////////////////////////////////////
void setup() {
  Serial.begin(115200);

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("SD Card Failed");
    display.display();
    while (1);
  }
  Serial.println("SD card initialized.");

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

  delay(1000);

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  pinMode(MOD, INPUT_PULLUP);
  pinMode(HOLD, INPUT_PULLUP);
  pinMode(NEXT, INPUT_PULLUP);
  pinMode(VAR, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lastStateCLK = digitalRead(CLK);
  readFromSD();

  Serial.print("Initial useVarData: "); Serial.println(useVarData ? "Yes" : "No");
  display.clearDisplay();
}

/////////////////////////////////////////////
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
  varSelectButton();
  encoder();

  if (flagMod == 1 && flagHold == 4) {
    if (isFreeze == false) {
      ultrasonic();
    }  
  }

  updateDisplay();

  Serial.print("MOD: "); Serial.print(modMenu);
  Serial.print(" | HOLD: "); Serial.print(holdMenu);
  Serial.print(" | NEXT: "); Serial.print(nextMenu);
  Serial.print(" | VAR: "); Serial.println(varMenu);
  Serial.println("");

  Serial.println("Loaded Tanks Values:");
  Serial.print("A: Height: "); Serial.println(tank_HeightRead[0]);
  Serial.print("B: Height: "); Serial.println(tank_HeightRead[1]);
  Serial.print("C: Height: "); Serial.println(tank_HeightRead[2]);
  Serial.println(" ");

  Serial.print("Var Data: ");
  Serial.println(useVarData);
  Serial.println(" ");

  delay(10);
}

/////////////////////////////////////////////
// Hardcoded interpolation functions
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
      return heightDataA[i] + (weightTankA - weightDataA[i]) * (heightDataA[i + 1] - heightDataA[i]) / (weightDataA[i + 1] - weightDataA[i]);
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
      return heightDataB[i] + (weightTankB - weightDataB[i]) * (heightDataB[i + 1] - heightDataB[i]) / (weightDataB[i + 1] - weightDataB[i]);
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
      return heightDataC[i] + (weightTankC - weightDataC[i]) * (heightDataC[i + 1] - heightDataC[i]) / (weightDataC[i + 1] - weightDataC[i]);
    }
  }
  return 0;
}

/////////////////////////////////////////////
// Variable data interpolation functions
float interpolateVarHeightToWeightA(float heightTankA) {
  if (useVarData) {
    int dataSize = 20;
    if (heightTankA <= varSetHeightARead[0]) return varSetWeightARead[0];
    if (heightTankA >= varSetHeightARead[dataSize - 1]) return varSetWeightARead[dataSize - 1];
    for (int i = 0; i < dataSize - 1; i++) {
      if (heightTankA >= varSetHeightARead[i] && heightTankA <= varSetHeightARead[i + 1]) {
        return varSetWeightARead[i] + (heightTankA - varSetHeightARead[i]) * 
               (varSetWeightARead[i + 1] - varSetWeightARead[i]) / 
               (varSetHeightARead[i + 1] - varSetHeightARead[i]);
      }
    }
    return 0;
  } else {
    return interpolateHeightToWeightA(heightTankA);
  }
}

float interpolateVarWeightToHeightA(float weightTankA) {
  if (useVarData) {
    int dataSize = 20;
    if (weightTankA <= varSetWeightARead[0]) return varSetHeightARead[0];
    if (weightTankA >= varSetWeightARead[dataSize - 1]) return varSetHeightARead[dataSize - 1];
    for (int i = 0; i < dataSize - 1; i++) {
      if (weightTankA >= varSetWeightARead[i] && weightTankA <= varSetWeightARead[i + 1]) {
        return varSetHeightARead[i] + (weightTankA - varSetWeightARead[i]) * 
               (varSetHeightARead[i + 1] - varSetHeightARead[i]) / 
               (varSetWeightARead[i + 1] - varSetWeightARead[i]);
      }
    }
    return 0;
  } else {
    return interpolateWeightToHeightA(weightTankA);
  }
}

float interpolateVarHeightToWeightB(float heightTankB) {
  if (useVarData) {
    int dataSize = 20;
    if (heightTankB <= varSetHeightBRead[0]) return varSetWeightBRead[0];
    if (heightTankB >= varSetHeightBRead[dataSize - 1]) return varSetWeightBRead[dataSize - 1];
    for (int i = 0; i < dataSize - 1; i++) {
      if (heightTankB >= varSetHeightBRead[i] && heightTankB <= varSetHeightBRead[i + 1]) {
        return varSetWeightBRead[i] + (heightTankB - varSetHeightBRead[i]) * 
               (varSetWeightBRead[i + 1] - varSetWeightBRead[i]) / 
               (varSetHeightBRead[i + 1] - varSetHeightBRead[i]);
      }
    }
    return 0;
  } else {
    return interpolateHeightToWeightB(heightTankB);
  }
}

float interpolateVarWeightToHeightB(float weightTankB) {
  if (useVarData) {
    int dataSize = 20;
    if (weightTankB <= varSetWeightBRead[0]) return varSetHeightBRead[0];
    if (weightTankB >= varSetWeightBRead[dataSize - 1]) return varSetHeightBRead[dataSize - 1];
    for (int i = 0; i < dataSize - 1; i++) {
      if (weightTankB >= varSetWeightBRead[i] && weightTankB <= varSetWeightBRead[i + 1]) {
        return varSetHeightBRead[i] + (weightTankB - varSetWeightBRead[i]) * 
               (varSetHeightBRead[i + 1] - varSetHeightBRead[i]) / 
               (varSetWeightBRead[i + 1] - varSetWeightBRead[i]);
      }
    }
    return 0;
  } else {
    return interpolateWeightToHeightB(weightTankB);
  }
}

float interpolateVarHeightToWeightC(float heightTankC) {
  if (useVarData) {
    int dataSize = 20;
    if (heightTankC <= varSetHeightCRead[0]) return varSetWeightCRead[0];
    if (heightTankC >= varSetHeightCRead[dataSize - 1]) return varSetWeightCRead[dataSize - 1];
    for (int i = 0; i < dataSize - 1; i++) {
      if (heightTankC >= varSetHeightCRead[i] && heightTankC <= varSetHeightCRead[i + 1]) {
        return varSetWeightCRead[i] + (heightTankC - varSetHeightCRead[i]) * 
               (varSetWeightCRead[i + 1] - varSetWeightCRead[i]) / 
               (varSetHeightCRead[i + 1] - varSetHeightCRead[i]);
      }
    }
    return 0;
  } else {
    return interpolateHeightToWeightC(heightTankC);
  }
}

float interpolateVarWeightToHeightC(float weightTankC) {
  if (useVarData) {
    int dataSize = 20;
    if (weightTankC <= varSetWeightCRead[0]) return varSetHeightCRead[0];
    if (weightTankC >= varSetWeightCRead[dataSize - 1]) return varSetHeightCRead[dataSize - 1];
    for (int i = 0; i < dataSize - 1; i++) {
      if (weightTankC >= varSetWeightCRead[i] && weightTankC <= varSetWeightCRead[i + 1]) {
        return varSetHeightCRead[i] + (weightTankC - varSetWeightCRead[i]) * 
               (varSetHeightCRead[i + 1] - varSetHeightCRead[i]) / 
               (varSetWeightCRead[i + 1] - varSetWeightCRead[i]);
      }
    }
    return 0;
  } else {
    return interpolateWeightToHeightC(weightTankC);
  }
}

/////////////////////////////////////////////
float calculateLitersFromWeight(float weight_lbs) {
  const float density_lbs_per_liter = 2.2;
  return weight_lbs / density_lbs_per_liter;
}

/////////////////////////////////////////////
float calculateTankPercentage(float currentHeight, float totalHeight) {
  if (totalHeight <= 0) return 0.0;
  float percentage = (currentHeight / totalHeight) * 100.0;
  if (percentage < 0) return 0.0;
  if (percentage > 100) return 100.0;
  return percentage;
}