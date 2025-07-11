#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <PubSubClient.h> // MQTT library for ESP32
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Your Blynk Legacy Auth Token
char auth[] = "YOUR_BLYNK_AUTH_TOKEN"; 

// WiFi Credentials
char ssid[] = "OPPO F9";
char pass[] = "abc9519992";

// MQTT Broker (HiveMQ Public Broker)
const char* mqttServer = "b66451eae191467c898439603853c8a5.s1.eu.hivemq.cloud";
const int mqttPort = 8883;
const char* mqttClientID = "Mycest"; // Must be unique
const char* mqttTopic = "Mycest2468";

// Define Sensor Pins
#define TRIG_PIN 17
#define ECHO_PIN 16
#define LED 2
#define SENSOR 27

WiFiClient espClient;
PubSubClient mqttClient(espClient);

LiquidCrystal_I2C lcd(0x27, 20, 4);

long previousMillis = 0;
int interval = 1000;
volatile byte pulseCount;
float flowRate;
float calibrationFactor = 4.5;

// Pulse counter for flow sensor
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  
  pinMode(LED, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected");

  // Connect to MQTT Broker
  mqttClient.setServer(mqttServer, mqttPort);
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect(mqttClientID)) {
      Serial.println("Connected!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }

  // Start Blynk Legacy
  Blynk.begin(auth, ssid, pass);
}

void loop() {
  Blynk.run();
  if (!mqttClient.connected()) {
    Serial.println("MQTT Disconnected. Reconnecting...");
    mqttClient.connect(mqttClientID);
  }
  mqttClient.loop(); // Keep MQTT connection alive

  // Ultrasonic Sensor Data
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.034) / 2;
  float percentage = constrain(map(distance, 0, 60, 100, 0), 0, 100);

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Send Sensor Data to MQTT
    char mqttMessage[50];
    snprintf(mqttMessage, 50, "Distance: %.2f cm, Percentage: %.2f %%", distance, percentage);
    mqttClient.publish(mqttTopic, mqttMessage);
    
    // Update Blynk App (V1 for Percentage)
    Blynk.virtualWrite(V1, percentage);
    
    Serial.println(mqttMessage);
  }

  lcd.setCursor(0, 0);
  lcd.print("Percentage: ");
  lcd.print(percentage);
  lcd.print("%");

  delay(500);
}
