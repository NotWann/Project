// Receiver ESP
// ESP32 Sender MAC Address:  94:E6:86:01:F1:9C

#include <esp_now.h>
#include <WiFi.h>

const int leftPin = 25;
const int rightPin = 26;
const int stopPin = 27;
const int relayPin = 4;    

uint8_t peerAddress[] = {0x94, 0xE6, 0x86, 0x01, 0xF1, 0x9C};  // Sender ESP32 MAC Address

typedef struct {
    bool ledState;
    bool limitState;
} Message;

Message msg;
bool buttonState = false;
bool lastButtonState = LOW;
bool relayState = false;

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) 
{
    memcpy(&msg, incomingData, sizeof(msg));
    digitalWrite(ledPin, msg.ledState);
    Serial.print("Received LED State: ");
    Serial.println(msg.ledState ? "ON" : "OFF");

    // Control Relay based on Limit Switch State from sender
    digitalWrite(relayPin, msg.limitState);
    Serial.print("Received Limit Switch State: ");
    Serial.println(msg.limitState ? "ON" : "OFF");
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
    pinMode(relayPin, OUTPUT);

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
    delay(50);
}
