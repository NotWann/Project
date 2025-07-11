#define TILT 21
#define LED 25
#define BUZZER 20
#define RELAY 22

//

int flagTilt = 0;
int lockRelay = 0;
unsigned long lastSentTime = 0;

//

String nmeaSentence = "";
float latitude;
float longitude;

//

String msg;
int flag = 0;

//

void setup()
{
  Serial.begin(115200);
  Serial1.begin(9600);    //  GPS (TX: 0, RX: 1) 
  Serial2.begin(38400);   //  GSM (TX: 16, RX: 17)
  
  pinMode(TILT, INPUT);
  
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);
  
  digitalWrite(RELAY, HIGH);

  delay(3000);
}

//

void loop()
{
  gps_read();

  int tiltValue = digitalRead(TILT);

  if (tiltValue == 0 && flagTilt == 1 && lockRelay == 0)
  {
    if (millis() - lastSentTime  > 2000)
    {
      SendMessage();

      digitalWrite(BUZZER, HIGH);
      digitalWrite(LED, HIGH);
      digitalWrite(RELAY, HIGH);
      delay(200);
      //digitalWrite(BUZZER, LOW);
      digitalWrite(LED, LOW);
      delay(200);

      Serial.println("Vibration Detected!");
      lastSentTime = millis();
    }
    flagTilt = 0;
    //flag = 0;
    lockRelay = 1;
    delay(1000);
    Serial2.println("AT+CFUN=1,1");   //    RESET gsm
  }

  if (tiltValue == 1 && flagTilt == 0)
  {
    digitalWrite(LED, LOW);
    //digitalWrite(BUZZER, LOW);
    Serial.println("No Movement.");
    
    flagTilt = 1;
    //flag = 0;
  }

  ReceiveMessage();
}

//

void SendMessage()
{
  Serial2.println("AT+CMGF=1");    
  delay(1000);  
  Serial2.println("AT+CMGS=\"+60143443016\"\r"); 
  delay(1000);
  Serial2.println("Theft Alert Location: "+ String(latitude,6)+"," + String(longitude,6));
  delay(100);
  Serial2.println((char)26);
  delay(1000);
  Serial.println("Alert! Vibration Detected!");
  msg = " ";
  delay(1000);
  Serial2.println("AT+CFUN=1,1");
}

void ReceiveMessage()
{
  //mySerial.println("AT+CNMI=2,2,0,0,0"); 
 // Serial.println(msg);
  if (Serial2.available()>0)
  {
    msg=Serial2.readString();
    Serial.print("mesej");
    Serial.println(msg);
    flag = 0;

    if((msg.indexOf("GPS") > -1))
    {
       digitalWrite(BUZZER,HIGH);
       delay(200);
       //digitalWrite(BUZZER,LOW);
       SendMessage();
       msg = "       ";
    }
    

    else if ((msg.indexOf("OFF") > -1) && (lockRelay == 1))
    {
       digitalWrite(RELAY, LOW);
       Serial2.println("AT+CMGF=1");    
       delay(1000);  
       Serial2.println("AT+CMGS=\"+60143443016\"\r"); 
       delay(1000);
       Serial2.println("RELAY OFF");
       delay(100);
       Serial2.println((char)26);
       delay(1000);
       Serial.println("RELAY OFF");
       delay(1000);
       Serial2.println("AT+CFUN=1,1");
       
       digitalWrite(BUZZER, HIGH);
       delay(200);
       //digitalWrite(BUZZER, LOW);
       lockRelay = 0;
    }
  }
}

//GPS F

void gps_read()
{
  if (Serial1.available()) {
    char c = Serial1.read();  // Read one character
    if (c == '\n') {          // End of sentence
      if (nmeaSentence.startsWith("$GPGGA")) {
        extractLatLong(nmeaSentence); // Extract data if GPGGA sentence
      }
      nmeaSentence = ""; // Clear buffer for next sentence
    } else {
      nmeaSentence += c; // Append character to buffer
    }
  }
}

//gps function calculation
void extractLatLong(String nmea) {
  // Example GPGGA sentence: $GPGGA,123456.78,3723.2475,N,12202.3578,W,1,08,0.9,545.4,M,46.9,M,,*47
  String rawLat = getField(nmea, 2);  // Latitude value (raw)
  String latDir = getField(nmea, 3); // Latitude direction (N/S)
  String rawLon = getField(nmea, 4); // Longitude value (raw)
  String lonDir = getField(nmea, 5); // Longitude direction (E/W)

  // Convert raw latitude and longitude to decimal degrees
  latitude = convertToDecimal(rawLat);
  if (latDir == "S") latitude = -latitude; // South is negative

  longitude = convertToDecimal(rawLon);
  if (lonDir == "W") longitude = -longitude; // West is negative

  // Display latitude and longitude in decimal degrees
  Serial.print("Latitude: ");
  Serial.println(latitude, 6); // Print with 6 decimal places
  Serial.print("Longitude: ");
  Serial.println(longitude, 6); // Print with 6 decimal places
}

float convertToDecimal(String rawValue) {
  // Convert DDMM.MMMM to decimal degrees
  float value = rawValue.toFloat();
  int degrees = (int)(value / 100); // Extract degrees (integer part)
  float minutes = value - (degrees * 100); // Extract minutes
  return degrees + (minutes / 60); // Combine degrees and minutes
}

String getField(String data, int index) {
  // Extract specific field from NMEA sentence by commas
  int commaCount = 0;
  int startIndex = 0;
  for (size_t i = 0; i < data.length(); i++) {
    if (data[i] == ',') {
      commaCount++;
      if (commaCount == index) {
        startIndex = i + 1;
      } else if (commaCount == index + 1) {
        return data.substring(startIndex, i);
      }
    }
  }
  return ""; // Return empty string if field not found
}