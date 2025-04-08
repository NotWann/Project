#define BLYNK_PRINT Serial

#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <BlynkSimpleEsp32.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create SSD1306 display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Preferences preferences;

#define MOD 27   // Green
#define NEXT 26  // Yellow
#define HOLD 25  // Red
#define VAR 14   // Var

// Rotary Encoder Inputs
#define CLK 34
#define DT 35
#define SW 4

#define TRIG_PIN 32
#define ECHO_PIN 33

char auth[] = "RfX6B_kVE8_zhTCCwblgNW2rbDEzb4ry";
char ssid[] = "abcd";
char pass[] = "123456789";

char server[] = "prakitblog.com";
int port = 8181;

const int buzzerPin = 2;

////////////////////////////////////////////////////////////////////////////////////////
int counter = 0;                    // Counter for encoder position
int currentStateCLK;                // Current state of CLK
int lastStateCLK = 0;               // Last state of CLK
unsigned long lastButtonPress = 0;  // Time of last button press

int lockPoint;  // Lock encoder rotation knob

////////////////////////////////////////////////////////////////////////////////////////
int modMenu = 0, holdMenu = 0, nextMenu = 0, varMenu = 0;
int flagMod = 0, flagHold = 0, flagNext = 0, flagVar = 0;

////////////////////////////////////////////////////////////////////////////////////////
// Sample data from your table (Pounds to Height in cm)
//Tank A
float weightDataA[] = { 0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000 };
float heightDataA[] = { 0, 7.3, 14.5, 21.1, 27.2, 32.5, 37.4, 42.4, 47.5, 52.8, 58, 62.9, 68.3, 73.5, 79.2, 85, 91.3, 97.3, 101.3 };
int dataSizeA = sizeof(weightDataA) / sizeof(weightDataA[0]);

//Tank B
float weightDataB[] = { 0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 8700, 9000 };
float heightDataB[] = { 0, 3.8, 8.2, 11.5, 14, 16.3, 18.5, 20.7, 23, 25.2, 27.4, 29.8, 32, 34.6, 36.9, 39.5, 42.3, 45.5, 47.1, 50.2 };
int dataSizeB = sizeof(weightDataB) / sizeof(weightDataB[0]);

//Tank C
float weightDataC[] = { 0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 7900, 8000 };
float heightDataC[] = { 0, 5.6, 10.2, 14.7, 18.8, 23, 26.7, 30.8, 34.9, 38.9, 43, 47.1, 51.8, 55.5, 59.6, 63.8, 67.8, 70.2 };
int dataSizeC = sizeof(weightDataC) / sizeof(weightDataC[0]);

float estimatedWeightA, estimatedHeightA;
float estimatedWeightB, estimatedHeightB;
float estimatedWeightC, estimatedHeightC;

float estimatedLitre[3] = { 0, 0, 0 };

////////////////////////////////////////////////////////////////////////////////////////
float varSetWeightA[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
float varSetHeightA[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };




////////////////////////////////////////////////////////////////////////////////////////
// Read and store variable
int tank_HeightRead[3] = { 0, 0, 0 };
float tank_LbsRead[3] = { 0, 0, 0 };
float tank_LitreRead[3] = { 0, 0, 0 };

//set encoder value
int set_Height[3] = { 0, 0, 0 };
float set_Lbs[3] = { 0, 0, 0 };
float set_Litre[3]{ 0, 0, 0 };

////////////////////////////////////////////////////////////////////////////////////////
float distance[3] = { 0, 0, 0 };  // Store distance for each tank
float lastValidDistance[3] = { 0, 0, 0 };
int heightTank[3] = { 0, 0, 0 };
float lbsTank[3] = { 0, 0, 0 };
float litreTank[3] = { 0, 0, 0 };
float percentage[3] = { 0, 0, 0 };

////////////////////////////////////////////////////////////////////////////////////////
// Function to save data to NVS
void saveToNVS() {
  preferences.begin("tankSet", false);  // Open NVS storage with namespace "tankA"

  preferences.putFloat("heightA", tank_HeightRead[0]);
  preferences.putFloat("lbsA", tank_LbsRead[0]);
  preferences.putFloat("litreA", tank_LitreRead[0]);

  preferences.putFloat("heightB", tank_HeightRead[1]);
  preferences.putFloat("lbsB", tank_LbsRead[1]);
  preferences.putFloat("litreB", tank_LitreRead[1]);

  preferences.putFloat("heightC", tank_HeightRead[2]);
  preferences.putFloat("lbsC", tank_LbsRead[2]);
  preferences.putFloat("litreC", tank_LitreRead[2]);

  preferences.end();  // Close NVS storage
}

// Function to read data from NVS
void readFromNVS() {
  preferences.begin("tankSet", true);  // Open NVS storage in read mode

  tank_HeightRead[0] = preferences.getFloat("heightA", 0.0);
  tank_LbsRead[0] = preferences.getFloat("lbsA", 0.0);
  tank_LitreRead[0] = preferences.getFloat("litreA", 0.0);

  tank_HeightRead[1] = preferences.getFloat("heightB", 0.0);
  tank_LbsRead[1] = preferences.getFloat("lbsB", 0.0);
  tank_LitreRead[1] = preferences.getFloat("litreB", 0.0);

  tank_HeightRead[2] = preferences.getFloat("heightC", 0.0);
  tank_LbsRead[2] = preferences.getFloat("lbsC", 0.0);
  tank_LitreRead[2] = preferences.getFloat("litreC", 0.0);

  preferences.end();  // Close NVS storage
}

////////////////////////////////////////////////////////////////////////////////////////
void saveVarDataSetA() {
  preferences.begin("varDataTankA", false);

  preferences.putFloat("varHeightA1", varSetHeightA[0]);
  preferences.putFloat("varWeightA1", varSetWeightA[0]);

  preferences.putFloat("varHeightA2", varSetHeightA[1]);
  preferences.putFloat("varWeightA2", varSetWeightA[1]);

  preferences.putFloat("varHeightA3", varSetHeightA[2]);
  preferences.putFloat("varWeightA3", varSetWeightA[2]);

  preferences.putFloat("varHeightA4", varSetHeightA[3]);
  preferences.putFloat("varWeightA4", varSetWeightA[3]);

  preferences.putFloat("varHeightA5", varSetHeightA[4]);
  preferences.putFloat("varWeightA5", varSetWeightA[4]);

  preferences.putFloat("varHeightA6", varSetHeightA[5]);
  preferences.putFloat("varWeightA6", varSetWeightA[5]);

  preferences.putFloat("varHeightA7", varSetHeightA[6]);
  preferences.putFloat("varWeightA7", varSetWeightA[6]);

  preferences.putFloat("varHeightA8", varSetHeightA[7]);
  preferences.putFloat("varWeightA8", varSetWeightA[7]);

  preferences.putFloat("varHeightA9", varSetHeightA[8]);
  preferences.putFloat("varWeightA9", varSetWeightA[8]);

  preferences.putFloat("varHeightA10", varSetHeightA[9]);
  preferences.putFloat("varWeightA10", varSetWeightA[9]);

  preferences.end();
}

void readVarDataSetA() {
  preferences.begin("varDataTankA", true);  // Open in read-only mode

  // Loop through all 20 elements of each array
  for (int i = 0; i < 20; i++) {
    char heightKey[16];  // Buffer for height key (e.g., "varHeightA1")
    char weightKey[16];  // Buffer for weight key (e.g., "varWeightA1")

    // Generate the same keys used when saving
    sprintf(heightKey, "varHeightA%d", i + 1);
    sprintf(weightKey, "varWeightA%d", i + 1);

    // Read values from NVS, default to 0.0 if not found
    varSetHeightA[i] = preferences.getFloat(heightKey, 0.0);
    varSetWeightA[i] = preferences.getFloat(weightKey, 0.0);

    // Print values to Serial Monitor
    Serial.print("Tank A ");
    Serial.print(i + 1);  // Display 1-20 instead of 0-19
    Serial.print(" - Height: ");
    Serial.print(varSetHeightA[i]);
    Serial.print(" | Weight: ");
    Serial.println(varSetWeightA[i]);
  }

  preferences.end();
}

////////////////////////////////////////////////////////////////////////////////////////
void encoder() {
  currentStateCLK = digitalRead(CLK);

  if (currentStateCLK == 1 && lastStateCLK == 0) {
    if ((digitalRead(DT) == 1 && currentStateCLK == 1) && lockPoint == 0) {
      counter++;
    }
    if ((digitalRead(DT) == 1 && currentStateCLK == 1) && lockPoint == 1) {
      counter--;
    }
    //Serial.print("Counter: "); Serial.println(counter);
    updateValues();
    varDisplaySetA();
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

////////////////////////////////////////////////////////////////////////////////////////
void updateValues() {
  if (flagHold == 1 && flagMod == 1) {
    if (flagNext == 1) {
      set_Height[0] = counter;
      tank_HeightRead[0] = set_Height[0];
    }

    else if (flagNext == 2) {
      set_Height[1] = counter;
      tank_HeightRead[1] = set_Height[1];
    }

    else if (flagNext == 3) {
      set_Height[2] = counter;
      tank_HeightRead[2] = set_Height[2];
    }
  }

  if (flagHold == 3 && flagNext == 2 && flagMod == 2) {
    if (flagVar >= 0 && flagVar <= 39)  // Valid range for 20 heights + 20 weights
    {
      int index = flagVar / 2;  // Array index from 0 to 19
      if (flagVar % 2 == 0)     // Even flagVar: set height
      {
        varSetHeightA[index] = counter;
      } else  // Odd flagVar: set weight
      {
        varSetWeightA[index] = counter;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////
void ultrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 40000);
  float rawDistance = (duration * 0.0343) / 2;  // Distance in cm with decimals

  // Update only the selected tank based on flagHold
  int tankIndex = -1;
  if (flagMod == 1 && flagNext == 0) tankIndex = 0;       // Tank A
  else if (flagMod == 1 && flagNext == 1) tankIndex = 1;  // Tank B
  else if (flagMod == 1 && flagNext == 2) tankIndex = 2;  // Tank C
  else if (flagMod == 1 && flagNext == 3) tankIndex = 0;  // Default to Tank A

  if (tankIndex != -1) {
    if (rawDistance <= 0 || rawDistance > tank_HeightRead[tankIndex]) {
      distance[tankIndex] = lastValidDistance[tankIndex];
    } else {
      distance[tankIndex] = rawDistance;
      lastValidDistance[tankIndex] = rawDistance;
    }

    if (distance[tankIndex] <= 20) {
      heightTank[tankIndex] = tank_HeightRead[tankIndex];  // Tank full
    } else {
      heightTank[tankIndex] = tank_HeightRead[tankIndex] - distance[tankIndex];  // Water level
    }

    // Update estimated values for the selected tank
    if (tankIndex == 0) {
      estimatedWeightA = interpolateHeightToWeightA(heightTank[0]);
      estimatedHeightA = interpolateWeightToHeightA(estimatedWeightA);
      estimatedLitre[0] = calculateLitersFromWeight(estimatedWeightA);
      percentage[0] = calculateTankPercentage(heightTank[0], tank_HeightRead[0]);
      // Send to Blynk with 2 decimal places
      Blynk.virtualWrite(V0, String(percentage[0], 2));      // Percentage Tank A
      Blynk.virtualWrite(V3, String(estimatedHeightA, 2));   // Height Tank A
      Blynk.virtualWrite(V4, String(estimatedWeightA, 2));   // Weight Tank A
      Blynk.virtualWrite(V5, String(estimatedLitre[0], 2));  // Liters Tank A
    } else if (tankIndex == 1) {
      estimatedWeightB = interpolateHeightToWeightB(heightTank[1]);
      estimatedHeightB = interpolateWeightToHeightB(estimatedWeightB);
      estimatedLitre[1] = calculateLitersFromWeight(estimatedWeightB);
      percentage[1] = calculateTankPercentage(heightTank[1], tank_HeightRead[1]);
      // Send to Blynk with 2 decimal places
      Blynk.virtualWrite(V1, String(percentage[1], 2));      // Percentage Tank B
      Blynk.virtualWrite(V6, String(estimatedHeightB, 2));   // Height Tank B
      Blynk.virtualWrite(V7, String(estimatedWeightB, 2));   // Weight Tank B
      Blynk.virtualWrite(V8, String(estimatedLitre[1], 2));  // Liters Tank B
    } else if (tankIndex == 2) {
      estimatedWeightC = interpolateHeightToWeightC(heightTank[2]);
      estimatedHeightC = interpolateWeightToHeightC(estimatedWeightC);
      estimatedLitre[2] = calculateLitersFromWeight(estimatedWeightC);
      percentage[2] = calculateTankPercentage(heightTank[2], tank_HeightRead[2]);
      // Send to Blynk with 2 decimal places
      Blynk.virtualWrite(V2, String(percentage[2], 2));       // Percentage Tank C
      Blynk.virtualWrite(V9, String(estimatedHeightC, 2));    // Height Tank C
      Blynk.virtualWrite(V10, String(estimatedWeightC, 2));   // Weight Tank C
      Blynk.virtualWrite(V11, String(estimatedLitre[2], 2));  // Liters Tank C
    }
  }

  Serial.print("Estimated Weight Tank A: ");
  Serial.print(estimatedWeightA);  //TANK A DISPLAY ESTIMATED READING VALUE
  Serial.println(" lbs");
  Serial.print("Estimated Height Tank A: ");
  Serial.print(estimatedHeightA);
  Serial.println("cm");

  Serial.print("Estimated Weight Tank B: ");
  Serial.print(estimatedWeightB);  //TANK B DISPLAY ESTIMATED READING VALUE
  Serial.println(" lbs");
  Serial.print("Estimated Height Tank B: ");
  Serial.print(estimatedHeightB);
  Serial.println("cm");

  Serial.print("Estimated Weight Tank C: ");
  Serial.print(estimatedWeightC);  //TANK C DISPLAY ESTIMATED READING VALUE
  Serial.println(" lbs");
  Serial.print("Estimated Height Tank C: ");
  Serial.print(estimatedHeightC);
  Serial.println("cm");
  Serial.println(" ");

  Serial.print("Height Tank A: ");
  Serial.print(heightTank[0]);
  Serial.println(" cm");
  Serial.print("Height Tank B: ");
  Serial.print(heightTank[1]);
  Serial.println(" cm");
  Serial.print("Height Tank C: ");
  Serial.print(heightTank[2]);
  Serial.println(" cm");
  Serial.println(" ");

  // Ensure water level is within valid range
  //if (heightTank[0] > tank_HeightRead[0]) heightTank[0] = tank_HeightRead[0];
  //if (heightTank[0] < 0) tank_HeightRead[0] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////
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
    } else {
      flagMod = 2;
    }

    if (pressVarDuration < longPressVarDuration) {
      if (modMenu == 1) flagMod = 1;
      else if (modMenu == 2) flagMod = 0;
    }

    varDataSetpress = 0;
  }
}

//////////////////////////////////////////////////////////////////////////////////////
void holdButton() {
  static unsigned long pressStartTime = 0;       // Time when button was pressed
  const unsigned long longPressDuration = 1000;  // 1 second threshold for long press

  // Check if button is pressed (LOW)
  if (digitalRead(HOLD) == LOW && pressStartTime == 0) {
    pressStartTime = millis();  // Record the start time of the press
    sound();                    // Play sound when button is first pressed
  }

  // Check if button is released (HIGH) after being pressed
  if (digitalRead(HOLD) == HIGH && pressStartTime != 0) {
    unsigned long pressDuration = millis() - pressStartTime;  // Calculate how long it was held

    if (pressDuration < longPressDuration) {
      // Short press: Increment holdMenu and update flagHold
      holdMenu = (holdMenu + 1) % 4;
      Serial.print("Short press detected, holdMenu: ");
      Serial.println(holdMenu);
    } else {
      // Long press: Set a specific flag or action
      Serial.println("Long press detected!");
      // Example: Set flagHold to a special value (e.g., 5) for long press
      flagHold = 4;  // You can define what this means in your logic
    }

    // Update flagHold based on holdMenu (for short press only)
    if (pressDuration < longPressDuration) {
      if (holdMenu == 1) flagHold = 1;
      else if (holdMenu == 2) flagHold = 2;
      else if (holdMenu == 3) flagHold = 3;
      else if (holdMenu == 0) flagHold = 0;
    }

    pressStartTime = 0;  // Reset the start time
  }
}

//////////////////////////////////////////////////////////////////////////////////////
void nextButton() {
  if (digitalRead(NEXT) == LOW) {
    delay(100);
    nextMenu = (nextMenu + 1) % 4;
    sound();
    saveToNVS();

    while (digitalRead(NEXT) == LOW);
  }

  if (nextMenu == 1) flagNext = 1;
  else if (nextMenu == 2) flagNext = 2;
  else if (nextMenu == 3) flagNext = 3;
  else if (nextMenu == 0) flagNext = 0;
}

//////////////////////////////////////////////////////////////////////////////////////
void varSelectButton() {
  if (digitalRead(VAR) == LOW) {
    delay(100);
    varMenu = (varMenu + 1) % 40;
    sound();
    saveVarDataSetA();
    while (digitalRead(VAR) == LOW);
  }

  flagVar = varMenu;
}

//////////////////////////////////////////////////////////////////////////////////////
void varDisplaySetA() {
  if (flagMod == 2 && flagHold == 2 && flagNext == 3) {
    if (flagVar >= 0 && flagVar <= 39)  // Assuming 0-39 for 20 heights + 20 weights
    {
      int index = flagVar / 2;  // Map flagVar to array index (0-19)

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);

      // Determine tank letter (A or B) and index
      int tankIndex = (flagVar / 2) + 1;             // 1-20 for display purposes
      char tankLetter = (flagVar < 40) ? 'A' : 'B';  // A for 0-19, B for 20-39

      display.setCursor(0, 0);
      display.print("Set Var Tank ");
      display.print(tankLetter);
      display.print(": ");
      display.print(tankIndex);

      display.setCursor(0, 16);
      display.print("Height: ");
      if (flagVar < 40) display.print(varSetHeightA[index]);

      display.setCursor(0, 32);
      display.print("Lbs: ");
      if (flagVar < 40) display.print(varSetWeightA[index]);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////
void sound() {
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
}

//////////////////////////////////////////////////////////////////////////////////////
void connectToBlynk() {
  unsigned long start = millis();
  while (!Blynk.connected() != WL_CONNECTED && millis() - start < 1000)  // Attempt connection
  {                                                                      // Try for 10 seconds
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

//////////////////////////////////////////////////////////////////////////////////////
void updateDisplay() {
  bool mainMenu = true;

  if (flagMod == 1) mainMenu = false;
  else if (flagMod == 0) mainMenu = true;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Main Menu (Mode Selection)
  if (mainMenu == true) {
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

  // Tank Setup and Monitoring Mode
  if (flagMod == 1 && flagHold == 1) {
    // Set Height Menu
    display.setCursor(0, 0);
    display.print("Set Height: ");

    display.setCursor(0, 16);
    if (flagNext == 1) display.print("> ");
    display.print("Tank A: ");
    display.print(tank_HeightRead[0]);
    display.print(" cm");

    display.setCursor(0, 32);
    if (flagNext == 2) display.print("> ");
    display.print("Tank B: ");
    display.print(tank_HeightRead[1]);
    display.print(" cm");

    display.setCursor(0, 48);
    if (flagNext == 3) display.print("> ");
    display.print("Tank C: ");
    display.print(tank_HeightRead[2]);
    display.print(" cm");
  }

  if (flagMod == 1 && flagHold == 2) {  // Set Data Menu
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

  if (flagMod == 1 && flagHold == 3) {  // Tank Display
    if (flagNext == 0) {                // Tank A
      display.setCursor(0, 0);
      display.print("Tank A: ");
      display.print(percentage[0], 2);
      display.print(" %");

      display.setCursor(0, 16);
      display.print("Height: ");
      display.print(estimatedHeightA, 2);
      display.print(" cm");

      display.setCursor(0, 32);
      display.print("Lbs: ");
      display.print(estimatedWeightA, 2);
      display.print(" lbs");

      display.setCursor(0, 48);
      display.print("Litre: ");
      display.print(estimatedLitre[0], 2);
      display.print(" L");
    }

    else if (flagNext == 1) {  // Tank B
      display.setCursor(0, 0);
      display.print("Tank B: ");
      display.print(percentage[1], 2);
      display.print(" %");

      display.setCursor(0, 16);
      display.print("Height: ");
      display.print(estimatedHeightB, 2);
      display.print(" cm");

      display.setCursor(0, 32);
      display.print("Lbs: ");
      display.print(estimatedWeightB, 2);
      display.print(" lbs");

      display.setCursor(0, 48);
      display.print("Litre: ");
      display.print(estimatedLitre[1], 2);
      display.print(" L");
    }

    else if (flagNext == 2) {  // Tank C
      display.setCursor(0, 0);
      display.print("Tank C: ");
      display.print(percentage[2], 2);
      display.print(" %");

      display.setCursor(0, 16);
      display.print("Height: ");
      display.print(estimatedHeightC, 2);
      display.print(" cm");

      display.setCursor(0, 32);
      display.print("Lbs: ");
      display.print(estimatedWeightC, 2);
      display.print(" lbs");

      display.setCursor(0, 48);
      display.print("Litre: ");
      display.print(estimatedLitre[2], 2);
      display.print(" L");
    }

    else if (flagNext == 3) {  // Back
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.print("<--");
    }
  }

  else if (flagMod == 2)
  {
    varDisplaySetA()
  }
  display.display();
}

//////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);

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

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);

  pinMode(MOD, INPUT_PULLUP);
  pinMode(HOLD, INPUT_PULLUP);
  pinMode(NEXT, INPUT_PULLUP);
  pinMode(VAR, INPUT_PULLUP);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Read the initial state of CLK
  lastStateCLK = digitalRead(CLK);

  readFromNVS();
  readVarDataSetA();
  display.clearDisplay();
}

//////////////////////////////////////////////////////////////////////////////////////
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

  updateDisplay();

  Serial.print("MOD: ");
  Serial.print(modMenu);
  Serial.print(" | HOLD: ");
  Serial.print(holdMenu);
  Serial.print(" | NEXT: ");
  Serial.print(nextMenu);
  Serial.print(" | VAR: ");
  Serial.println(varMenu);
  Serial.println("");

  ///////////////////////////////////////
  //  SAVE SET VALUE
  Serial.println("Loaded Tanks Values:");
  Serial.print("A: Height: ");
  Serial.println(tank_HeightRead[0]);
  Serial.print("B: Height: ");
  Serial.println(tank_HeightRead[1]);
  Serial.print("C: Height: ");
  Serial.println(tank_HeightRead[2]);
  Serial.println(" ");
  ////////////////////////////////////////

  if (flagHold == 4) {
    ultrasonic();
  }

  delay(10);
}

//CALCULATION
////////////////////////////////////////TANK A//////////////////////////////////////////////
// Linear interpolation function
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

// Function to estimate height based on liquid weight
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

////////////////////////////////////////TANK B//////////////////////////////////////////////
// Linear interpolation function
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

// Function to estimate height based on liquid weight
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

////////////////////////////////////////TANK C//////////////////////////////////////////////
// Linear interpolation function
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

// Function to estimate height based on liquid weight

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

//////////////////////////////////////////////////////////////////////////////////////
//Litre calculation
float calculateLitersFromWeight(float weight_lbs) {
  const float density_lbs_per_liter = 2.2;
  return weight_lbs / density_lbs_per_liter;
}

//////////////////////////////////////////////////////////////////////////////////////
//Percentage calculation
float calculateTankPercentage(float currentHeight, float totalHeight) {
  if (totalHeight <= 0) return 0.0;  // Avoid division by zero
  float percentage = (currentHeight / totalHeight) * 100.0;
  if (percentage < 0) return 0.0;      // Clamp to 0% if negative
  if (percentage > 100) return 100.0;  // Clamp to 100% if overfull
  return percentage;
}
