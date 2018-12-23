
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <EEPROM.h>
#include <Time.h>
#include <DS3231.h>

#define IRQ   (2)
#define RESET (3)

#define ShipMEM (4096) // Taille de l'EEPROM de la carte courant 4096bits pour la mega ADK
#define REFRESH (45)   // temps en seconde ou la carte va faire le tri des abonnements expirés (x1000 ms)


Adafruit_PN532      nfc(IRQ, RESET);      // Commutation en I2C avec le shield NFC
DS3231              rtc(SDA, SCL);        // Commutation I2C pour le shield clock (2nd slave)

boolean             add = false;
uint8_t             ite = 0;
uint8_t             admin[] = { 1, 114, 25, 170 };
Time                time;
int                 TM = 0;

int                 out1 = 9;
int                 out2 = 6;
float               V1 = 0;
float               V2 = 0;


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
        for (int j = 0; j < 8; j++)
                EEPROM.write(i * 8 + j, 0);      // Remove even if rewritten aftermath, it could be the last in the list
        while (++i < ite)
        {
                for (int j = 0; j < 8; j++)
                {
                        EEPROM.write((i - 1) * 8 + j, EEPROM.read(i * 8 + j));    //replace to not leave a hole in the list
                        EEPROM.write(i * 8 + j, 0);
                }
        }
        --ite;
}

struct ms           getEEPROM(uint8_t i)
{
        struct ms   lol;

        for (int j = 0; j < 7; j++)
                lol.content[j] = EEPROM.read(i * 8 + j);    // get each member to check if user is registered
        return (lol);
}

void                saveEEPROM(uint8_t val[])
{
        int  Fecha = makeTimeStamp();
        for (int i = 0; i < 7; i++)
                EEPROM.write(ite * 8 + i, val[i]);    // save a new user
        EEPROM.write(8 * ite + 7, Fecha);
        ++ite;
}

void                 chargeEEPROM()
{
        uint8_t      list[][7] = {
                { 4, 62, 97, 50, 198, 78, 128 },
                { 4, 95, 168, 50, 198, 78, 128 }
        };
  
        uint8_t    valid = EEPROM.read(0);                 // fill a first time the memory
  
        if (valid == 0)
        {
                time = rtc.getTime();
                for (int i = 0; i < ShipMEM; i++)
                        EEPROM.write(i, 0);
                saveEEPROM(list[0]);
                saveEEPROM(list[1]);
        }
        else
        {
                int i = 0;
                while (EEPROM.read(i++))
                        if (i % 7 == 0 && i != 0)         // count number of registered member at boot time
                                ++ite;
        }
}

void                 setup(void)
{
        Serial.begin(115200);
        delay(50);
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

void    call_one(int pin)
{
        digitalWrite(pin, LOW);
        delay(150);
        digitalWrite(pin, HIGH);       
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
                if (!cc((uint8_t *) existing[i], (uint8_t *) mth, 9))
                        return (i + 1);
        }
    
}

int                  makeTimeStamp()
{
        time = rtc.getTime();
        setTime(time.hour, time.min, time.sec, time.date, retrieveM(rtc.getMonthStr()), time.year);
        Serial.print("Epoch: ");
        Serial.println(now());
        return (now() + 604800);      // get Epoch from current time (unix timestamp) plus one week in seconds
}

void                 remove_expired()
{
        int          Fecha;
        
        time = rtc.getTime();
        setTime(time.hour, time.min, time.sec, time.date, retrieveM(rtc.getMonthStr()), time.year);
        for (int i = 0 ; i < ShipMEM; i += 7)
        {
                Fecha = EEPROM.read(i);
                Serial.print("LE ici ");
                Serial.println(Fecha);
                if (Fecha < now())
                        removeEEPROM(i - 7);
        }
  
}

void                 onInterrupt()
{
        if (digitalRead(12) == LOW)
        {
                delay(1000);
                for (int i = 2 ; i < ShipMEM; i++)
                        EEPROM.write(i, 0);
        }
}

void                 loop(void)
{  
        uint8_t      success;
        uint8_t      uid[] = { 0, 0, 0, 0, 0, 0, 0 };
        uint8_t      uidLength;
        struct ms    lol;
  
        success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
  
        //V1 = analogRead(A5) * (5 / 1023.0);
        V2 = analogRead(A0) * (5 / 1023.0);
          
        if (V1 >= 0.3) 
                call_one(out1);
          
        if (V2 >= 0.3) 
                call_one(out2);
          
        delay(2);
        ++TM;
        if (TM >= REFRESH * 1000)
        {
                TM = 0;
                remove_expired();
        }
        if (success && uidLength == 4 && cc(uid, admin, uidLength) == 0)
        {
                add = !add;
                delay(800);
        }
    
        if (success && uidLength == 7)
        {
                if (add == 0)
                {
                        for (uint8_t i = 0 ; i <= ShipMEM / 8;)
                        {
                                lol = getEEPROM(i);
                                if (cc(lol.content, uid, uidLength) == 0)
                                {
                                        call_one(out2);
                                        delay(500);
                                        break ; 
                                }
                                i += 8;
                        }
                }
                else if (add == 1)
                {
                        for (uint8_t i = 0; i <= ShipMEM / 8;)
                        {
                                lol = getEEPROM(i);
                                if (cc(lol.content, uid, uidLength) == 0)
                                {
                                        add = false;
                                        removeEEPROM(i);
                                        delay(500);
                                        break ;
                                }
                                i += 8;
                        }
                        if (add == 1)
                        {
                                time = rtc.getTime();
                                add = false;
                                delay(500);
                                saveEEPROM(uid);
                        }
                }
        }
}

