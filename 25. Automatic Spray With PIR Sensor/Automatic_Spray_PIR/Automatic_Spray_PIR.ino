#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

// Blynk configuration
char auth[] = "DDCgXnmw7ZtZ0_YnJ-TJ0mdeJorGCyKQ";  // Auth token
char ssid[] = "abcd";                              // WiFi SSID
char pass[] = "123456789";                         // WiFi password
char server[] = "prakitblog.com";                  // Blynk server
int port = 8181;                                   // Blynk port

// Pin definitions
const int buzzer_pin = 2;
const int pir_pin = 4;
const int relay_pin = 13;
const int pot_pin = 33;

// Variables
int lock_pir = 0;
unsigned long last_trigger_time = 0;  // Time of last PIR trigger
int wait_time_seconds = 10;
unsigned long trigger_count = 0;      // Counter for PIR triggers
bool output_active = false;           // State for relay and buzzer
unsigned long output_start_time = 0;  // Time when relay/buzzer turned on

// Smoothing variables for potentiometer
const int num_readings = 10;  // Number of readings for moving average
int readings[num_readings];   // Array to store readings
int read_index = 0;           // Current reading index
long total = 0;               // Running total

// Buffers for time and date
char timeBuffer[16];  // For HH:MM:SS
char dateBuffer[16];  // For DD/MM/YYYY
char logBuffer[64];   // For trigger log with count, time, and date

// Blynk objects
WidgetTerminal terminal(V1);
WidgetRTC rtc;
BlynkTimer timer;

// Blynk terminal commands
BLYNK_WRITE(V1) {
  if (String("help") == param.asStr()) {
    terminal.println(F("Blynk v" BLYNK_VERSION ": Device started"));
    terminal.println(F("-------------"));
    terminal.println("Type: ");
    terminal.println("clear - to clear the terminal");
    terminal.println("reset - to reset count");
  } else if (String("clear") == param.asStr()) {
    terminal.clear();
    terminal.println("Terminal cleared!");
  } else if (String("reset") == param.asStr()) {
    terminal.println("Count reset!");
    trigger_count = 0;
  } else {
    terminal.print("You said: ");
    terminal.write(param.getBuffer(), param.getLength());
    terminal.println();
  }
  terminal.flush();
}

// Read potentiometer and calculate wait time
void read_pot() {
  total = total - readings[read_index];          // Subtract old reading
  readings[read_index] = analogRead(pot_pin);    // Read new value
  total = total + readings[read_index];          // Add new reading
  read_index = (read_index + 1) % num_readings;  // Advance to next index

  // Calculate average and map to wait time (1-60 seconds)
  int pot_value = total / num_readings;
  wait_time_seconds = map(pot_value, 0, 4095, 1, 60);

  // Debug output
  Serial.print("Wait Time (seconds): ");
  Serial.println(wait_time_seconds);
  Blynk.virtualWrite(V5, wait_time_seconds);
}

// Update and display time and date
void clockDisplay() {
  if (timeStatus() != timeNotSet) {  // Ensure time is synced
    // Format time as HH:MM:SS
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", hour(), minute(), second());
    // Format date as DD/MM/YYYY
    snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d", day(), month(), year());

    // Send to Blynk app
    Blynk.virtualWrite(V2, timeBuffer);
    Blynk.virtualWrite(V3, dateBuffer);

    // Debug output
    Serial.print("Current time: ");
    Serial.print(timeBuffer);
    Serial.print(" ");
    Serial.print(dateBuffer);
    Serial.println();
  }
}

// Read PIR sensor and log trigger count with timestamp
void read_pir() {
  int current_pirState = digitalRead(pir_pin);
  unsigned long current_time = millis();

  // Manage relay and buzzer non-blocking timing
  if (output_active && (current_time - output_start_time >= 1000)) {
    digitalWrite(relay_pin, LOW);
    digitalWrite(buzzer_pin, LOW);
    output_active = false;
  }

  // Calculate and display countdown
  if (last_trigger_time > 0) {  // Ensure a trigger has happened
    unsigned long time_since_trigger = current_time - last_trigger_time;
    if (time_since_trigger < wait_time_seconds * 1000UL) {
      int time_remaining = (wait_time_seconds * 1000UL - time_since_trigger) / 1000;
      Serial.print("Countdown: ");
      Serial.print(time_remaining);
      Serial.println(" seconds");
      Blynk.virtualWrite(V0, time_remaining);
    } else {
      Serial.println("PIR ready to trigger");
    }
  } else {
    Serial.println("PIR ready to trigger (no motion detected yet)");
  }

  if (current_pirState == HIGH && lock_pir == 0 && (last_trigger_time == 0 || current_time - last_trigger_time >= wait_time_seconds * 1000UL)) {
    trigger_count++;  // Increment trigger counter

    // Format log with count, time, and date
    if (timeStatus() != timeNotSet) {
      snprintf(logBuffer, sizeof(logBuffer), "Count: %lu | Time: %02d:%02d:%02d | Date: %02d/%02d/%04d",
               trigger_count, hour(), minute(), second(), day(), month(), year());
    } else {
      snprintf(logBuffer, sizeof(logBuffer), "Count: %lu | Time: Not synced | Date: Not synced", trigger_count);
    }

    // Log to terminal and Serial
    Serial.println(logBuffer);
    terminal.println(logBuffer);
    terminal.flush();

    // Activate relay and buzzer
    digitalWrite(relay_pin, HIGH);
    digitalWrite(buzzer_pin, HIGH);
    output_active = true;
    output_start_time = current_time;  // Record start time for output
    lock_pir = 1;
    last_trigger_time = current_time;  // Record trigger time
  } else if (current_pirState == LOW && lock_pir == 1) {
    Serial.println("No Motion Detected");
    lock_pir = 0;  // Reset lock when motion stops
  }
}

// Reconnect to Blynk if disconnected
void connectToBlynk() {
  unsigned long start = millis();
  unsigned long last_attempt = 0;
  while (!Blynk.connected() && millis() - start < 10000) {  // 10 seconds timeout
    if (millis() - last_attempt >= 1000) {                  // Attempt every 1 second
      Blynk.run();
      last_attempt = millis();
    }
  }
  if (Blynk.connected()) {
    Serial.println("Blynk Connected!");
  } else {
    Serial.println("Blynk connection failed!");
  }
}
n
BLYNK_CONNECTED() {
  // Synchronize time on connection
  rtc.begin();
}

void setup() {
  Serial.begin(115200);

  // Initialize readings array
  for (int i = 0; i < num_readings; i++) {
    readings[i] = 0;
  }

  // Initialize pins
  pinMode(pir_pin, INPUT);
  pinMode(buzzer_pin, OUTPUT);
  pinMode(relay_pin, OUTPUT);

  // Connect to Blynk
  Blynk.begin(auth, ssid, pass, server, port);

  // Clear terminal and show startup message
  terminal.clear();
  terminal.println(F("Blynk v" BLYNK_VERSION ": Device started"));
  terminal.println(F("-------------"));
  terminal.println("Type: ");
  terminal.println("help - for info");
  terminal.flush();

  // Sync time every 10 minutes
  setSyncInterval(10 * 60);

  // Update clock display every 10 seconds
  timer.setInterval(10000L, clockDisplay);
}

void loop() {
  if (!Blynk.connected()) {
    Serial.println("Blynk disconnected! Reconnecting...");
    connectToBlynk();
  }
  Blynk.run();
  timer.run();
  read_pot();
  read_pir();
}