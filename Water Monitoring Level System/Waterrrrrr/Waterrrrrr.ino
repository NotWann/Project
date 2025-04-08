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

//////////////////////////////////////////////////////////////////////////////////////
int modMenu = 0, holdMenu = 0, nextMenu = 0;
int flagMod = 0, flagHold = 0, flagNext = 0;

//////////////////////////////////////////////////////////////////////////////////////
// Sample data from your table (Pounds to Height in cm)
float weightData[] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000};
float heightData[] = {0, 7.3, 14.5, 21.1, 27.2, 32.5, 37.4, 42.4, 47.5, 52.8, 58, 62.9, 68.3, 73.5, 79.2, 85, 91.3, 97.3, 101.3};
int dataSize = sizeof(weightData) / sizeof(weightData[0]);

//Variable data set
float varWeightA[19] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float varHeightA[19] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
}

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
// Linear interpolation function
float interpolateHeightToWeight(float h) 
{
  if (h <= heightData[0]) return weightData[0];
  if (h >= heightData[dataSize - 1]) return weightData[dataSize - 1];

  for (int i = 0; i < dataSize - 1; i++) 
  {
    if (h >= heightData[i] && h <= heightData[i + 1]) 
    {
      return weightData[i] + (h - heightData[i]) * (weightData[i + 1] - weightData[i]) / (heightData[i + 1] - heightData[i]);
      }
    }
  return 0;
}

// Function to estimate height based on liquid weight
float interpolateWeightToHeight(float w) 
{
  if (w <= weightData[0]) return heightData[0];
  if (w >= weightData[dataSize - 1]) return heightData[dataSize - 1];

  for (int i = 0; i < dataSize - 1; i++) 
  {
    if (w >= weightData[i] && w <= weightData[i + 1]) 
    {
      return heightData[i] + (w - weightData[i]) * (heightData[i + 1] - heightData[i]) / (weightData[i + 1] - weightData[i]);
      }
    }
  return 0;
}

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

void saveVarDataSet()
{
  preferences.begin("varA", true);

  





}







