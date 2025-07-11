#define BLYNK_PRINT Serial

#include <Wire.heightTankA>
#include <WiFi.heightTankA>
#include <Adafruit_GFX.heightTankA>
#include <Adafruit_SSD1306.heightTankA>
#include <Preferences.heightTankA>
#include <BlynkSimpleEsp32.heightTankA>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create SSD1306 display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Preferences preferences;

#define MOD 27    // Hijau
#define NEXT 26   //Kuning
#define HOLD 25   //Merah

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

int lockPoint;

////////////////////////////////////////////////////////////////////////////////////////
int modMenu = 0, holdMenu = 0, nextMenu = 0;
int flagMod = 0, flagHold = 0, flagNext = 0;

////////////////////////////////////////////////////////////////////////////////////////
// Sample data from your table (Pounds to Height in cm)
//Tank A
float weightDataA[] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000};
float heightDataA[] = {0, 7.3, 14.5, 21.1, 27.2, 32.5, 37.4, 42.4, 47.5, 52.8, 58, 62.9, 68.3, 73.5, 79.2, 85, 91.3, 97.3, 101.3};
int dataSizeA = sizeof(weightDataA) / sizeof(weightDataA[0]);

//Tank B
float weightDataB[] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000};
float heightDataB[] = {0, 3.8, 8.2, 11.5, 14, 16.3, 18.5, 20.7, 23, 25.2, 27.4, 29.8, 32, 34.6, 36.9, 39.5, 42.3, 45.5, 47.1, 50.2};
int dataSizeB = sizeof(weightDataB) / sizeof(weightDataB[0]);

//Tank C
float weightDataC[] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000};
float heightDataC[] = {0, 5.6, 10.2, 14.7, 18.8, 23, 26.7, 30.8, 34.9, 38.9, 43, 47.1, 51.8, 55.5, 59.6, 63.8, 67.8, 70.2};
int dataSizeC = sizeof(weightDataC) / sizeof(weightDataC[0]);

////////////////////////////////////////////////////////////////////////////////////////
// Read
int tank_HeightRead[3] = {0, 0, 0};
float tank_LbsRead[3] = {0, 0, 0};
float tank_LitreRead[3] = {0, 0, 0};

//set pot value
int set_Height[3] = {0, 0, 0};
float set_Lbs[3] = {0, 0, 0};
float set_Litre[3] {0, 0, 0};

////////////////////////////////////////////////////////////////////////////////////////
float distance, lastValidDistance;

int heightTank[3] = {0, 0, 0};
int percentageTank[3] = {0, 0, 0};
float lbsTank[3] = {0, 0, 0};
float litreTank[3] = {0, 0, 0};

////////////////////////////////////////////////////////////////////////////////////////
// Function to save data to NVS
void saveToNVS() 
{
  preferences.begin("tankSet", false); // Open NVS storage with namespace "tankA"
    
  preferences.putFloat("heightA", tank_HeightRead[0]);
  preferences.putFloat("lbsA", tank_LbsRead[0]);
  preferences.putFloat("litreA", tank_LitreRead[0]);

  preferences.putFloat("heightB", tank_HeightRead[1]);
  preferences.putFloat("lbsB", tank_LbsRead[1]);
  preferences.putFloat("litreB", tank_LitreRead[1]);

  preferences.putFloat("heightC", tank_HeightRead[2]);
  preferences.putFloat("lbsC", tank_LbsRead[2]);
  preferences.putFloat("litreC", tank_LitreRead[2]);
    
  preferences.end(); // Close NVS storage
}

// Function to read data from NVS
void readFromNVS()
{
  preferences.begin("tankSet", true); // Open NVS storage in read mode
    
  tank_HeightRead[0] = preferences.getFloat("heightA", 0.0);
  tank_LbsRead[0] = preferences.getFloat("lbsA", 0.0);
  tank_LitreRead[0] = preferences.getFloat("litreA", 0.0);

  tank_HeightRead[1] = preferences.getFloat("heightB", 0.0);
  tank_LbsRead[1] = preferences.getFloat("lbsB", 0.0);
  tank_LitreRead[1] = preferences.getFloat("litreB", 0.0);

  tank_HeightRead[2] = preferences.getFloat("heightC", 0.0);
  tank_LbsRead[2] = preferences.getFloat("lbsC", 0.0);
  tank_LitreRead[2] = preferences.getFloat("litreC", 0.0);
    
  preferences.end(); // Close NVS storage

}

////////////////////////////////////////////////////////////////////////////////////////
void encoder()
{
  currentStateCLK = digitalRead(CLK);

  if (currentStateCLK == 1 && lastStateCLK == 0) 
  {
     if ((digitalRead(DT) == 1 && currentStateCLK == 1) && lockPoint == 0) 
    {
      counter++;
    } 
    if ((digitalRead(DT) == 1 && currentStateCLK == 1) && lockPoint == 1)
    {
      counter--;
    }
    Serial.print("Counter: ");
    Serial.println(counter);

    updateValues();
  }
  lastStateCLK = currentStateCLK;

  int btnState = digitalRead(SW);

  if (btnState == LOW) 
  {
    if (millis() - lastButtonPress > 50) 
    {
      Serial.println("Button pressed!");
      lockPoint = (lockPoint + 1) % 2; 
    }
    lastButtonPress = millis();
  }
  delay(1);
}

////////////////////////////////////////////////////////////////////////////////////////
void updateValues()
{
  if (flagHold == 0 && flagMod == 1) 
  {
    if (flagNext == 1) 
    {
      set_Height[0] = counter;
      tank_HeightRead[0] = set_Height[0];
    }
            
    else if (flagNext == 2) 
    {
      set_Lbs[0] = counter;
      tank_LbsRead[0] = set_Lbs[0];
    }
            
    else if (flagNext == 3) 
    {
      set_Litre[0] = counter;
      tank_LitreRead[0] = set_Litre[0];
    }        
  }

  else if (flagHold == 1 && flagMod == 1) 
  {
    if (flagNext == 1) 
    {
      set_Height[1] = counter;
      tank_HeightRead[1] = set_Height[1];
    }
            
    else if (flagNext == 2) 
    {
      set_Lbs[1] = counter;
      tank_LbsRead[1] = set_Lbs[1];
    }
            
    else if (flagNext == 3) 
    {
      set_Litre[1] = counter;
      tank_LitreRead[1] = set_Litre[1];
    }      
  }

  else if (flagHold == 2 && flagMod == 1) 
  {
    if (flagNext == 1) 
    {
      set_Height[2] = counter;
      tank_HeightRead[2] = set_Height[2];
    }
            
    else if (flagNext == 2) 
    {
      set_Lbs[2] = counter;
      tank_LbsRead[2] = set_Lbs[2];
    }
            
    else if (flagNext == 3) 
    {
      set_Litre[2] = counter;
      tank_LitreRead[2] = set_Litre[2];
    } 
  }
}

////////////////////////////////////////////////////////////////////////////////////////
void ultrasonic() 
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 40000);
  distance = (duration * 0.0343) / 2;

  if (distance <= 0 || distance > tank_HeightRead[0])
  {
    distance = lastValidDistance;
  }
  else  
  {
    lastValidDistance = distance;
  }

  //Blind area adjustment
  if (distance <= 20) 
  {
    heightTank[0] = tank_HeightRead[0];   // Assume the tank is full
  }
  else 
  {
    heightTank[0] = tank_HeightRead[0] - distance;    // Adjusted water level
  }

  float estimatedWeight = interpolateHeightToWeight(heightTank[0]);

  float estimatedHeight = interpolateWeightToHeight(estimatedWeight);

  Serial.print("Height Tank A: ");
  Serial.print(heightTank[0]);
  Serial.println(" cm");
  Serial.print("Weight Tank A: ");
  Serial.print(estimatedWeight);
  Serial.println(" lbs");
  Serial.print("Estimated Height Tank A: ");
  Serial.print(estimatedHeight);
  Serial.println( "cm");

  // Ensure water level is within valid range
  //if (heightTank[0] > tank_HeightRead[0]) heightTank[0] = tank_HeightRead[0];
  //if (heightTank[0] < 0) tank_HeightRead[0] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////
void modButton() 
{
  if (digitalRead(MOD) == LOW) 
  {
    delay(100);
    modMenu = (modMenu + 1) % 2;
    sound();
    while (digitalRead(MOD) == LOW);
  }

  if (modMenu == 1)
  {
    flagMod = 1;
  } 
  else if (modMenu == 0)
  {
    flagMod = 0;
  }
}

//////////////////////////////////////////////////////////////////////////////////////
void holdButton() 
{
  if (digitalRead(HOLD) == LOW) 
  {
    delay(100);
    holdMenu = (holdMenu + 1) % 5;
    sound();
    while (digitalRead(HOLD) == LOW);
  }

  if (holdMenu == 1) 
  {
    flagHold = 1;
  } 
  else if (holdMenu == 2) 
  {
    flagHold = 2;
  }
  else if (holdMenu == 3) 
  {
    flagHold = 3;
  }
  else if (holdMenu == 4) 
  {
    flagHold = 4;
  }
  else if (holdMenu == 0) 
  {
    flagHold = 0;
  }
}

//////////////////////////////////////////////////////////////////////////////////////
void nextButton() 
{
  if (digitalRead(NEXT) == LOW) 
  {
    delay(100);
    nextMenu = (nextMenu + 1) % 4;
    sound();
    saveToNVS();
    while (digitalRead(NEXT) == LOW);
  }

  if (nextMenu == 1) 
  {
    flagNext = 1;
  } 
  else if (nextMenu == 2) 
  {
    flagNext = 2;
  } 
  else if (nextMenu == 3) 
  {
    flagNext = 3;
  }
  else if (nextMenu == 0) 
  {
    flagNext = 0;
  }
}

//////////////////////////////////////////////////////////////////////////////////////
void sound() 
{
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
}

//////////////////////////////////////////////////////////////////////////////////////
void connectToBlynk() 
{
  unsigned long start = millis();
  while (!Blynk.connected() != WL_CONNECTED && millis() - start < 1000) // Attempt connection
  {  // Try for 10 seconds
    Blynk.run();
    delay(1000);
  }

  if (Blynk.connected()) 
  {
    Serial.println("Connected!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("WiFi Connected");
    display.display();

    pinMode(buzzerPin, OUTPUT);

    for (int i = 0; i < 2; i++) 
    {
      sound();
    }
  } 
  else 
  {
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
void setup()
{
  Serial.begin(115200);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
  {
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

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Read the initial state of CLK
  lastStateCLK = digitalRead(CLK);

  readFromNVS();
  display.clearDisplay();
}

void loop()
{
  if (!Blynk.connected()) 
  {
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

  display.clearDisplay();

  

// Mode Set:
  if (modMenu == 0) 
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

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
  
    if (holdMenu == 3 || holdMenu == 4)
    {
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
  }


  // Main Display
  if (modMenu == 1 && holdMenu == 4)
  {
    if (nextMenu == 0)
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);

      display.setCursor(0, 0);
      display.print("Tank A: ");

      display.setCursor(0, 16);
      display.print("Height: 000");

      display.setCursor(0, 32);
      display.print("Lbs: 000");

      display.setCursor(0, 48);
      display.print("Litre: 000");
    }

    if (nextMenu == 1)
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);

      display.setCursor(0, 0);
      display.print("Tank B: ");

      display.setCursor(0, 16);
      display.print("Height: 000");

      display.setCursor(0, 32);
      display.print("Lbs: 000");

      display.setCursor(0, 48);
      display.print("Litre: 000");
    }

    if (nextMenu == 2)
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);

      display.setCursor(0, 0);
      display.print("Tank C: ");

      display.setCursor(0, 16);
      display.print("Height: 000");

      display.setCursor(0, 32);
      display.print("Lbs: 000");

      display.setCursor(0, 48);
      display.print("Litre: 000");
    }

  //percentage display
    if (nextMenu == 3)
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);

      display.setCursor(0, 0);
      display.print("Percentage: ");

      display.setCursor(0, 16);
      display.print("Tank A: 30% ");

      display.setCursor(0, 32);
      display.print("Tank B: 50% ");

      display.setCursor(0, 48);
      display.print("Tank C: 100% ");
    }
  }



  

// First
  if (flagHold == 0 && flagMod == 1)
  {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);

      display.setCursor(0, 0);
      display.print("Set Tank A: ");

      display.setCursor(0, 16);
      display.print("Height: ");
      display.print(tank_HeightRead[0]);

      display.setCursor(0, 32);
      display.print("Lbs: ");
      display.print(tank_LbsRead[0]);

      display.setCursor(0, 48);
      display.print("Litre: ");
      display.print(tank_LitreRead[0]);   //baca yang simpan
  }

   if (holdMenu == 1 && flagMod == 1)
  {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);

      display.setCursor(0, 0);
      display.print("Set Tank B:");

      display.setCursor(0, 16);
      display.print("Height: ");
      display.print(tank_HeightRead[1]);

      display.setCursor(0, 32);
      display.print("Lbs: ");
      display.print(tank_LbsRead[1]);

      display.setCursor(0, 48);
      display.print("Litre: ");
      display.print(tank_LitreRead[1]);   // baca yang simpan
  }

   if (holdMenu == 2 && flagMod == 1)
  {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);

      display.setCursor(0, 0);
      display.print("Tank C:");

      display.setCursor(0, 16);
      display.print("Height: ");
      display.print(tank_HeightRead[2]);

      display.setCursor(0, 32);
      display.print("Lbs: ");
      display.print(tank_LbsRead[2]);

      display.setCursor(0, 48);
      display.print("Litre: ");
      display.print(tank_LitreRead[2]);   // baca yang simpan
  }

//////////////////////////////////////////////////////////////////////////////////////
  if (holdMenu == 3 && flagMod == 1)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

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

//////////////////////////////////////////////////////////////////////////////////////

  display.display();

  Serial.print("MOD: ");
  Serial.print(modMenu);
  Serial.print(" | HOLD: ");
  Serial.print(holdMenu);
  Serial.print(" | NEXT: ");
  Serial.println(nextMenu);
  Serial.println("");

  Serial.println("Loaded Tank A Values:");
  Serial.print("Height: "); 
  Serial.print(tank_HeightRead[0]);
  Serial.print(" | Lbs: "); 
  Serial.print(tank_LbsRead[0]);
  Serial.print(" | Litre: "); 
  Serial.println(tank_LitreRead[0]);

  Serial.println("Loaded Tank B Values:");
  Serial.print("Height: "); 
  Serial.print(tank_HeightRead[1]);
  Serial.print(" | Lbs: "); 
  Serial.print(tank_LbsRead[1]);
  Serial.print(" | Litre: "); 
  Serial.println(tank_LitreRead[1]);

  Serial.println("Loaded Tank C Values:");
  Serial.print("Height: "); 
  Serial.print(tank_HeightRead[2]);
  Serial.print(" | Lbs: "); 
  Serial.print(tank_LbsRead[2]);
  Serial.print(" | Litre: "); 
  Serial.println(tank_LitreRead[2]);
  Serial.println(" ");

  ultrasonic();

  Serial.print("Distance: ");
  Serial.println(distance);


  delay(10);
}

////////////////////////////////////////TANK A//////////////////////////////////////////////
// Linear interpolation function
float interpolateHeightToWeight(float heightTankA) {
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
float interpolateWeightToHeight(float weightTankA) {
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
float interpolateHeightToWeight(float heightTankB) {
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
float interpolateWeightToHeight(float weightTankB) {
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
float interpolateHeightToWeight(float heightTankC) {
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
float interpolateWeightToHeight(float weightTankC) {
    if (weightTankC <= weightDataC[0]) return heightDataC[0];
    if (weightTankC >= weightDataC[dataSizeC - 1]) return heightDataC[dataSizeC - 1];

    for (int i = 0; i < dataSizeC - 1; i++) {
        if (weightTankC >= weightDataC[i] && weightTankC <= weightDataC[i + 1]) {
            return heightDataC[i] + (weightTankC - weightDataC[i]) * (heightDataC[i + 1] - heightDataC[i]) / (weightDataC[i + 1] - weightDataC[i]);
        }
    }
    return 0;
}



