// Pin definitions
const int motorA1_pin = 13;  // RPWM (Forward PWM)
const int motorA2_pin = 14;  // LPWM (Reverse PWM)
const int enable_pin = 22;

// PWM settings
const int PWM_CHANNEL_A1 = 0;  // PWM channel for forward
const int PWM_CHANNEL_A2 = 1;  // PWM channel for reverse
const int PWM_FREQ = 1000;     // PWM frequency (1 kHz)
const int PWM_RESOLUTION = 8;  // 8-bit resolution (0–255)

void moveforward(int speed) {
  // Ensure speed is within valid range (0–255)
  speed = constrain(speed, 0, 255);
  
  // Set forward direction
  ledcWrite(PWM_CHANNEL_A1, speed);  // Forward PWM
  ledcWrite(PWM_CHANNEL_A2, 0);      // Reverse off
  digitalWrite(enable_pin, HIGH);    // Enable driver
}

void setup() {
  Serial.begin(115200);
  
  // Set pin modes
  pinMode(enable_pin, OUTPUT);
  digitalWrite(enable_pin, HIGH);  // Enable driver by default

  // Configure PWM channels
  ledcSetup(PWM_CHANNEL_A1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_A2, PWM_FREQ, PWM_RESOLUTION);

  // Attach PWM pins
  ledcAttachPin(motorA1_pin, PWM_CHANNEL_A1);
  ledcAttachPin(motorA2_pin, PWM_CHANNEL_A2);
}

void loop() {
  // Example: Move forward at different speeds
  Serial.println("Moving forward at speed 128 (50%)");
  moveforward(128);  // 50% speed
  delay(3000);
  
  Serial.println("Moving forward at speed 255 (100%)");
  moveforward(255);  // Full speed
  delay(3000);
  
  Serial.println("Stopping");
  moveforward(0);  // Stop
  delay(2000);



  if (xAxis == 1024 && yAxis == 1024) stop();
  else if ((xAxis >= 600 && xAxis <= 1600) && (yAxis >= 2000 && yAxis <= 2048)) moveForward();
  else if ((xAxis >= 600 && xAxis <= 1600) && (yAxis >= 0 && yAxis <= 20)) reverse();
  else if ((xAxis >= 2000 && xAxis <= 2048) && (yAxis >= 600 && yAxis <= 1600)) turnRight();
  else if ((xAxis >= 0 && xAxis <= 100) && (yAxis >= 600 && yAxis <= 1600)) turnLeft();
}