// Receiver ESP
// ESP32 Sender MAC Address:  AC:67:B2:35:A7:18

#include <esp_now.h>
#include <WiFi.h>

const int leftPin = 25;
const int rightPin = 26;
const int stopPin = 27;
const int relayPin = 4;
const int buzzerPin = 2;    

bool prevRelayState = false;

int flagRelay = 0;

uint8_t peerAddress[] = {0xAC, 0x67, 0xB2, 0x35, 0xA7, 0x18};  // Sender ESP32 MAC Address

typedef struct 
{
  bool leftLed;
  bool rightLed;
  bool stopLed;
  bool limitState;
}
Message;

Message msg;
bool leftButtonState = false;
bool rightButtonState = false;
bool stopButtonState = false;

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

  pinMode(leftPin, INPUT_PULLUP);
  pinMode(rightPin, INPUT_PULLUP);
  pinMode(stopPin, INPUT_PULLDOWN);

  pinMode(relayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

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
  int currentLeftState = digitalRead(leftPin);
  int currentRightState = digitalRead(rightPin);
  int currentStopState = digitalRead(stopPin);

  //  left signal
  if (currentLeftState == HIGH)
  {
    delay(50);
    msg.leftLed = 1;

    esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&msg, sizeof(msg));

    if (result == ESP_OK) 
    {
      Serial.println("LEFT - Message sent successfully");
    } 
    else 
    {
      Serial.println("LEFT - Message send failed");
      currentLeftState = LOW;
      msg.leftLed = 0;
    }
  }
  else
  {
    currentLeftState = LOW;
    msg.leftLed = 0;
    esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&msg, sizeof(msg));
  }

  //  right signal
  if (currentRightState == HIGH)
  {
    delay(50);
    msg.rightLed = 1;

    esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&msg, sizeof(msg));

    if (result == ESP_OK) 
    {
      Serial.println("RIGHT - Message sent successfully");
    } 
    else 
    {
      Serial.println("RIGHT - Message send failed");
      currentRightState = LOW;
      msg.rightLed = 0;
    }
  }
  else 
  {
    currentRightState = LOW;
    msg.rightLed = 0;
    esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&msg, sizeof(msg));
  }

  //  stop signal
  if (currentStopState == HIGH)
  {
    delay(50);
    msg.stopLed = 1;

    esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&msg, sizeof(msg));

    if (result == ESP_OK) 
    {
      Serial.println("STOP - Message sent successfully");
    } 
    else 
    {
      Serial.println("STOP - Message send failed");
      currentStopState = LOW;
      msg.stopLed = 0;
    }
  }
  else
  {
    currentStopState = LOW;
    msg.stopLed = 0;
    esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&msg, sizeof(msg));
  }

  bool currentRelayState = msg.limitState;

  if (currentRelayState && !prevRelayState)
  {
    flagRelay = !flagRelay;    
    digitalWrite(relayPin, HIGH);
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    //digitalWrite(relayPin, LOW);
    digitalWrite(buzzerPin, LOW);
    delay(1000);
  }
  else if (currentRelayState == 0) 
  {
    digitalWrite(relayPin, LOW);
    //digitalWrite(buzzerPin, LOW);
  }
  prevRelayState = currentRelayState;

  Serial.print("STOP: ");
  Serial.println(msg.stopLed);

  delay(50);
}
