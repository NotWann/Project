#define TILT 21
#define LED 25
#define BUZZER 20

int flagTilt = 0;
unsigned long lastSentTime = 0;
String gpsBuffer = "";

float latitude = 0.0, longitude = 0.0;
bool locationValid = false;

// Function to read GPS data
void readGPS()
{
  while (Serial1.available()) 
  {
    char g = Serial1.read();
    if (g == '\n') 
    {
      parseGPSData(gpsBuffer);
      gpsBuffer = "";
    } 
    else 
    {
      gpsBuffer += g;
    }
  }
}

// Function to parse GPS NMEA data
void parseGPSData(String data) 
{
  if (data.startsWith("$GPGGA") || data.startsWith("$GPRMC")) 
  {
    String values[15];
    int index = 0, lastIndex = 0;

    for (size_t i = 0; i < data.length(); i++) 
    {
      if (data[i] == ',' || i == data.length() - 1) 
      {
        values[index++] = data.substring(lastIndex, i);
        lastIndex = i + 1;
      }
    }

    if (values[2] != "" && values[4] != "") 
    {
      latitude = convertToDecimal(values[2].toFloat(), values[3]);
      longitude = convertToDecimal(values[4].toFloat(), values[5]);
      locationValid = true;
    }
  }
}

// Function to convert GPS coordinates to decimal format
float convertToDecimal(float raw, String direction) 
{
  int degrees = (int)(raw / 100);
  float minutes = raw - (degrees * 100);
  float decimal = degrees + (minutes / 60);
  if (direction == "S" || direction == "W") decimal *= -1;
  return decimal;
}

// Function to return location as a string
String getLocationString() 
{
  if (locationValid) 
  {
    return "Location: " + String(latitude, 6) + "," + String(longitude, 6);
  }
  return "GPS location not available yet.";
}

// Function to send SMS via GSM module
void sendSMS(String phoneNumber, String message) 
{
  Serial.println("Sending message...");
  
  Serial2.print("AT+CMGS=\"");
  Serial2.print(phoneNumber);
  Serial2.println("\"");
  delay(1000);

  unsigned long startTime = millis();
  while (millis() - startTime < 5000) 
  {
    if (Serial2.available()) 
    {
      String response = Serial2.readString();
      Serial.println("GSM Response: " + response);
      if (response.indexOf(">") >= 0) break;
    }
  }

  if (millis() - startTime >= 5000) 
  {
    Serial.println("Error: GSM did not respond to AT+CMGS");
    return;
  }

  Serial2.print(message);
  delay(1000);
  Serial2.write(26);  // Send CTRL+Z
  delay(5000);

  startTime = millis();
  while (millis() - startTime < 5000) 
  {
    if (Serial2.available()) 
    {
      String response = Serial2.readString();
      Serial.println("GSM Response: " + response);
      if (response.indexOf("OK") >= 0 || response.indexOf("+CMGS") >= 0) 
      {
        Serial.println("SMS Sent Successfully!");
        sendATCommand("AT+CMGD=1,4");
        return;
      }
    }
  }

  Serial.println("SMS Sending Failed or No Response.");
  sendATCommand("AT+CMGD=1,4");
}

// Function to check incoming SMS and respond with location
void checkSMS()
{
  if (Serial2.available()) 
  {
    String sms = Serial2.readString();
    Serial.println("GSM Data: " + sms);

    if (sms.indexOf("+CMT:") >= 0) 
    {
      int contentStart = sms.indexOf("\r\n", sms.indexOf("+CMT:")) + 2;
      String smsContent = sms.substring(contentStart);
      smsContent.trim();

      if (smsContent.indexOf("LOCATION") >= 0) 
      {
        int phoneStart = sms.indexOf("+CMT: \"") + 7;
        int phoneEnd = sms.indexOf("\"", phoneStart);
        if (phoneStart > 0 && phoneEnd > 0) 
        {
          sendATCommand("AT+CMGF=1");
          sendATCommand("AT+CNMI=2,2,0,0,0");
          sendATCommand("AT+CSMP=17,167,0,0");
          sendSMS(sms.substring(phoneStart, phoneEnd), getLocationString());

          digitalWrite(BUZZER, HIGH);
          digitalWrite(LED, HIGH);
          delay(200);
          digitalWrite(BUZZER, LOW);
          digitalWrite(LED, LOW);
          delay(200);

          sendATCommand("AT+CMGD=1,4");
        }
      }
    }
  }
}

// Function to send AT command
void sendATCommand(String command)
{
  Serial.println("Sending: " + command);
  Serial2.println(command);
  delay(1000);

  while (Serial2.available()) 
  {
    Serial.write(Serial2.read());
  }
}

// Setup function
void setup()
{
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial2.begin(38400);

  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(TILT, INPUT);

  sendATCommand("AT");
  sendATCommand("AT+CSQ");
  sendATCommand("AT+CMGF=1");
  sendATCommand("AT+CNMI=2,2,0,0,0");
  sendATCommand("AT+CSMP=17,167,0,0");
  sendSMS("+60199306403", "System ON");
  sendATCommand("AT+CMGD=1,4");
  
}

// Loop function
void loop()
{
  readGPS();

  int tiltValue = digitalRead(TILT);

  if (tiltValue == 0 && flagTilt == 1)
  {
    if (millis() - lastSentTime  > 2000)
    {
      sendATCommand("AT+CMGF=1");
      String alertMessage = "From Anti Theft System: Vibration Detected! " + getLocationString();
      sendSMS("+60199306403", alertMessage);
      sendATCommand("AT+CNMI=2,2,0,0,0");

      digitalWrite(BUZZER, HIGH);
      digitalWrite(LED, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      digitalWrite(LED, LOW);
      delay(200);

      Serial.println("Vibration Detected!");
      lastSentTime = millis();
    }
    flagTilt = 0;
  }

  if (tiltValue == 1 && flagTilt == 0)
  {
    digitalWrite(LED, LOW);
    digitalWrite(BUZZER, LOW);
    Serial.println("No Movement.");
    
    sendATCommand("AT+CMGD=1,4");
    
    flagTilt = 1;
  }

  checkSMS();
}
