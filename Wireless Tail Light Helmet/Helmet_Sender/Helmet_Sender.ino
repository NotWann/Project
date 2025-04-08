//  Sender ESP
// ESP32 Receiver MAC Address:  3C:61:05:2E:87:48

#include <esp_now.h>
#include <WiFi.h>

const int limitSwitch = 4;
const int leftLedPin = 25;
const int stopLedPin = 26;
const int rightLedPin = 27;

int currentLimitState = 0;

uint8_t peerAddress[] = {0x3C, 0x61, 0x05, 0x2E, 0x87, 0x48};  // Receiver ESP32 MAC Address

typedef struct
{
  bool leftLed;
  bool rightLed;
  bool stopLed;
  bool limitState;
}
Message;

Message msg;
bool lastLimitState = LOW;

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
{
  memcpy(&msg, incomingData, sizeof(msg));
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
    Serial.print("Send status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup()
{
  Serial.begin(115200);

  pinMode(leftLedPin, OUTPUT);
  pinMode(rightLedPin, OUTPUT);
  pinMode(stopLedPin, OUTPUT);

  pinMode(limitSwitch, INPUT_PULLUP);

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
   // Limit Switch
  currentLimitState = digitalRead(limitSwitch);

  if (currentLimitState == LOW)
  {
    msg.limitState = 1;
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
  else 
  {
    msg.limitState = 0;
    esp_now_send(peerAddress, (uint8_t *)&msg, sizeof(msg));
  }
  

  int leftState = msg.leftLed;
  int rightState = msg.rightLed;
  int stopState = msg.stopLed;

  if (leftState == 1)
  {
    digitalWrite(leftLedPin, HIGH);
    delay(200);
    digitalWrite(leftLedPin, LOW);
    delay(200);
  }
  else  
  {
    digitalWrite(leftLedPin, LOW);
  }

  if (rightState == 1)
  {
    digitalWrite(rightLedPin, HIGH);
    delay(200);
    digitalWrite(rightLedPin, LOW);
    delay(200);
  }
  else 
  {
    digitalWrite(rightLedPin, LOW);
  }

  if (stopState == 1)
  {
    digitalWrite(stopLedPin, HIGH);
  }
  else
  {
    digitalWrite(stopLedPin, LOW);
  }
  
  Serial.print("Limit: ");
  Serial.println(msg.limitState);
  Serial.print("Received Stop LED State: ");
  Serial.println(msg.stopLed);
  delay(50);
}











