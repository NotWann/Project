#include <TinyGPS++.h>

#define Tilt  21
#define LED  25
#define BUZZER 20

int flagTilt = 0;

// Create a TinyGPS++ object
TinyGPSPlus gps;

// Variables to store latest GPS position
float latitude = 0.0;
float longitude = 0.0;
bool locationValid = false;

void sendATCommand(String command) 
{
  Serial.print("Sending: ");
  Serial.println(command);
  Serial2.println(command);
  delay(1000);
  
  // Read response from GSM module
  while (Serial2.available()) {
    Serial.write(Serial2.read());
  }
  Serial.println(); // New line for better readability
}

void sendSMS(String phoneNumber, String message) 
{
  Serial.println("Sending SMS...");
  Serial2.print("AT+CMGS=\"");  
  Serial2.print(phoneNumber);  
  Serial2.println("\"");  
  delay(1000);
  
  Serial2.print(message);  // Send message
  delay(500);
  
  Serial2.write(26);  // Send Ctrl+Z (ASCII 26) to finalize SMS
  delay(5000);
      
  Serial.println("SMS Sent!");
}

void updateSerial() {
  while (Serial.available()) 
  {
    Serial2.write(Serial.read());
  }  //Forward what Serial received to Software Serial Port
    
  while(Serial2.available()) 
  {
    Serial.write(Serial2.read());
  }  //Forward what Software Serial received to Serial Port
}

void readGPS() {
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    if (gps.encode(c)) {
      updateGPSData();
    }
  }
}

void updateGPSData() {
  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    locationValid = true;
  }
}

String getLocationString() {
  if (locationValid) {
    return "Location: http://maps.google.com/maps?q=" + 
           String(latitude, 6) + "," + 
           String(longitude, 6);
  } else {
    return "GPS location not available yet.";
  }
}

void setup() 
{
  delay(2000);
  Serial.begin(115200);
  Serial2.begin(38400);  // GSM Module
  Serial1.begin(9600);   // GPS Module
  
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(Tilt, INPUT);
  
  Serial2.println("AT");  // AT Command test
  updateSerial();
  Serial2.println("AT+CSQ"); //Signal quality test, value range is 0-31 , 31 is the best
  updateSerial();
    
  delay(500);
}

void loop() 
{
  // Read GPS data
  readGPS();
  
  int tiltValue = digitalRead(Tilt);
  if (tiltValue == 1 && flagTilt == 0) //if the DO from Tilt sensor is low, no tilt detected. You might need to adjust the potentiometer to get reading
  {
      digitalWrite(LED,LOW);
    digitalWrite(BUZZER, LOW);
    updateSerial();
    Serial.println("Not Tilt");
    
    flagTilt = 1;
  }
    
  if (tiltValue == 0 && flagTilt == 1)
  {
    sendATCommand("AT");  // Check communication
    sendATCommand("AT+CMGF=1");  // Set SMS mode to text
    sendATCommand("AT+CSMP=17,167,0,0");  // Set SMS parameters
      
    // Get the current location and add it to the alert message
    String alertMessage = "From Anti Theft System: Vibration Detected! ";
    alertMessage += getLocationString();
    
    sendSMS("+60199306403", alertMessage);
    //Serial2.println("AT+CIPGSMLOC=1,1");
    updateSerial();
    
    digitalWrite(LED,HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(BUZZER, LOW);
    delay(200);
    Serial.println("TILT!");
        
    flagTilt = 0;
  }
  
  // Check for SMS location requests
  while (Serial2.available()) {
    String response = Serial2.readString();
    if (response.indexOf("+CMT:") >= 0) {
      if (response.indexOf("LOCATION") >= 0 || 
          response.indexOf("WHERE") >= 0 || 
          response.indexOf("GPS") >= 0 || 
          response.indexOf("POSITION") >= 0) {
        
        // Extract sender's phone number
        int phoneStartIndex = response.indexOf("\"+") + 2;
        int phoneEndIndex = response.indexOf("\"", phoneStartIndex);
        String senderPhone = "";
        
        if (phoneStartIndex > 0 && phoneEndIndex > 0) {
          senderPhone = response.substring(phoneStartIndex, phoneEndIndex);
          // Only send if we have a valid phone number
          if (senderPhone.length() >= 10) {
            sendATCommand("AT+CMGF=1");  // Set SMS mode to text
            sendSMS(senderPhone, "Anti-theft system: " + getLocationString());
          }
        }
      }
    }
  }
  
  delay(10);
}