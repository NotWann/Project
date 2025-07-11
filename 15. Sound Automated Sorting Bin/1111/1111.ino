#include <driver/i2s.h>
#include <ESP32Servo.h>

// INMP441 pin config
#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14
#define I2S_PORT I2S_NUM_0

#define bufferLen 64
int16_t sBuffer[bufferLen];

unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 100; // ms

Servo servoAtas;  // Create servo object for servoAtas
Servo servoBawah; // Create servo object for servoBawah

int pos_atas = 0;    // Variable to store the servo position
int pos_bawah = 0;
int servo_atas = 17;  // Pin for servoAtas
int servo_bawah = 16; // Pin for servoBawah

const int e18_sensor = 27;
int lock_object = 0;
const int led = 4;

void i2s_install() {
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

void detect_sampah() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastReadTime >= READ_INTERVAL) 
  {
    lastReadTime = currentMillis;

    size_t bytesIn = 0;
    esp_err_t result = i2s_read(I2S_PORT, &sBuffer, sizeof(sBuffer), &bytesIn, portMAX_DELAY);

    if (result == ESP_OK && bytesIn > 0) 
    {
      int16_t samples_read = bytesIn / sizeof(int16_t);
      int16_t max_val = INT16_MIN;
      int16_t min_val = INT16_MAX;

      for (int i = 0; i < samples_read; i++) 
      {
        if (sBuffer[i] > max_val) max_val = sBuffer[i];
        if (sBuffer[i] < min_val) min_val = sBuffer[i];
      }

      int peak_to_peak = max_val - min_val;

      if (lock_object == 1) {
        if (peak_to_peak > 7000) {
          Serial.println("Commanding servoBawah to 150");
          servoBawah.write(150 );
          Serial.println("Commanding servoAtas to 170");
          Serial.println("BESI");
          delay(3000);
          servoAtas.write(180);
          delay(3000);
          digitalWrite(led, LOW);
          lock_object = 0;
        } 
        else if (peak_to_peak > 1200 && peak_to_peak < 7000) 
        {
          Serial.println("Commanding servoBawah to 100");
          servoBawah.write(50);
          Serial.println("Commanding servoAtas to 170");
          Serial.println("PLASTIC");
          delay(3000);
          servoAtas.write(180);
          delay(3000);
          digitalWrite(led, LOW);
          lock_object = 0;

        }
        else if (peak_to_peak > 300 && peak_to_peak < 1200) 
        {
          Serial.println("Commanding servoBawah to 45");
          servoBawah.write(0);
          Serial.println("Commanding servoAtas to 170");
          Serial.println("PAPER");
          delay(3000);
          servoAtas.write(180);
          delay(3000);
          digitalWrite(led, LOW);
          lock_object = 0;
        }
      }
      
      Serial.print("Peak2Peak: ");
      Serial.println(peak_to_peak);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(e18_sensor, INPUT);
  pinMode(led, OUTPUT); // Corrected to OUTPUT for LED control

  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);

  servoAtas.attach(servo_atas, 1500, 10000);
  servoBawah.attach(servo_bawah);
  servoAtas.write(0);
  servoBawah.write(0);
  Serial.println("Servos initialized");
}

void loop() {
  int state = digitalRead(e18_sensor);
  //Serial.print("E18 Sensor State: ");
  //Serial.println(state);
  if (state == LOW && lock_object == 0) 
  {
    Serial.println("Object Detected");
    digitalWrite(led, HIGH);
    lock_object = 1;
  } 
  detect_sampah();

  if (lock_object == 0)
  {
    servoAtas.write(0);
    servoBawah.write(0);
  }
  delay(10);
}