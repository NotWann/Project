#include <esp_now.h>
#include <WiFi.h>

// Toggle ESP-NOW (set to 0 to disable for debugging)
#define USE_ESP_NOW 1

// Receiver's MAC address (only used if USE_ESP_NOW is 1)
uint8_t receiverMacAddress[] = {0xB0, 0xB2, 0x1C, 0x97, 0xB1, 0x88};

// Pin definitions
const int VRX_PIN = 34; // X-axis (ADC1_CH0)
const int VRY_PIN = 39; // Y-axis (ADC1_CH6)
const int SW_PIN = 33;  // Button

// Structure to hold joystick data
typedef struct struct_message {
  int xValue; // Mapped 0–2095
  int yValue; // Mapped 0–2095
  bool buttonPressed; // 0 = not pressed, 1 = pressed
} struct_message;

struct_message joystickData;

// Variables for low-pass filter
float smoothX = 0;
float smoothY = 0;
const float ALPHA = 0.3; // Low-pass filter constant (0–1, lower = smoother)

// Callback function for send status
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Function to read averaged ADC value (10 readings)
int readAveragedADC(int pin) {
  int readings = 0;
  for (int i = 0; i < 10; i++) {
    readings += analogRead(pin);
    delay(1); // Small delay for ADC stability
  }
  return readings / 10;
}

void setup() {
  Serial.begin(115200); // Start Serial for debugging
  pinMode(SW_PIN, INPUT_PULLUP); // Button with pull-up
  analogSetAttenuation(ADC_11db); // Set ADC to 0–3.3V range

#if USE_ESP_NOW
  // Initialize WiFi in STA mode for ESP-NOW
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
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
#else
  // Disable Wi-Fi for debugging
  WiFi.mode(WIFI_OFF);
#endif
}

void loop() {
  // Read averaged joystick values
  int rawX = readAveragedADC(VRX_PIN);
  int rawY = readAveragedADC(VRY_PIN);

  // Apply low-pass filter
  smoothX = (ALPHA * rawX) + ((1 - ALPHA) * smoothX);
  smoothY = (ALPHA * rawY) + ((1 - ALPHA) * smoothY);

  // Map smoothed values to 0–2095
  joystickData.xValue = map(constrain((int)smoothX, 0, 4095), 0, 4095, 0, 255);
  joystickData.yValue = map(constrain((int)smoothY, 0, 4095), 0, 4095, 0, 255);

  // Read button state
  joystickData.buttonPressed = (digitalRead(SW_PIN) == LOW); // LOW = pressed

  // Print raw and mapped values for debugging
  Serial.print("Raw X:"); Serial.print(rawX);
  Serial.print(",Raw Y:"); Serial.print(rawY);
  Serial.print(",Mapped X:"); Serial.print(joystickData.xValue);
  Serial.print(",Mapped Y:"); Serial.println(joystickData.yValue);

#if USE_ESP_NOW
  // Send data via ESP-NOW
  esp_err_t result = esp_now_send(receiverMacAddress, (uint8_t *) &joystickData, sizeof(joystickData));
  if (result == ESP_OK) {
    Serial.println("Sent joystick data");
  } else {
    Serial.println("Error sending data");
  }
#endif

  delay(5); // Update every 100ms
}