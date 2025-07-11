#define Tilt  21
#define LED  25
#define BUZZER 20

int flagTilt = 0;
int flagReset = 0;
unsigned long lastSendTime = 0;

// GPS Data Variables
String rawGPSData = "";
float latitude = 0.0, longitude = 0.0;
bool locationValid = false;

void sendATCommand(String command) 
{
  Serial.println("Sending: " + command);
  Serial2.println(command);
  delay(1000);

  String response = "";
  unsigned long startTime = millis();
  if (millis() - startTime < 5000) 
  {
    if (Serial2.available() > 0) 
    {
      Serial.write(Serial2.read());
    }
  }
}

void sendSMS(String phoneNumber, String message) 
{
  Serial.println("Sending SMS...");
  Serial2.println("AT+CMGF=1");
  delay(1000);
  Serial2.print("AT+CMGS=\"");  
  Serial2.print(phoneNumber);  
  Serial2.println("\"");  
  delay(1000);
  
  Serial2.print(message);  
  delay(100);
  
  Serial2.write(26);  // Send Ctrl+Z to finalize SMS
  delay(1000);
}

void checkSMS() 
{
  Serial1.println("AT+CNMI=2,2,0,0,0");
  delay(1000);
  if (Serial1.available() > 0)
  {
    char c = Serial2.read();
    Serial.print(c);

    rawGPSData += c;  // Store response
    if (c == '\n') 
    {
      if (rawGPSData.indexOf("+CMT:") >= 0) 
      {
        if (rawGPSData.indexOf("LOCATION") >= 0) 
        {
          int phoneStart = rawGPSData.indexOf("\"+") + 2;
          int phoneEnd = rawGPSData.indexOf("\"", phoneStart);
          String senderPhone = rawGPSData.substring(phoneStart, phoneEnd);

          if (senderPhone.length() >= 10) 
          {
            sendATCommand("AT+CMGF=1");
            sendATCommand("AT+CNMI=1,2,0,0,0"); 
            sendATCommand("AT+CSMP=17,167,0,0"); 
            sendSMS(senderPhone, "Anti-theft system: " + getLocationString());

            updateSerial();

            digitalWrite(BUZZER, HIGH);
            delay(200);
            digitalWrite(BUZZER, LOW);
            delay(200);

            rawGPSData = ""; // Reset buffer to prevent hangs
          }
        }
      }
    }
    Serial.write(Serial2.read());
  }
}

void readGPS() 
{
  while (Serial1.available()) 
  {
    char c = Serial1.read();
    if (c == '\n') 
    {
      parseGPSData(rawGPSData);
      rawGPSData = ""; // Reset buffer
    } 
    else 
    {
      rawGPSData += c;
    }
  }
}

void parseGPSData(String data) 
{
  if (data.startsWith("$GPGGA") || data.startsWith("$GPRMC")) 
  {
    String values[15];
    int index = 0, lastIndex = 0;

    // Split NMEA sentence by commas
    for (int i = 0; i < data.length(); i++)
    {
      if (data[i] == ',' || i == data.length() - 1) 
      {
        values[index++] = data.substring(lastIndex, i);
        lastIndex = i + 1;
      }
    }

    // Extract latitude and longitude
    if (values[2] != "" && values[4] != "") 
    {
      latitude = convertToDecimal(values[2].toFloat(), values[3]);
      longitude = convertToDecimal(values[4].toFloat(), values[5]);
      locationValid = true;
    }
  }
}

float convertToDecimal(float raw, String direction) 
{
  int degrees = (int)(raw / 100);
  float minutes = raw - (degrees * 100);
  float decimal = degrees + (minutes / 60);
  if (direction == "S" || direction == "W") decimal *= -1;
  return decimal;
}

String getLocationString() 
{
  if (locationValid) {
    return "Location: " + String(latitude, 6) + "," + String(longitude, 6);
  }
  return "GPS location not available yet.";
}

void updateSerial() {
  if (Serial.available() > 0) 
  {
    Serial2.write(Serial.read());
  }  //Forward what Serial received to Software Serial Port
    
  if (Serial2.available() > 0) 
  {
    Serial.write(Serial2.read());
  }  //Forward what Software Serial received to Serial Port
  delay(10);
}

void setup() 
{
  Serial.begin(115200);
  Serial2.begin(38400);  // GSM Module
  Serial1.begin(9600);   // GPS Module
  
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(Tilt, INPUT);
  
  sendATCommand("AT");
  sendATCommand("AT+CSQ");
  sendATCommand("AT+CMGF=1");   // Set SMS to text mode
  sendATCommand("AT+CNMI=1,2,0,0,0"); // New SMS notification
  sendATCommand("AT+CSMP=17,167,0,0"); // Proper SMS format
  
  sendSMS("+60199306403", "System ON");

  sendATCommand("AT+CMGD=1,4"); // Clear old messages

  //checkSMS();
  updateSerial();
}

void loop()
{
  readGPS();

  int tiltValue = digitalRead(Tilt);  

  if (tiltValue == 0 && flagTilt == 1) 
  {
    //if (millis() - lastSendTime > 2000) 
    //{  // Send SMS every 2s max
      sendATCommand("AT+CMGF=1");   // Set SMS to text mode
      sendATCommand("AT+CNMI=1,2,0,0,0"); // New SMS notification
      sendATCommand("AT+CSMP=17,167,0,0"); // Proper SMS format
      String alertMessage = "From Anti Theft System: Vibration Detected! " + getLocationString();
      sendSMS("+60199306403", alertMessage);

      digitalWrite(LED, HIGH);
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      delay(200);
      Serial.println("TILT!");

      //lastSendTime = millis();
    //}
     flagTilt = 0;
  }

  if (tiltValue == 1 && flagTilt == 0) 
  {
    digitalWrite(LED, LOW);
    digitalWrite(BUZZER, LOW);
    Serial.println("Not Tilt");

    flagTilt = 1;
    
  }
  updateSerial();
  checkSMS();
  
}


