
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <EEPROM.h>
#include <Time.h>
#include <DS3231.h>

#define IRQ   (2)
#define RESET (3)

#define ShipMEM (2048) // Taille de l'EEPROM de la carte courant 4096bits pour la mega ADK
#define DUREE (518400)


Adafruit_PN532      nfc(IRQ, RESET);      // Commutation en I2C avec le shield NFC
DS3231              rtc(SDA, SCL);        // Commutation I2C pour le shield clock (2nd slave)

boolean             add = false;
uint8_t             ite = 0;
uint8_t             admin[] = { 1, 114, 25, 170 };
Time                time;

int                 out1 = 9;
int                 out2 = 6;
float               V1 = 0;
float               V2 = 0;

int                 led = 19;
uint8_t             bzez;

struct ms
{
        uint8_t     content[7];                   // struct to return each uid in memory and compare to the detected one
};


int                 cc(uint8_t a[], uint8_t b[], uint8_t l)
{
        for(int i = 0; i < l; i++)
        {
                if (a[i] != b[i])                 // compare to find the corresponding if exists
                        return (1);
        }
        return (0); 
}

void                removeEEPROM(uint8_t i)
{
        for (int j = 1; j <= 11; j++)
                EEPROM.write(i * 11 + j, 0);      // Remove even if rewritten aftermath, it could be the last in the list
        while (++i < ite)
        {
                for (int j = 1; j <= 11; j++)
                {
                        EEPROM.write((i - 1) * 11 + j, EEPROM.read(i * 11 + j));    //replace to not leave a hole in the list
                        EEPROM.write(i * 11 + j, 0);
                }
        }
        ite = ite > 0 ? ite - 1 : 0;
}

struct ms           getEEPROM(uint8_t i)
{
        struct ms   lol;

        for (int j = 1; j <= 7; j++)
                lol.content[j - 1] = EEPROM.read(i * 11 + j);    // get each member to check if user is registered   
        return (lol);
}

void                saveEEPROM(uint8_t val[], int force)
{
        struct ms   Fecha = makeTimeStamp(force);

        for (int i = 1; i <= 7; i++)
                EEPROM.write(ite * 11 + i, val[i - 1]);    // save a new user
        EEPROM.write((11 * (ite + 1)) - 3, Fecha.content[0]);
        EEPROM.write((11 * (ite + 1)) - 2, Fecha.content[1]);
        EEPROM.write((11 * (ite + 1)) - 1, Fecha.content[2]);
        EEPROM.write((11 * (ite + 1)), Fecha.content[3]);
        ++ite;
        Serial.println("created member");
        
}

void                 chargeEEPROM()
{
        uint8_t      list[][7] = {
                { 4, 62, 97, 50, 198, 78, 128 },
                { 4, 95, 168, 50, 198, 78, 128 },
                { 4, 244, 140, 50, 198, 78, 128 }
        };

        ite = 0;
        add = 0;
        if (EEPROM.read(0) == 0)                 // fill a first time the memory
        {
                for (int i = 0; i <= ShipMEM; i++)
                        EEPROM.write(i, 0);
                EEPROM.write(0, (uint8_t) 42);             // says that memory isn't empty
                saveEEPROM(list[0], 31536000);
                saveEEPROM(list[1], 31536000);
                saveEEPROM(list[2], 31536000);
        }
        else
        {
                int i = 1;
                while (EEPROM.read(i++))
                        if (i % 11 == 0)         // count number of registered member at boot time
                                ++ite;
                Serial.print("Welcome Back, There is ");
                Serial.print(ite);
                Serial.println(" registered yet");
        }
}

void                 setup(void)
{
        Serial.begin(115200);
        delay(50);
        pinMode(led, OUTPUT);  
        pinMode(out1, OUTPUT);
        pinMode(out2, OUTPUT);
        digitalWrite(out1, HIGH);
        digitalWrite(out2, HIGH);
        nfc.begin();
        rtc.begin();
        pinMode(12, INPUT_PULLUP);
        uint32_t versiondata = nfc.getFirmwareVersion();  // setup things for shields I2C comm..
        if (!versiondata)                                 // passive reading, interrupt funct
                while (1);
        nfc.setPassiveActivationRetries(0x01);
        nfc.SAMConfig();
        attachInterrupt(0, onInterrupt, RISING);
        chargeEEPROM();
}

void                 call_one(int pin)
{
        digitalWrite(pin, LOW);
        delay(150);
        digitalWrite(pin, HIGH);       
        blink(1, 500);
}

int                  retrieveM(char *mth)
{
        char         existing[13][10] = {
                "January",
                "February",
                "March",
                "April",
                "May",
                "June",
                "July",
                "August",
                "September",
                "October",
                "November",
                "December",
        };
        
        for (int i = 0; i < 12; i++)
        {
                if (strcmp(mth, existing[i]) == 0)
                        return (i + 1);
        }
}

struct ms          makeTimeStamp(int force)
{
        struct ms  data;
        long int   eeppoo;

        time = rtc.getTime();
        setTime(time.hour, time.min, time.sec, time.date, retrieveM(rtc.getMonthStr()), time.year);
        eeppoo = now() + force;
        data.content[0] = (eeppoo >> 24);
        data.content[1] = (eeppoo >> 16);
        data.content[2] = (eeppoo >> 8);
        data.content[3] = (eeppoo >> 0);
        return (data);      // get Epoch from current time (unix timestamp) plus one week in seconds
}

void                 remove_expired()
{
        long int     Fecha;
        
        time = rtc.getTime();
        setTime(time.hour, time.min, time.sec, time.date, retrieveM(rtc.getMonthStr()), time.year);
        for (int i = 8; i < ShipMEM;)
        {
                Fecha = EEPROM.read(i);
                Fecha = Fecha << 8;
                Fecha += EEPROM.read(i + 1);
                Fecha = Fecha << 8;
                Fecha += EEPROM.read(i + 2);
                Fecha = Fecha << 8;
                Fecha += EEPROM.read(i + 3);
                Serial.print("LE ici ");
                Serial.println(Fecha);
                if (Fecha == 0)
                        return ;
                if (Fecha < now())
                        removeEEPROM(i - 7);
                i += 11;
        }
}

void                 onInterrupt()
{
        //V1 = analogRead(A5) * (5 / 1023.0);
        V2 = analogRead(A0) * (5 / 1023.0);
          
        if (V1 >= 0.3) 
                call_one(out1);
          
        if (V2 >= 0.3) 
                call_one(out2);
}

void                 loop(void)
{  
        uint8_t      success;
        uint8_t      uid[] = { 0, 0, 0, 0, 0, 0, 0 };
        uint8_t      uidLength;
        struct ms    lol;
  
        success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
 
//        DEBUG();      

        if (digitalRead(12) == LOW)
        {
                Serial.println("RESET");      
                blink(2, 1000);
                EEPROM.write(0, 0);
                chargeEEPROM();
        }
   
        delay(2);
        time = rtc.getTime();
        
        if (time.min % 10 == 0 && time.sec == 00)
                remove_expired();
        if (time.sec == bzez)
        {
                ++bzez;
                digitalWrite(led, HIGH);
                digitalWrite(led, LOW);
          
        }
        if (success && uidLength == 4 && cc(uid, admin, uidLength) == 0)
        {
                add = add == 1 ? 0 : 1;      
                blink(2, 800);
        }
    
        if (success && uidLength == 7)
        {
                if (add == 0)
                {
                        for (uint8_t i = 0; i <= ShipMEM / 11; i++)
                        {
                                lol = getEEPROM(i);
                                if (cc(lol.content, uid, uidLength) == 0)
                                {
                                        call_one(out2);      
                                        blink(1, 500);
                                        break ; 
                                }
                        }
                }
                else if (add == 1)
                {
                        for (uint8_t i = 0; i <= ShipMEM / 11; i++)
                        {
                                lol = getEEPROM(i);
                                if (cc(lol.content, uid, uidLength) == 0)
                                {
                                        add = false;
                                        removeEEPROM(i);
                                        blink(3, 500);
                                        break ;
                                }
                        }
                        if (add == 1)
                        {
                                time = rtc.getTime();
                                add = false;
                                blink(2, 500);
                                saveEEPROM(uid, DUREE);
                        }
                }
        }
}

void         blink(int mode, int duree)
{
        digitalWrite(led, HIGH);
        delay(70 + mode == 2 ? 100 : mode == 1 ? 200 : 0);
        digitalWrite(led, LOW);
        if (mode == 1)
        {
                delay(duree / 3);
                digitalWrite(led, HIGH);
                delay(70);
                digitalWrite(led, LOW);
                delay(duree / 3);
        }
        if (mode == 2)
        {
                delay(duree / 2);
                digitalWrite(led, HIGH);
                delay(70);
                digitalWrite(led, LOW);
                delay(duree / 4);
                digitalWrite(led, HIGH);
                delay(70);
                digitalWrite(led, LOW);
                delay(duree / 3);
        }
        if (mode == 3)
        {
                delay(duree / 4);
                digitalWrite(led, HIGH);
                delay(70);
                digitalWrite(led, LOW);
                delay(duree / 4);
                digitalWrite(led, HIGH);
                delay(70);
                digitalWrite(led, LOW);
                delay(duree / 4);
        }
        digitalWrite(led, HIGH);
        delay(70);
        digitalWrite(led, LOW);
}

void DEBUG()
{
  int i = 0;
  uint8_t val;
   Serial.println("DD");
  while(val = EEPROM.read(i++))
  {
   Serial.print("index= ");
   Serial.print(i);
   Serial.print(": ");
   Serial.print(val); 
   Serial.print(" soit ");
   Serial.println(val, BIN); 
  }

   Serial.print("ite = ");
   Serial.print(ite);  
   Serial.print(" 0= ");  
   Serial.println(EEPROM.read(0));  
}

