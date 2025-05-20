#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 2        // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // Adjust to DHT11 if needed

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

const int buzzerPin = 5;
const int potPin = 1;   // Potentiometer on A1
const int relayFan = 10;
const int relayMist = 11;

int flagBuzzer = 0;
int flag = 0;

float temperature;
int setTarget = 0;

// Variables for smoothing potentiometer readings
const int numReadings = 10;    // Number of readings to average
int readings[numReadings];     // Array to store readings
int readIndex = 0;             // Index of the current reading
int total = 0;                 // Running total
int average = 0;               // The smoothed average

void dhtSensor() {
  delay(10);

  temperature = dht.readTemperature();

  if (isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  if (temperature > 40 && flagBuzzer == 0) {
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(200);
    digitalWrite(buzzerPin, HIGH);
    delay(200);
    digitalWrite(buzzerPin, LOW);

    flagBuzzer = 1;
  } 
  if (temperature < 40 && flagBuzzer == 1) {
    digitalWrite(buzzerPin, LOW);

    flagBuzzer = 0;
  }

  Serial.print("Temperature: "); 
  Serial.print(temperature); 
  Serial.println(" *C");
  Serial.println(" ");
}

void displayOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE); 

  display.setCursor(0, 0);     
  display.print("Temperature: "); 
  display.print(temperature); 
  display.print(" C");
  
  display.setCursor(0, 16);
  display.print("Target: "); 
  display.print(setTarget); 
  display.print(" C");

  display.display();
}

// Function to smooth potentiometer readings
int smoothPotReading() {
  // Subtract the last reading from the total
  total = total - readings[readIndex];
  // Read the new value from the potentiometer
  readings[readIndex] = analogRead(potPin);
  // Add the new reading to the total
  total = total + readings[readIndex];
  // Move to the next index
  readIndex = readIndex + 1;
  // If we’ve reached the end of the array, wrap around
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  // Calculate the average
  average = total / numReadings;
  return average;
}

void setup() {
  Serial.begin(115200);

  // Initialize the readings array to 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }

  pinMode(buzzerPin, OUTPUT);
  pinMode(relayFan, OUTPUT);
  pinMode(relayMist, OUTPUT);

  digitalWrite(relayFan, HIGH);
  digitalWrite(relayMist, HIGH);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  dht.begin();

  display.clearDisplay();
  display.setTextSize(1);     
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0);     

  display.println("Smart Mist Cap");
  display.println("Temperature Reading");
  display.display();
}

void loop() {
  dhtSensor();
  
  int potValue = smoothPotReading(); // Get smoothed potentiometer value
  setTarget = map(potValue, 0, 1023, 100, 0); // Map to target range

  displayOLED();

  if (temperature > setTarget && flag == 0) {
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(200);
    digitalWrite(buzzerPin, HIGH);
    delay(200);
    digitalWrite(buzzerPin, LOW);
    digitalWrite(relayMist, LOW);
    delay(100);
    digitalWrite(relayMist, HIGH);
    digitalWrite(relayFan, LOW);
    delay(3000);
    digitalWrite(relayMist, LOW);
    delay(100);
    digitalWrite(relayMist, HIGH);
    digitalWrite(relayFan, HIGH);

    flag = 1;
  }

  if (temperature < setTarget) {
    digitalWrite(relayMist, HIGH);
    digitalWrite(relayFan, HIGH);
    digitalWrite(buzzerPin, LOW);

    flag = 0;
  }
  
  Serial.print("Pot Value: ");
  Serial.println(potValue);
  Serial.print("Set Target: ");
  Serial.println(setTarget);

  delay(100);
}