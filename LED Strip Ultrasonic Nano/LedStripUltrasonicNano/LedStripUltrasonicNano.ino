const int trigPin = 10;
const int echoPin = 11;
int distance;

const int greenPin = 4;
const int redPin = 5;
const int bluePin = 6;

// Timing variables for non-blocking color cycling
unsigned long previousMillis = 0;
const long interval = 100; // Time between color changes (ms)
int colorIndex = 0;

// Array to store the 8 color combinations (R, G, B)
const bool colorCombinations[8][3] = {
  {false, false, false}, // Off
  {true, false, false},  // Red
  {false, true, false},  // Green
  {false, false, true},  // Blue
  {true, true, false},   // Yellow
  {false, true, true},   // Cyan
  {true, false, true},   // Magenta
  {true, true, true}     // White
};

void ultrasonic() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  delay(25);
}

void setRGB(bool red, bool green, bool blue) {
  // Active-low: LOW = ON, HIGH = OFF
  digitalWrite(redPin, red ? LOW : HIGH);
  digitalWrite(greenPin, green ? LOW : HIGH);
  digitalWrite(bluePin, blue ? LOW : HIGH);
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Initialize LEDs to OFF (HIGH for active-low)
  digitalWrite(greenPin, HIGH);
  digitalWrite(redPin, HIGH);
  digitalWrite(bluePin, HIGH);
}

void loop() {
  ultrasonic();

  unsigned long currentMillis = millis();

  if (distance < 20) {
    // Cycle through colors non-blocking
    if (currentMillis - previousMillis >= interval) {
      // Set the current color
      setRGB(colorCombinations[colorIndex][0], 
             colorCombinations[colorIndex][1], 
             colorCombinations[colorIndex][2]);
      
      // Move to the next color
      colorIndex = (colorIndex + 1) % 8; // Loop back to 0 after 7
      previousMillis = currentMillis;     // Update the last change time
    }
  } else {
    // Turn all LEDs off
    setRGB(false, false, false);
    colorIndex = 0; // Reset to start of color sequence when restarting
  }
}