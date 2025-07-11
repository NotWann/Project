#include <ezButton.h>
#include <esp_now.h>
#include <WiFi.h>

// Pin definitions
#define VRX_PIN 34 //VRX
#define VRY_PIN 39 //VRY
#define SW_PIN 33 

// Initialize button with debouncing
ezButton button(SW_PIN);

// Receiver MAC Address
uint8_t receiverMacAddress[] = {0xB0, 0xB2, 0x1C, 0x97, 0xB1, 0x88};

// Structure to hold joystick data
typedef struct struct_message {
  int xValue;
  int yValue;
  int buttonState;
} struct_message;

// Create a struct_message to hold data to send
struct_message joystickData;

// Callback function for data sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

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

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  button.loop(); // Update button state

  // Read averaged analog values
  valueX = readAveragedADC(VRX_PIN);
  valueY = readAveragedADC(VRY_PIN);

  // Map averaged values to 0–1000
  smooth_valueX = map(constrain(valueX, 0, 4095), 0, 4095, 0, 1000);
  smooth_valueY = map(constrain(valueY, 0, 4095), 0, 4095, 0, 1000);

  // Read button state (0 = not pressed, 1 = pressed)
  bValue = (button.getState() == LOW) ? 1 : 0; // LOW = pressed

  // Set values to send
  joystickData.xValue = smooth_valueX;
  joystickData.yValue = smooth_valueY;
  joystickData.buttonState = bValue;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(receiverMacAddress, (uint8_t *) &joystickData, sizeof(joystickData));
  
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
  }

  // Print values to Serial for debugging
  Serial.print("X:");
  Serial.print(smooth_valueX);
  Serial.print(",Y:");
  Serial.print(smooth_valueY);
  Serial.print(",Button:");
  Serial.println(bValue);

  delay(50); // Adjust delay to control transmission rate
}