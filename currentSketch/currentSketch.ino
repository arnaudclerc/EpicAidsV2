
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <EEPROM.h>
#include <DS3231.h>

#define IRQ   (2)
#define RESET (3)

Adafruit_PN532 nfc(IRQ, RESET);
DS3231  rtc(SDA, SCL);

boolean add = false;
uint8_t ite = 0;
uint8_t admin[] = { 1, 114, 25, 170 };
Time  t;

int out1 = 9;
int out2 = 6;
float V1 = 0;
float V2 = 0;

struct ms
{
    uint8_t  content[7];
};

int cc(uint8_t a[], uint8_t b[], uint8_t l)
{
    for(int i = 0; i < l; i++)
    {
        if (a[i] != b[i])
            return (1);
    }
    return (0); 
}

void removeEEPROM(uint8_t i)
{
    uint8_t  tmp[7];
    uint8_t  stk;
    
    for (int j = 0; j < 7; j++)
        EEPROM.write(i * 7 + j, 0);
    while (++i < ite)
    {
        for (int j = 0; j < 7; j++)
        {
            stk = EEPROM.read(i * 7 + j);
            EEPROM.write(i * 7 + j, 0);
            EEPROM.write((i - 1) * 6 + j, stk);
        }
    }
    --ite;
}

struct ms  getEEPROM(uint8_t i)
{
    struct ms    lol;

    for (int j = 0; j < 7; j++)
          lol.content[j] = EEPROM.read(i * 7 + j);
    return (lol);
}

void saveEEPROM(uint8_t val[])
{
    for (int i = 0; i < 7; i++)
        EEPROM.write(7 * ite + i, val[i]);
    ++ite;
}


void chargeEEPROM()
{
  uint8_t list[][7] = {
    { 4, 62, 97, 50, 198, 78, 128 },
    { 4, 95, 168, 50, 198, 78, 128 }
  };
  
  uint8_t valid = EEPROM.read(0);
  
  if (valid == 0)
  {
      for (int i = 0; i < 256; i++)
          EEPROM.write(i, 0);
      saveEEPROM(list[0]);
      saveEEPROM(list[1]);
  }
  else
  {
      int i = 0;
      while (EEPROM.read(i++))
          if (i % 6 == 0 && i != 0)
              ++ite;
  }
}


void setup(void)
{
  Serial.begin(115200);
  delay(50);
  nfc.begin();
  rtc.begin();
  pinMode(12, INPUT_PULLUP);
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
      while (1);
  nfc.setPassiveActivationRetries(0x01);
  nfc.SAMConfig();
  attachInterrupt(0, onInterrupt, RISING);
  chargeEEPROM();
}

void    call_one(int pin)
{
                digitalWrite(pin, LOW);
                delay(150);
                digitalWrite(pin, HIGH);       
}

void onInterrupt()
{
    if (digitalRead(12) == LOW)
    {
        delay(1000);
        for (int i = 0 ; i < 256 ; i++)
            EEPROM.write(i, 0);
    }
}


void loop(void)
{  
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  struct ms      lol;
  
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
  
    //V1 = analogRead(A5) * (5 / 1023.0);
    V2 = analogRead(A0) * (5 / 1023.0);
          
    if (V1 >= 0.3) 
        call_one(out1);
          
    if (V2 >= 0.3) 
        call_one(out2);
          
    delay(2);

    if (success && uidLength == 4 && cc(uid, admin, uidLength) == 0)
    {
        add = !add;
        delay(800);
    }
    
    if (success && uidLength == 7)
    {
        if (add == 0)
        {
            for (uint8_t i = 0 ; i <= 256; i+=2)
            {
                lol = getEEPROM(i);
                if (cc(lol.content, uid, uidLength) == 0)
                {
                    call_one(out2);
                    delay(500);
                    break ; 
                }
            }
        }
        else if (add == 1)
        {
            for (uint8_t i = 0; i <= 256; i+=2)
            {
                lol = getEEPROM(i);
                if (cc(lol.content, uid, uidLength) == 0)
                {
                    add = false;
                    removeEEPROM(i);
                    delay(500);
                    break ;
                }
            }
            if (add == 1)
            {
                t = rtc.getTime();
                add = false;
                delay(500);
                saveEEPROM(uid);
            }
        }
    }
}




/*
  Some bullshit going on
  
  Working on it
  t = rtc.getTime();
  
  // Send date over serial connection
  Serial.print(":");
  Serial.print(t.date, DEC);
  Serial.print(". ");
  Serial.print(rtc.getMonthStr());
  Serial.print(" . ");
  Serial.print(t.year, DEC);
  Serial.print(" --- ");
  
  // Send Day-of-Week and time
  Serial.print("");
  Serial.print(t.dow, DEC);
  Serial.print("th. ");
  Serial.print(t.hour, DEC);
  Serial.print("h");
  Serial.print(t.min, DEC);
  Serial.print("m");
  Serial.print(t.sec, DEC);
  Serial.println("s"); */

