#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RtcDS3231.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
RtcDS3231<TwoWire> Rtc(Wire);

#define TIME 11
#define WATER 12
#define MOD 13

#define TRIG_PIN 9
#define ECHO_PIN 10

#define TURBIDTY_PIN 3

int relayIN1 = 2;
int relayIN2 = 6;
int relayIN3 = 7;

const int potPin = 0;
const int buzzerPin = 5;

int hour[4] = {0, 1, 2, 3};
int minute[4] = {4, 5, 6, 7};
int water[2] = {8,  9};

int H1, H2, H3, H4;
int M1, M2, M3, M4;
int W1, W2;

int S_H1, S_H2, S_H3, S_H4;  //  Stored hour
int S_M1, S_M2, S_M3, S_M4;  //  Stored minute 
int S_W1, S_W2;              //  Stored water, turbidty

int modMenu = 0;
int timeMenu = 0;
int waterMenu = 0;

int flagMenu = 0;
int flagTime = 0;
int flagWater = 0;

int flow = 0;

bool wasError(const char* errorTopic = "")
{
    uint8_t error = Rtc.LastError();
    if (error != 0)
    {
        Serial.print("[");
        Serial.print(errorTopic);
        Serial.print("] WIRE communications error (");
        Serial.print(error);
        Serial.print(") : ");

        switch (error)
        {
        case Rtc_Wire_Error_None:
            Serial.println("(none?!)");
            break;
        case Rtc_Wire_Error_TxBufferOverflow:
            Serial.println("transmit buffer overflow");
            break;
        case Rtc_Wire_Error_NoAddressableDevice:
            Serial.println("no device responded");
            break;
        case Rtc_Wire_Error_UnsupportedRequest:
            Serial.println("device doesn't support request");
            break;
        case Rtc_Wire_Error_Unspecific:
            Serial.println("unspecified error");
            break;
        case Rtc_Wire_Error_CommunicationTimeout:
            Serial.println("communications timed out");
            break;
        }
        return true;
    }
    return false;
}

void readMemory()
{
  S_H1 = EEPROM.read(hour[0]);
  S_M1 = EEPROM.read(minute[0]);
  S_H2 = EEPROM.read(hour[1]);
  S_M2 = EEPROM.read(minute[1]);
  S_H3 = EEPROM.read(hour[2]);
  S_M3 = EEPROM.read(minute[2]);
  S_H4 = EEPROM.read(hour[3]);
  S_M4 = EEPROM.read(minute[3]);
  S_W1 = EEPROM.read(water[0]);
  S_W2 = EEPROM.read(water[1]);
}

void modButton()
{
  if (digitalRead(MOD) == LOW)
  {
    delay(100);
    modMenu = (modMenu + 1) % 3;
    //while (digitalRead(MOD) == LOW);
    sound();
  }

  if (modMenu == 1)
  {
    flagMenu = 1;
  }
  else if (modMenu == 2)
  {
    flagMenu = 2;
  }
  else if (modMenu == 0)
  {
    flagMenu = 0;
  }
}

void timeButton()
{
   if (digitalRead(TIME) == LOW)
  {
    delay(100);
    timeMenu = (timeMenu + 1) % 9;
    //while (digitalRead(TIME) == LOW);
    sound();
  }

  if (timeMenu == 1)
  { 
    lcd.clear();
    EEPROM.update(hour[0], H1);
    flagTime = 1;
  }
  else if (timeMenu == 2)
  { 
    EEPROM.update(minute[0], M1);
    flagTime = 2;
  }
  else if (timeMenu == 3)
  {
    EEPROM.update(hour[1], H2);
    flagTime = 3;
  }
  else if (timeMenu == 4)
  {
    EEPROM.update(minute[1], M2);
    flagTime = 4;
  }
  else if (timeMenu == 5)
  {
    EEPROM.update(hour[2], H3);
    flagTime = 5;
  }
  else if (timeMenu == 6)
  {
    EEPROM.update(minute[2], M3);
    flagTime = 6;
  }
  else if (timeMenu == 7)
  {
    EEPROM.update(hour[3], H4);
    flagTime = 7;
  }
  else if (timeMenu == 8)
  {
    EEPROM.update(minute[3], M4);
    flagTime = 8;
  }
  else if (timeMenu == 0)
  {
    flagTime = 9;
  } 
} 

void waterButton()
{
  if (digitalRead(WATER) == LOW)
  {
    delay(100);
    waterMenu = (waterMenu + 1) % 3;
    //while (digitalRead(WATER) == LOW);
    sound();
  }

  if (waterMenu == 1)
  { 
    EEPROM.update(water[0], W1);
    flagWater = 1;
  }
  else if (waterMenu == 2)
  {
    EEPROM.update(water[1], W2);
    flagWater = 2;
  }
  else if (waterMenu == 0)
  {
    flagWater = 0;
  }
}

void sound()
{
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
  delay(100);
}

void setup()
{
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  pinMode(TIME, INPUT_PULLUP);
  pinMode(WATER, INPUT_PULLUP);
  pinMode(MOD, INPUT_PULLUP);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(TURBIDTY_PIN, INPUT);

  pinMode(relayIN1, OUTPUT);
  pinMode(relayIN2, OUTPUT);
  digitalWrite(relayIN2, LOW);
  pinMode(relayIN3, OUTPUT);
  digitalWrite(relayIN3, LOW);

  pinMode(potPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  Rtc.Begin();
#if defined(WIRE_HAS_TIMEOUT)
    Wire.setWireTimeout(3000 /* us */, true /* reset_on_timeout */);
#endif

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDateTime(compiled);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) 
    {
        if (!wasError("setup IsDateTimeValid"))
        {
            // Common Causes:
            //    1) first time you ran and the device wasn't running yet
            //    2) the battery on the device is low or even missing

            Serial.println("RTC lost confidence in the DateTime!");

            // following line sets the RTC to the date & time this sketch was compiled
            // it will also reset the valid flag internally unless the Rtc device is
            // having an issue

            Rtc.SetDateTime(compiled);
        }
    }

    if (!Rtc.GetIsRunning())
    {
        if (!wasError("setup GetIsRunning"))
        {
            Serial.println("RTC was not actively running, starting now");
            Rtc.SetIsRunning(true);
        }
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (!wasError("setup GetDateTime"))
    {
        if (now < compiled)
        {
            Serial.println("RTC is older than compile time, updating DateTime");
            Rtc.SetDateTime(compiled);
        }
        else if (now > compiled)
        {
            Serial.println("RTC is newer than compile time, this is expected");
        }
        else if (now == compiled)
        {
            Serial.println("RTC is the same as compile time, while not expected all is still fine");
        }
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.Enable32kHzPin(false);
    wasError("setup Enable32kHzPin");
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
    wasError("setup SetSquareWavePin");
}

void loop()
{
  if (!Rtc.IsDateTimeValid()) 
    {
        if (!wasError("loop IsDateTimeValid"))
        {
            // Common Causes:
            //    1) the battery on the device is low or even missing and the power line was disconnected
            Serial.println("RTC lost confidence in the DateTime!");
        }
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (!wasError("loop GetDateTime"))
    {
        printDateTime(now);
        Serial.println();
    }

  int currentHour = now.Hour();
  int currentMinute = now.Minute();

  modButton();
  timeButton();
  waterButton();
  readMemory();

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseInLong(ECHO_PIN, HIGH, 30000);
  int distance = (duration * 0.034) / 2;

  int absWaterLevel = constrain(distance, 0, 65);

  int potValue = analogRead(potPin);

  int sensorValue = analogRead(TURBIDTY_PIN);
  float turbidty = sensorValue * (5.0 / 1023.0);

  // Time
  if (flagMenu == 1 && flagTime == 1)
  {
    H1 = constrain(map(potValue, 0, 1023, 23, 0), 0, 23);
  }
  else if (flagMenu == 1 && flagTime == 2)
  {
    M1 = constrain(map(potValue, 0, 1023, 59, 0), 0, 59);
  }
  else if (flagMenu == 1 && flagTime == 3)
  {
    H2 = constrain(map(potValue, 0, 1023, 23, 0), 0, 23);
  }
  else if (flagMenu == 1 && flagTime == 4)
  {
    M2 = constrain(map(potValue, 0, 1023, 59, 0), 0, 59);
  }
  else if (flagMenu == 1 && flagTime == 5)
  {
    H3 = constrain(map(potValue, 0, 1023, 23, 0), 0, 23);
  }
  else if (flagMenu == 1 && flagTime == 6)
  {
    M3 = constrain(map(potValue, 0, 1023, 59, 0), 0, 59);
  }
  else if (flagMenu == 1 && flagTime == 7)
  {
    H4 = constrain(map(potValue, 0, 1023, 23, 0), 0, 23);
  }
  else if (flagMenu == 1 && flagTime == 8)
  {
    M4 = constrain(map(potValue, 0, 1023, 59, 0), 0, 59);
  }

  //  Water
  if (flagMenu == 2 && flagWater == 1)
  {
    W1 = constrain(map(potValue, 0, 1023, 65, 0), 0, 65);
  }
  else if (flagMenu == 2 && flagWater == 2)
  {
    W2 = constrain(map(potValue, 0, 1023, 5, 0), 0, 5);
  }

//////////////////////////////////
  if (modMenu == 0)
  { 
    lcd.clear();          //  Time
    lcd.setCursor(0, 0);
    if (S_H1 < 10) lcd.print("0");
    lcd.print(S_H1);
    lcd.print(":");
    if (S_M1 < 10) lcd.print("0");
    lcd.print(S_M1);
    lcd.setCursor(0, 1);
    if (S_H2 < 10) lcd.print("0");
    lcd.print(S_H2);
    lcd.print(":");
    if (S_M2 < 10) lcd.print("0");
    lcd.print(S_M2);
    lcd.setCursor(0, 2);
    if (S_H3 < 10) lcd.print("0");
    lcd.print(S_H3);
    lcd.print(":");
    if (S_M3 < 10) lcd.print("0");
    lcd.print(S_M3);
    lcd.setCursor(0, 3);
    if (S_H4 < 10) lcd.print("0");
    lcd.print(S_H4);
    lcd.print(":");
    if (S_M4 < 10) lcd.print("0");
    lcd.print(S_M4);

    //  water level
    lcd.setCursor(7, 0);
    lcd.print("Water: ");
    lcd.print(absWaterLevel);
    lcd.print(" cm ");
    lcd.setCursor(7, 1);
    lcd.print("Set: ");
    lcd.print(S_W1);
    lcd.print(" cm ");
    lcd.setCursor(7, 2);
    lcd.print("Turbi: ");
    lcd.print(turbidty, 2);
    lcd.setCursor(7, 3);
    lcd.print("Set: ");
    lcd.print(S_W2);
  }
  
/////////////////////////////////////
  if (modMenu == 1)
  { 
    lcd.clear(); 
    lcd.setCursor(0, 0);
    if (H1 < 10) lcd.print("0");
    lcd.print(H1);
    lcd.print(":");
    if (M1 < 10) lcd.print("0");
    lcd.print(M1);
    lcd.setCursor(0, 1);
    if (H2 < 10) lcd.print("0");
    lcd.print(H2);
    lcd.print(":");
    if (M2 < 10) lcd.print("0");
    lcd.print(M2);
    lcd.setCursor(0, 2);
    if (H3 < 10) lcd.print("0");
    lcd.print(H3);
    lcd.print(":");
    if (M3 < 10) lcd.print("0");
    lcd.print(M3);
    lcd.setCursor(0, 3);
    if (H4 < 10) lcd.print("0");
    lcd.print(H4);
    lcd.print(":");
    if (M4 < 10) lcd.print("0");
    lcd.print(M4);

    lcd.setCursor(7, 3);
    lcd.print("TIME: ");
    if (currentHour < 10) lcd.print("0");
    lcd.print(currentHour);
    lcd.print(":");
    if (currentMinute < 10) lcd.print("0");
    lcd.print(currentMinute);
  }

////////////////////////////////////////////////////
  if (modMenu == 2)
  {
    lcd.clear(); 
    lcd.setCursor(0, 1);
    lcd.print("Set Water: ");
    if (W1 < 10) lcd.print(" ");
    lcd.print(W1);
    lcd.print(" cm ");
    lcd.setCursor(0, 2);
    lcd.print("Set Turbity: ");
    if (W2 < 10) lcd.print(" ");
    lcd.print(W2);
  }

/////////////////////////////////////////////////////
  if ((currentHour == S_H1 && currentMinute == S_M1 && now.Second() == 0) ||
      (currentHour == S_H2 && currentMinute == S_M2 && now.Second() == 0) ||
      (currentHour == S_H3 && currentMinute == S_M3 && now.Second() == 0) ||
      (currentHour == S_H4 && currentMinute == S_M4 && now.Second() == 0)) 
    {
      
      digitalWrite(relayIN1, HIGH); // Turn on relay
      Serial.println("Relay ON!");
      delay(1000);
      digitalWrite(relayIN1, LOW);
    }

if (turbidty < S_W2 && flow == 0)
{
  digitalWrite(relayIN3, LOW);
  digitalWrite(relayIN2, HIGH);

  flow = 1;
}
 if (absWaterLevel >= S_W1 && flow == 1)
  {
    digitalWrite(relayIN3, HIGH);
    digitalWrite(relayIN2, LOW);
    
    flow = 2;
  }
  if (absWaterLevel <= 10 && flow == 2)
  {
    digitalWrite(relayIN3, LOW);
    digitalWrite(relayIN2, LOW);
    
    flow = 0;
  }

  Serial.print("MOD: ");
  Serial.print(flagMenu);
  Serial.print(" || TIME: ");
  Serial.print(timeMenu);
  Serial.print(" || WATER: ");
  Serial.println(waterMenu);

  Serial.print("Turbidty: ");
  Serial.println(turbidty);

  Serial.println(S_W2);

  delay(200);
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
    char datestring[26];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}
