//  Sender ESP
// ESP32 Receiver MAC Address:  EC:64:C9:86:0F:CC

#include <esp_now.h>
#include <WiFi.h>

const int limitSwitch = 4;
const int ledLeft = 26;
const int ledRight = 27;
const int ledStop = 2;

uint8_t peerAddress[] = {0xEC, 0x64, 0xC9, 0x86, 0x0F, 0xCC};  // Receiver ESP32 MAC Address

typedef struct {
    bool ledState;
    bool limitState;
} Message;

Message msg;

bool buttonState = false;
bool lastButtonState = LOW;  // Used for debouncing
int lastLimitState = LOW;   // Used for debouncing

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) 
{
    memcpy(&msg, incomingData, sizeof(msg));
    digitalWrite(ledPin, msg.ledState);
    Serial.print("Received LED State: ");
    Serial.println(msg.ledState ? "ON" : "OFF");
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
    Serial.print("Send status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() 
{
    Serial.begin(115200);
    pinMode(buttonPin, INPUT);
    pinMode(ledPin, OUTPUT);
    pinMode(limitSwitch, INPUT_PULLUP);  // Internal pull-up to prevent floating

    WiFi.mode(WIFI_STA);
    
    if (esp_now_init() != ESP_OK) 
    {
        Serial.println("ESP-NOW init failed!");
        return;
    }

    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);

    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, peerAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) 
    {
        Serial.println("Failed to add peer!");
        return;
    }
}

void loop() 
{
    // Handle Button Press with Debouncing
    bool currentButtonState = digitalRead(buttonPin);
    if (currentButtonState == HIGH && lastButtonState == LOW) 
    {
        delay(50); // Debounce delay
        buttonState = !buttonState;  // Toggle LED state
        msg.ledState = buttonState;

        esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&msg, sizeof(msg));

        if (result == ESP_OK) 
        {
            Serial.println("Message sent successfully");
        } 
        else 
        {
            Serial.println("Message send failed");
        }
    }
    lastButtonState = currentButtonState;  // Update last state

    // Handle Limit Switch Change Detection
    bool currentLimitState = digitalRead(limitSwitch);
    if (currentLimitState != lastLimitState) 
    {
        delay(50); // Debounce delay
        msg.limitState = currentLimitState;

        esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&msg, sizeof(msg));

        if (result == ESP_OK) 
        {
            Serial.println("Limit switch - Message sent successfully");
        } 
        else 
        {
            Serial.println("Limit switch - Message send failed");
        }
    }
    lastLimitState = currentLimitState;  // Update last state

  

  delay(50); // Reduce CPU usage
}