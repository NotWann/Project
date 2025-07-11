#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create SSD1306 display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define MOD 27
#define HOLD 32
#define NEXT 33

#define TRIG_PIN 12
#define ECHO_PIN 14

const int potPin = 36;

int modMenu = 0, holdMenu = 0, nextMenu = 0;
int flagMod = 0, flagHold = 0, flagNext = 0;

int setWaterLevel
float setLBS, setLiter;

float smoothWaterLevel = 0, smoothLBS = 0, smoothLiter = 0;
float alpha = 0.1;  // Smoothing factor (lower = smoother)

float readSmoothPot(int pin) {
    return analogRead(pin) / 4095.0;  // Normalize ADC value (0 to 1)
}

void updateValues() {
    float potValue = readSmoothPot(potPin);

    if (flagNext == 1) {
        smoothLBS = (alpha * (potValue * 7.3)) + ((1 - alpha) * smoothLBS);
        setLBS = smoothLBS;
    } 
    else if (flagNext == 2) {
        smoothLiter = (alpha * (potValue * 0.2)) + ((1 - alpha) * smoothLiter);
        setLiter = smoothLiter;
    }
    else if (flagNext == 3) {
        smoothWaterLevel = (alpha * (potValue * 100)) + ((1 - alpha) * smoothWaterLevel);
        setWaterLevel = smoothWaterLevel;
    }
}


void modButton()
{
  if (digitalRead(MOD) == LOW)
  {
    delay(100);
    modMenu = (modMenu + 1) % 2;
    while(digitalRead(MOD) == LOW);
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

void holdButton()
{
  if (digitalRead(HOLD) == LOW)
  {
    delay(100);
    holdMenu = (holdMenu + 1) % 2;
    while(digitalRead(HOLD) == LOW);
  }

  if (holdMenu == 1)
  {
    flagHold = 1;
  }
  else if (holdMenu == 0)
  {
    flagHold = 0;
  }
}

void nextButton()
{
  if (digitalRead(NEXT) == LOW)
  {
    delay(100);
    nextMenu = (nextMenu + 1) % 3;
    while(digitalRead(NEXT) == LOW);
  }

  if (nextMenu == 1)
  {
    flagNext = 1;
  }
  else if (nextMenu == 2)
  {
    flagNext = 2;
  }
  else if (nextMenu == 0)
  {
    flagNext = 0;
  }
}

// Store float values in EEPROM
void saveToEEPROM() {
    EEPROM.put(0, setLBS);
    EEPROM.put(4, setLiter);
    EEPROM.put(8, setWaterLevel);
    EEPROM.commit();
}

// Read float values from EEPROM
void loadFromEEPROM() {
    EEPROM.get(0, setLBS);
    EEPROM.get(4, setLiter);
    EEPROM.get(8, setWaterLevel);
}

void setup()
{
  Serial.begin(115200);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // 0x3C is the I2C address
    Serial.println("SSD1306 allocation failed");
    while (1);
  }

  loadFromEEPROM();  // Load saved values

  pinMode(MOD,  INPUT_PULLUP);
  pinMode(HOLD,  INPUT_PULLUP);
  pinMode(NEXT,  INPUT_PULLUP);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  display.clearDisplay();
}

void loop()
{
  modButton();
  holdButton();
  nextButton();

  updateValues();

  long duration;
  float distance;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration * 0.0343) / 2;
  smoothWaterLevel = (alpha * distance) + ((1 - alpha) * smoothWaterLevel);
  setWaterLevel = smoothWaterLevel;

  display.clearDisplay();

  //  Main display
  if (modMenu == 0)
  {
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.print("Water Level: ");
    display.print(distance);

    display.setCursor(0, 16);
    display.print("LBS: ");
    display.print(LBS);

    display.setCursor(0, 32);
    display.print("Liter: ");
    display.print(Liter);

    display.setCursor(0, 48);
    display.print("Percentage: ");
  }

  //  Set Mode display
  if (modMenu == 1)
{
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.print("Set LBS: ");
    display.print(setLBS);

    display.setCursor(0, 16);
    display.print("Set Liter: ");
    display.print(setLiter);

}


  display.display();

  Serial.print("MOD: ");
  Serial.print(modMenu);
  Serial.print(" | HOLD: ");
  Serial.print(holdMenu);
  Serial.print(" | NEXT: ");
  Serial.println(nextMenu);
  

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  Serial.print("Set LBS: ");
  Serial.println(setLBS);

  delay(100);

}



