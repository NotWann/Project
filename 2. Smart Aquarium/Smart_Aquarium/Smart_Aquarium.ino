#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

const int buttonNext = 6;
const int buttonSelect = 7;
const int buttonSave = 8;

int currentItem = 0;
int previousItem = 0;

const int firstMenuTotal = 2;
String firstMenuItems[] = {
  "1. Set Water Level",
  "2. Set Time",
};

int waterLevel = 30;
int targetLevel = 50;

void setup()
{
  Serial.begin(115200);
  
  lcd.clear();
  lcd.init();
  lcd.backlight();

  pinMode(buttonNext, INPUT_PULLUP);
  pinMode(buttonSave, INPUT_PULLUP);
  pinMode(buttonSelect, INPUT_PULLUP);

  mainScreen();
}

void loop()
{
  if (!digitalRead(buttonSelect))
  {
    select();
    delay(100);
  }
}

void mainScreen()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Level: ");
  lcd.print(waterLevel);
  lcd.print(" cm ");

  lcd.setCursor(0, 1);
  lcd.print("Target: ");
  lcd.print(targetLevel);
  lcd.print(" cm ");

  lcd.setCursor(0,3);
  lcd.print(">");
  lcd.setCursor(1,3);
  lcd.print("EDIT");
}

void select()
{
  lcd.clear();
  lcd.setCursor(0, 0);

  firstMenu();
  delay(200);

  if (!digitalRead(buttonSelect))
  {
    delay(200)
    mainScreen();
  }
  
}

void firstMenu()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Choose: ");

  for (int i = 0; i < 2 && i < firstMenuItems; i++)
  {
    lcd.setCursor(1, i + 1);
    lcd.print(firstMenuItems[(currentItem + i) % firstMenuTotal]);
  }
  lcd.setCursor(0, 1);
  lcd.print(">");
}