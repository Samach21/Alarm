#include <Arduino.h>
#include <Wire.h>
#include "hd44780.h"
#include "hd44780_I2Cexp.h"

hd44780_I2Cexp lcd;

void pushButton1();
void pushButton2();
void pushButton3();
void pushButton4();
void pushButton5();
void writeEEPROM_page(int, unsigned int , byte* , byte);
void readEEPROM_page(int , unsigned int , byte*, int);
float tempOUT();

const int LCD_COLS = 16;
const int LCD_ROWS = 2;

int sec = 0;
int minutes = 0;
int hours = 0;

int preminutes = 0;
int prehours = 0;
char preEEprom[5];
int dataNOW = 0;

int readtemp = 0;

byte deviceAddress;
unsigned int eepromAddress;

const int BUTTON1 = 2;
const int BUTTON2 = 3;
const int BUTTON3 = 4;
const int BUTTON4 = 5;
const int BUTTON5 = 6;
const int temp = A0;
const int buzzer = 10;
const int IR = 9;

int page = 1;
bool Check = false;
bool changeH = true;
bool clear = false;
bool setTime = false;
int setTimeint = 1;
bool firstON = false;

bool alarmON = false;
String page3Show[2];
bool load = false;
int showN = 0;
unsigned long a = 0;

unsigned long debounceDelay = 50;
int buttonState1;
int lastButtonState1 = HIGH;
unsigned long lastDebounceTime1 = 0;
int buttonState2;
int lastButtonState2 = HIGH;
unsigned long lastDebounceTime2 = 0;
int buttonState3;
int lastButtonState3 = HIGH;
unsigned long lastDebounceTime3 = 0;
int buttonState4;
int lastButtonState4 = HIGH;
unsigned long lastDebounceTime4 = 0;
int buttonState5;
int lastButtonState5 = HIGH;
unsigned long lastDebounceTime5 = 0;

byte up[] = {
  0x00,
  0x04,
  0x0E,
  0x1F,
  0x04,
  0x04,
  0x04,
  0x00
};

byte down[] = {
  0x00,
  0x04,
  0x04,
  0x04,
  0x1F,
  0x0E,
  0x04,
  0x00
};

byte customChar[] = {
  0x00,
  0x00,
  0x0E,
  0x13,
  0x15,
  0x11,
  0x0E,
  0x00
};

ISR(TIMER1_COMPA_vect)
{
   sec++;
   if (sec >= 60)
   {
     minutes++;
     sec = 0;
     Check = true;
   }
   if (minutes >= 60)
   {
     hours++;
     minutes = 0;
   }
}

void setup()
 {
   Wire.begin();
   pinMode(BUTTON1, INPUT_PULLUP);
   pinMode(BUTTON2, INPUT_PULLUP); 
   pinMode(BUTTON3, INPUT_PULLUP);
   pinMode(BUTTON4, INPUT_PULLUP); 
   pinMode(BUTTON5, INPUT_PULLUP);
   pinMode(buzzer, OUTPUT);
   pinMode(temp, INPUT);
   pinMode(IR, INPUT);
   deviceAddress = 0x50;
   int status;
   status = lcd.begin(LCD_COLS, LCD_ROWS);
   if(status)
   {
     hd44780::fatalError(status);
   }
   Serial.begin(115200);
   noInterrupts(); 
   TCCR1A = 0; 
   TCCR1B = 0;
   TCNT1 = 0;
   OCR1A = 15624; 
   TCCR1B |= (1 << WGM12); 
   TCCR1B |= (1 << CS12) | (1 << CS10); 
   TIMSK1 |= (1 << OCIE1A);
   interrupts();
   lcd.createChar(0, up);
   lcd.createChar(1, down);
   lcd.createChar(2, customChar);
}

void loop() 
{
  if (tempOUT() >= 35.f && alarmON == false)
  {
    tone(buzzer, 250, 5);
    lcd.setCursor(4,0);
    lcd.print("warning!");
    lcd.setCursor(4,1);
    lcd.print("highTEMP");
  }
  else if (tempOUT() < 35.f && alarmON == false) {
  if (page == 1)
  {
    if (a == 1)
    {
      lcd.clear();
      showN = 0;
      a++;
    }
    if (setTime)
    {
      switch (setTimeint)
      {
      case 1:
        lcd.setCursor(5,0);
        lcd.print("v");
        break;
      case 2:
        lcd.setCursor(8,0);
        lcd.print("v");
        break;
      case 3:
        lcd.setCursor(11,0);
        lcd.print("v");
        break;
      default:
        break;
      }
      pushButton2();
      pushButton3();
      pushButton4();
      pushButton5();
    }
    else
    {
      lcd.setCursor(4,0);
      lcd.print("         ");
      if (dataNOW != 0) {
        pushButton1();
        lcd.setCursor(15,0);
        lcd.write(2);
      }
      pushButton2();
      pushButton5();
    }
    String H, M, S;
    lcd.setCursor(4,1);
    if (sec < 10)
      S = "0" + (String)sec;
    else
      S = (String)sec;
    if (minutes < 10)
      M = "0" + (String)minutes;
    else
      M = (String)minutes;
    if (hours < 10)
      H = "0" + (String)hours;
    else
      H = (String)hours;
    lcd.print(H + ":" + M + ":" + S);
  }
  else if (page == 2)
  {
    pushButton1();
    pushButton2();
    pushButton3();
    pushButton4();
    pushButton5();
    if (changeH)
    {
      lcd.setCursor(7,0);
      lcd.print("v");
    }
    else
    {
      lcd.setCursor(11,0);
      lcd.print("v");
    }
    String H, M;
    lcd.setCursor(4,1);
    if (preminutes < 10)
      M = "0" + (String)preminutes;
    else
      M = (String)preminutes;
    if (prehours < 10)
      H = "0" + (String)prehours;
    else
      H = (String)prehours;
    lcd.print("H:" + H + "M:" + M);
  }
  else if (page == 3)
  {
    if (load)
    {
      lcd.clear();
      String alaemTime[dataNOW];
      for (int i = 0, j = 0; i < dataNOW; i++, j += 16)
      {
        char buffer[6];
        if (i < 3)
          readEEPROM_page(deviceAddress, j, (byte *)buffer, 6);
        if (i >= 3)
          readEEPROM_page(deviceAddress, j, (byte *)buffer, 5);
        alaemTime[i] = buffer;
      }
      page3Show[0] = alaemTime[showN];
      page3Show[1] = alaemTime[showN + 1];
      load = !load;
    }
    pushButton1();
    pushButton2();
    if (showN < (dataNOW - 2))
    {
      pushButton4();
    }
    if (showN > 0)
    {
      pushButton3();
    }
    if (dataNOW != 0) {
    lcd.setCursor(0,0);
    lcd.write(0);
    lcd.print((String)(showN + 1) + ": ");
    lcd.print(page3Show[0]);
    if (dataNOW >= 2)
    {
      lcd.setCursor(0,1);
      lcd.write(1);
      lcd.print((String)(showN + 2) + ": ");
      lcd.print(page3Show[1]);
    }
    }
  }
  if (Check)
  {
    String H, M;
    if (minutes < 10)
      M = "0" + (String)minutes;
    else
      M = (String)minutes;
    if (hours < 10)
      H = "0" + (String)hours;
    else
      H = (String)hours;
    String realTime = H + ":" + M;
    String alaemTime[dataNOW];
    for (int i = 0, j = 0; i < dataNOW; i++, j += 16)
    {
      char buffer[6];
      if (i < 3)
          readEEPROM_page(deviceAddress, j, (byte *)buffer, 6);
        if (i >= 3)
          readEEPROM_page(deviceAddress, j, (byte *)buffer, 5);
      alaemTime[i] = buffer;
      Serial.println(alaemTime[i]);
      if (realTime == alaemTime[i])
      {
        alarmON = true;
        lcd.clear();
        firstON = true;
        break;
      }
    }
    Check = !Check;
  }
  }
  if (alarmON)
  {
    if (firstON)
    {
      lcd.clear();
      Wire.beginTransmission(8);
      //Wire.write("x is ");
      Wire.write(1);
      Wire.endTransmission();
      firstON = !firstON;
    }
    tone(buzzer, 523, 5);
    lcd.setCursor(4,0);
    lcd.print("TIME TO!");
    lcd.setCursor(3,1);
    lcd.print("MEDICINES!");
    if (digitalRead(IR) == LOW)
    {
      alarmON = false;
      lcd.clear();
    }
  }
}

void pushButton1()
{
  int reading = digitalRead(BUTTON1);
  if (reading != lastButtonState1) {
    lastDebounceTime1 = millis();
  }
  if ((millis() - lastDebounceTime1) > debounceDelay) {
    if (reading != buttonState1) {
      buttonState1 = reading;

      if (buttonState1 == LOW) {
        if (page == 1)
        {
          load = true;
          page = 3;
        }
        else if (page == 2)
        {
          page = 1;
          lcd.clear();
          String H, M, all;
          if (prehours < 10)
            H = "0" + (String)prehours;
          else
            H = (String)prehours;
          if (preminutes < 10)
            M = "0" + (String)preminutes;
          else
            M = (String)preminutes;
          all = H + ":" + M;
          all.toCharArray(preEEprom, 6);
          Serial.println(preEEprom);
          writeEEPROM_page(deviceAddress, eepromAddress, (byte *)preEEprom, 5);
          eepromAddress += 16;
          dataNOW++;
          changeH = true;
        }
        else if (page == 3)
        {
          page = 1;
          a = 1;
        }
      }
    }
  }
  lastButtonState1 = reading;
}

void pushButton2()
{
  int reading = digitalRead(BUTTON2);
  if (reading != lastButtonState2) {
    lastDebounceTime2 = millis();
  }
  if ((millis() - lastDebounceTime2) > debounceDelay) {
    if (reading != buttonState2) {
      buttonState2 = reading;

      if (buttonState2 == LOW) {
        if (page == 2)
        {
          page = 1;
          lcd.clear();
        }
        else if (page == 3)
        {
          lcd.clear();
          dataNOW = 0;
          eepromAddress = 0;
        }
        else if (page == 1)
        {
          if (setTime)
          {
            setTimeint++;
            if (setTimeint > 3)
            {
              setTimeint = 1;
            }
            lcd.clear();
          }
          else if (!setTime)
          {
            setTime = !setTime;
          }
        }
      }
    }
  }
  lastButtonState2 = reading;
}

void pushButton3()
{
  int reading = digitalRead(BUTTON3);
  if (reading != lastButtonState3) {
    lastDebounceTime3 = millis();
  }
  if ((millis() - lastDebounceTime3) > debounceDelay) {
    if (reading != buttonState3) {
      buttonState3 = reading;

      if (buttonState3 == LOW) {
        if (changeH)
        {
          prehours++;
        }
        else
        {
          preminutes++;
        }
        if (preminutes > 59)
        {
          preminutes = 0;
        }
        if (prehours > 23)
        {
          prehours = 0;
        }
        if (setTime)
        {
          switch (setTimeint)
          {
          case 1:
            hours++;
            if (hours > 23)
            {
              hours = 0;
            }
            break;
          case 2:
            minutes++;
            if (minutes > 59)
            {
              minutes = 0;
            }
            break;
          case 3:
            sec++;
            if (sec > 59)
            {
              sec = 0;
            }
            break;
          default:
            break;
          }
        }
        if (page == 3)
        {
          showN--;
          load = true;
        }
      }
    }
  }
  lastButtonState3 = reading;
}

void pushButton4()
{
  int reading = digitalRead(BUTTON4);
  if (reading != lastButtonState4) {
    lastDebounceTime4 = millis();
  }
  if ((millis() - lastDebounceTime4) > debounceDelay) {
    if (reading != buttonState4) {
      buttonState4 = reading;

      if (buttonState4 == LOW) {
        if (changeH)
        {
          prehours--;
        }
        else
        {
          preminutes--;
        }
        if (preminutes < 0)
        {
          preminutes = 59;
        }
        if (prehours < 0)
        {
          prehours = 23;
        }
        if (setTime)
        {
          switch (setTimeint)
          {
          case 1:
            hours--;
            if (hours < 0)
            {
              hours = 23;
            }
            break;
          case 2:
            minutes--;
            if (minutes < 0)
            {
              minutes = 59;
            }
            break;
          case 3:
            sec--;
            if (sec < 0)
            {
              sec = 59;
            }
            break;
          default:
            break;
          }
        }
        if (page == 3)
        {
          showN++;
          load = true;
        }
      }
    }
  }
  lastButtonState4 = reading;
}

void pushButton5()
{
  int reading = digitalRead(BUTTON5);
  if (reading != lastButtonState5) {
    lastDebounceTime5 = millis();
  }
  if ((millis() - lastDebounceTime5) > debounceDelay) {
    if (reading != buttonState5) {
      buttonState5 = reading;

      if (buttonState5 == LOW) {
        if (page == 1)
        {
          if (setTime)
          {
            setTime = !setTime;
          }
          else if (!setTime)
          {
            page = 2;
            preminutes = 0;
            prehours = 0;
          }
        }
        else if (page == 2)
        {
          changeH = !changeH;
          lcd.clear();
        }
      }
    }
  }
  lastButtonState5 = reading;
}

void writeEEPROM_page(int device, unsigned int address, byte* buffer, byte length )
{
  byte i;
  Wire.beginTransmission(device | (int)(address >> 8));
  Wire.write((int)(address & 0xFF));
  for ( i = 0; i < length; i++)
  Wire.write(buffer[i]);
  Wire.endTransmission();
  delay(10);
}

void readEEPROM_page(int device, unsigned int address, byte *buffer, int length )
{
 byte i;
 Wire.beginTransmission(device | (int)(address >> 8));
 Wire.write((int)(address & 0xFF));
 Wire.endTransmission();
 Wire.requestFrom(device, length);
 for ( i = 0; i < length; i++ )
 if (Wire.available())
 buffer[i] = Wire.read();
}

float tempOUT()
{
  readtemp = analogRead(temp);
  float R1 = 10000;
  float logR2,R2,T;
  float c1 = 1.009249522e-03, c2 = 2.378405444e-04,c3 = 2.019202697e-07;
  R2 = R1 * (1023.0 / (float)readtemp - 1.0);
  logR2 = log(R2);
  T = (1.0/ (c1+c2 * logR2 + c3 * logR2 * logR2 * logR2 ));
  T = T - 273.15;
  return T;
}