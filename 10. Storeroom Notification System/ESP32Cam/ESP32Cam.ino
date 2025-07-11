#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// I2C pins for ESP32-CAM
#define I2C_SDA 14
#define I2C_SCL 15

// Initialize the LCD (address 0x27 is common; adjust if different)
LiquidCrystal_I2C lcd(0x27, 16, 2); // 16x2 LCD

// Wi-Fi credentials
const char *ssid = "abcd";
const char *password = "123456789";

// Telegram Bot credentials (replace with your own)
#define BOT_TOKEN "7581971100:AAF8HQB1A5xb1I4XigwZDMy7bAquirK60as" // e.g., "6869476801:AAEOcKK7JYWUEkJkW4P5qomsQLbziHUKX4s"
#define CHAT_ID "6265743974" // e.g., "0123456789"

// Camera model
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Pin definitions
#define BUTTON_PIN 12 // Push button on GPIO 12 and GND
#define FLASH_LED_PIN 4 // Onboard flash LED

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOT_TOKEN, clientTCP);

int botRequestDelay = 1000; // Check Telegram messages every 1s
unsigned long lastTimeBotRan;
bool sendPhoto = false;

void startCameraServer();
void setupLedFlash(int pin);

// Send photo to Telegram
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return "Camera capture failed";
  }

  digitalWrite(FLASH_LED_PIN, HIGH); // Flash LED on
  delay(100); // Brief flash
  digitalWrite(FLASH_LED_PIN, LOW);

  Serial.println("Connecting to " + String(myDomain));
  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");

    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + String(CHAT_ID) + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    size_t imageLen = fb->len;
    size_t extraLen = head.length() + tail.length();
    size_t totalLen = imageLen + extraLen;

    clientTCP.println("POST /bot" + String(BOT_TOKEN) + "/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    clientTCP.println();
    clientTCP.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n += 1024) {
      if (n + 1024 < fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      } else {
        size_t remainder = fbLen % 1024;
        clientTCP.write(fbBuf, remainder);
      }
    }
    clientTCP.print(tail);
    esp_camera_fb_return(fb);

    int waitTime = 10000; // Timeout 10s
    long startTimer = millis();
    boolean state = false;
    String getAll = "";
    while ((startTimer + waitTime) > millis()) {
      while (clientTCP.available()) {
        char c = clientTCP.read();
        getAll += c;
        state = true;
      }
      delay(100);
    }
    clientTCP.stop();
    if (state) {
      Serial.println("Photo sent successfully");
      return "Photo sent successfully";
    } else {
      Serial.println("Failed to send photo");
      return "Failed to send photo";
    }
  } else {
    esp_camera_fb_return(fb);
    Serial.println("Connection to Telegram failed");
    return "Connection to Telegram failed";
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.setDebugOutput(true);
  Serial.println();
  lcd.init();
  lcd.backlight();

  // Initialize button and LED
  pinMode(BUTTON_PIN, INPUT); // Internal pull-up for button
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; // 320x240 for stability
  config.jpeg_quality = 15; // Lower quality for faster send
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  if (psramFound()) {
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_CIF; // 400x296 if no PSRAM
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    ESP.restart();
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV2640_PID) {
    s->set_vflip(s, 1); // Flip vertically
    s->set_brightness(s, 0);
    s->set_saturation(s, 0);
  }
  s->set_framesize(s, FRAMESIZE_QVGA); // Start with low resolution
  s->set_quality(s, 15);

  // Setup LED flash
  #if defined(LED_GPIO_NUM)
    setupLedFlash(LED_GPIO_NUM);
  #endif

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' for streaming");

  // Start web server
  startCameraServer();

  // Set Telegram client to insecure mode
  clientTCP.setInsecure();

  lcd.setCursor(0, 0);
  lcd.print("Storeroom Notify");
  lcd.setCursor(0, 1);
  lcd.print("System");
}

void loop() {
  // Check for button press
  if (digitalRead(BUTTON_PIN) == HIGH) { // Button pressed (active LOW)
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == HIGH) {
      Serial.println("Button pressed, capturing photo...");
      sendPhoto = true;
    }
    while (digitalRead(BUTTON_PIN) == HIGH); // Wait for release
  }

  // Check for Telegram commands
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      for (int i = 0; i < numNewMessages; i++) {
        String chat_id = String(bot.messages[i].chat_id);
        if (chat_id != CHAT_ID) continue; // Only respond to the specified chat
        String text = bot.messages[i].text;

        // Handle commands
        if (text == "/start") {
          bot.sendMessage(CHAT_ID, "Storeroom Notification System Using Arduino Nano & ESP32 Cam With PIR Sensor", "");
        } else if (text == "/help") {
          bot.sendMessage(CHAT_ID, "Type in telegram search bar '@myidbot', type /getid to get your own id", "");
        } else if (text == "/token") {
          bot.sendMessage(CHAT_ID, BOT_TOKEN, "");
        } else if (text == "Available") {
          bot.sendMessage(CHAT_ID, "Status: Available", "");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Status: ");
          lcd.setCursor(0, 1);
          lcd.print("Available");
          
        } else if (text == "Not available") {
          bot.sendMessage(CHAT_ID, "Status: Not Available", "");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Status: ");
          lcd.setCursor(0, 1);
          lcd.print("Not Available");
        }
      }
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  // Send photo if triggered
  if (sendPhoto) {
    String result = sendPhotoTelegram();
    bot.sendMessage(CHAT_ID, result, "");
    sendPhoto = false;
  }

  delay(100); // Reduced delay for responsiveness
}