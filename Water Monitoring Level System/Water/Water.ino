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

#define MOD 27    // Hijau
#define NEXT 26   //Kuning
#define HOLD 25   //Merah


#define TRIG_PIN 32
#define ECHO_PIN 33

char auth[] = "RfX6B_kVE8_zhTCCwblgNW2rbDEzb4ry";
char ssid[] = "abcd";
char pass[] = "123456789";

char server[] = "prakitblog.com";
int port = 8181;

const int potPin = 35;
const int buzzerPin = 2;

////////////////////////////////////////////////////////////////////////////////////////
int modMenu = 0, holdMenu = 0, nextMenu = 0;
int flagMod = 0, flagHold = 0, flagNext = 0;

////////////////////////////////////////////////////////////////////////////////////////
// Sample data from your table (Pounds to Height in cm)
float weightData[] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000};
float heightData[] = {0, 7.3, 14.5, 21.1, 27.2, 32.5, 37.4, 42.4, 47.5, 52.8, 58, 62.9, 68.3, 73.5, 79.2, 85, 91.3, 97.3, 101.3};
int dataSize = sizeof(weightData) / sizeof(weightData[0]);


// Read
int tankA_HeightRead[3] = {0, 0, 0};
float tankA_LbsRead[3] = {0, 0, 0};
float tankA_LitreRead[3] = {0, 0, 0};

//set pot value
int smoothA_Height[3] = {0, 0, 0};
float smoothA_Lbs[3] = {0, 0, 0};
float smoothA_Litre[3] {0, 0, 0};

float alpha = 0.1;

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
  preferences.begin("tankA", false); // Open NVS storage with namespace "tankA"
    
  preferences.putFloat("heightA", tankA_HeightRead[0]);
  preferences.putFloat("lbsA", tankA_LbsRead[0]);
  preferences.putFloat("litreA", tankA_LitreRead[0]);

  preferences.putFloat("heightB", tankA_HeightRead[1]);
  preferences.putFloat("lbsB", tankA_LbsRead[1]);
  preferences.putFloat("litreB", tankA_LitreRead[1]);

  preferences.putFloat("heightC", tankA_HeightRead[2]);
  preferences.putFloat("lbsC", tankA_LbsRead[2]);
  preferences.putFloat("litreC", tankA_LitreRead[2]);
    
  preferences.end(); // Close NVS storage
}

// Function to read data from NVS
void readFromNVS()
{
  preferences.begin("tankA", true); // Open NVS storage in read mode
    
  tankA_HeightRead[0] = preferences.getFloat("heightA", 0.0);
  tankA_LbsRead[0] = preferences.getFloat("lbsA", 0.0);
  tankA_LitreRead[0] = preferences.getFloat("litreA", 0.0);

  tankA_HeightRead[1] = preferences.getFloat("heightB", 0.0);
  tankA_LbsRead[1] = preferences.getFloat("lbsB", 0.0);
  tankA_LitreRead[1] = preferences.getFloat("litreB", 0.0);

  tankA_HeightRead[2] = preferences.getFloat("heightC", 0.0);
  tankA_LbsRead[2] = preferences.getFloat("lbsC", 0.0);
  tankA_LitreRead[2] = preferences.getFloat("litreC", 0.0);
    
  preferences.end(); // Close NVS storage

}

////////////////////////////////////////////////////////////////////////////////////////
float readSmoothPot(int pin) 
{
  return analogRead(pin) / 4095.0;  // Normalize ADC value (0 to 1)
}

void updateValues()
{
  float potValue = readSmoothPot(potPin);

  if (flagHold == 0 && flagMod == 1)
  {
    if (flagNext == 1)
    {
      smoothA_Height[0] = (alpha * (potValue * 1000)) + ((1 - alpha) * smoothA_Height[0]);
      tankA_HeightRead[0] = smoothA_Height[0];
    }
    else if (flagNext == 2)
    {
      smoothA_Lbs[0] = (alpha * (potValue * 3000)) + ((1 - alpha) * smoothA_Lbs[0]);
      tankA_LbsRead[0] = smoothA_Lbs[0];
    }
    else if (flagNext == 3)
    {
      smoothA_Litre[0] = (alpha * (potValue * 1000)) + ((1 - alpha) * smoothA_Litre[0]);
      tankA_LitreRead[0] = smoothA_Litre[0];
    }
  }

  if (flagHold == 1 && flagMod == 1)
  {
    if (flagNext == 1)
    {
      smoothA_Height[1] = (alpha * (potValue * 1000)) + ((1 - alpha) * smoothA_Height[1]);
      tankA_HeightRead[1] = smoothA_Height[1];
    }
    else if (flagNext == 2)
    {
      smoothA_Lbs[1] = (alpha * (potValue * 3000)) + ((1 - alpha) * smoothA_Lbs[1]);
      tankA_LbsRead[1] = smoothA_Lbs[1];
    }
    else if (flagNext == 3)
    {
      smoothA_Litre[1] = (alpha * (potValue * 1000)) + ((1 - alpha) * smoothA_Litre[1]);
      tankA_LitreRead[1] = smoothA_Litre[1];
    }
  }

  if (flagHold == 2 && flagMod == 1)
  {
    if (flagNext == 1)
    {
      smoothA_Height[2] = (alpha * (potValue * 1000)) + ((1 - alpha) * smoothA_Height[2]);
      tankA_HeightRead[2] = smoothA_Height[2];
    }
    else if (flagNext == 2)
    {
      smoothA_Lbs[2] = (alpha * (potValue * 3000)) + ((1 - alpha) * smoothA_Lbs[2]);
      tankA_LbsRead[2] = smoothA_Lbs[2];
    }
    else if (flagNext == 3)
    {
      smoothA_Litre[2] = (alpha * (potValue * 1000)) + ((1 - alpha) * smoothA_Litre[2]);
      tankA_LitreRead[2] = smoothA_Litre[2];
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

  //float lbsA = tankA_LbsRead[0];
  
  //heightTank[0] = 0.5626 + (0.01444 * lbsA) - (8.82e-7 * lbsA * lbsA) + (5.99e-11 * lbsA * lbsA * lbsA);
  
  
  
  if (distance <= 0 || distance > tankA_HeightRead[0])
  {
    distance = lastValidDistance;
  }
  else  
  {
    lastValidDistance = distance;
  }

  //Blind area adjustment
  /*if (distance <= 20) 
  {
    heightTank[0] = tankA_HeightRead[0];   // Assume the tank is full
  }
  else 
  {*/
    heightTank[0] = tankA_HeightRead[0] - distance;    // Adjusted water level
  //}

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
  //if (heightTank[0] > tankA_HeightRead[0]) heightTank[0] = tankA_HeightRead[0];
  //if (heightTank[0] < 0) tankA_HeightRead[0] = 0;
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

  pinMode(MOD, INPUT_PULLUP);
  pinMode(HOLD, INPUT_PULLUP);
  pinMode(NEXT, INPUT_PULLUP);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

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

  updateValues();

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
      display.print(tankA_HeightRead[0]);

      display.setCursor(0, 32);
      display.print("Lbs: ");
      display.print(tankA_LbsRead[0]);

      display.setCursor(0, 48);
      display.print("Litre: ");
      display.print(tankA_LitreRead[0]);   //baca yang simpan
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
      display.print(tankA_HeightRead[1]);

      display.setCursor(0, 32);
      display.print("Lbs: ");
      display.print(tankA_LbsRead[1]);

      display.setCursor(0, 48);
      display.print("Litre: ");
      display.print(tankA_LitreRead[1]);   // baca yang simpan
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
      display.print(tankA_HeightRead[2]);

      display.setCursor(0, 32);
      display.print("Lbs: ");
      display.print(tankA_LbsRead[2]);

      display.setCursor(0, 48);
      display.print("Litre: ");
      display.print(tankA_LitreRead[2]);   // baca yang simpan
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
  Serial.print(tankA_HeightRead[0]);
  Serial.print(" | Lbs: "); 
  Serial.print(tankA_LbsRead[0]);
  Serial.print(" | Litre: "); 
  Serial.println(tankA_LitreRead[0]);

  Serial.println("Loaded Tank B Values:");
  Serial.print("Height: "); 
  Serial.print(tankA_HeightRead[1]);
  Serial.print(" | Lbs: "); 
  Serial.print(tankA_LbsRead[1]);
  Serial.print(" | Litre: "); 
  Serial.println(tankA_LitreRead[1]);

  Serial.println("Loaded Tank C Values:");
  Serial.print("Height: "); 
  Serial.print(tankA_HeightRead[2]);
  Serial.print(" | Lbs: "); 
  Serial.print(tankA_LbsRead[2]);
  Serial.print(" | Litre: "); 
  Serial.println(tankA_LitreRead[2]);
  Serial.println(" ");

  ultrasonic();

  Serial.print("Distance: ");
  Serial.println(distance);


  delay(100);
}

// Linear interpolation function
float interpolateHeightToWeight(float h) {
    if (h <= heightData[0]) return weightData[0];
    if (h >= heightData[dataSize - 1]) return weightData[dataSize - 1];

    for (int i = 0; i < dataSize - 1; i++) {
        if (h >= heightData[i] && h <= heightData[i + 1]) {
            return weightData[i] + (h - heightData[i]) * (weightData[i + 1] - weightData[i]) / (heightData[i + 1] - heightData[i]);
        }
    }
    return 0;
}

// Function to estimate height based on liquid weight
float interpolateWeightToHeight(float w) {
    if (w <= weightData[0]) return heightData[0];
    if (w >= weightData[dataSize - 1]) return heightData[dataSize - 1];

    for (int i = 0; i < dataSize - 1; i++) {
        if (w >= weightData[i] && w <= weightData[i + 1]) {
            return heightData[i] + (w - weightData[i]) * (heightData[i + 1] - heightData[i]) / (weightData[i + 1] - weightData[i]);
        }
    }
    return 0;
}








