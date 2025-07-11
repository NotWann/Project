#include <ezButton.h> // Library for button debouncing

// Pin definitions
#define VRX_PIN 34 // ESP32 GPIO36 (ADC1_CH0) connected to VRX
#define VRY_PIN 39 // ESP32 GPIO39 (ADC1_CH3) connected to VRY
#define SW_PIN 33  // ESP32 GPIO17 connected to SW (button)

// Initialize button with debouncing
ezButton button(SW_PIN);

// Variables to store readings
int valueX = 0; // Store raw X-axis value
int valueY = 0; // Store raw Y-axis value
int smooth_valueX = 0; // Store smoothed and mapped X value
int smooth_valueY = 0; // Store smoothed and mapped Y value
int bValue = 0; // Store button state (0 = not pressed, 1 = pressed)

// Function to read averaged ADC value (5 readings)
int readAveragedADC(int pin) {
  int readings = 0;
  for (int i = 0; i < 5; i++) {
    readings += analogRead(pin);
    delay(1); // Small delay between readings for ADC stability
  }
  return readings / 5;
}

void setup() {
  Serial.begin(115200); // Start Serial at 115200 baud
  analogSetAttenuation(ADC_11db); // Set ADC to 0–3.3V range
  button.setDebounceTime(50); // Set debounce time to 50ms
}

void loop() {
  button.loop(); // Update button state

  // Read averaged analog values
  valueX = readAveragedADC(VRX_PIN);
  valueY = readAveragedADC(VRY_PIN);

  // Map averaged values to 0–2095
  smooth_valueX = map(constrain(valueX, 0, 4095), 0, 4095, 0, 1000);
  smooth_valueY = map(constrain(valueY, 0, 4095), 0, 4095, 0, 1000);

  // Read button state (0 = not pressed, 1 = pressed)
  bValue = (button.getState() == LOW) ? 1 : 0; // LOW = pressed

  Serial.print("X:");
  Serial.print(smooth_valueX);
  Serial.print(",Y:");
  Serial.print(smooth_valueY);
  Serial.print(",Button:");
  Serial.println(bValue);

  delay(5); 
}