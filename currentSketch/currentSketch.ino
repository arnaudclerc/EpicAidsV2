#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h> // OLD   #include <PN532.h>

#define NSS   (10)

int out1 = 9;
int out2 = 6;
float V1 = 0;
float V2 = 0;

Adafruit_PN532 nfc(NSS);

boolean add = false;
uint8_t ite = 0;
uint8_t admin[] = { 1, 114, 25, 170 };
uint8_t list[][7] = {
                { 4, 62, 97, 50, 198, 78, 128 },
                { 4, 95, 168, 50, 198, 78, 128 }
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


void setup(void)
{
        Serial.begin(115200);
        pinMode(out1, OUTPUT);
        pinMode(out2, OUTPUT);
        digitalWrite(out1, HIGH);
        digitalWrite(out2, HIGH);
        delay(50);
        nfc.begin();
        uint32_t versiondata = nfc.getFirmwareVersion();
        if (! versiondata) {
                Serial.println("ERROR");
                while(1);
        }
        nfc.SAMConfig();
}
        
void    call_one(int pin)
{
                digitalWrite(pin, LOW);
                delay(150);
                digitalWrite(pin, HIGH);       
}

void    loop(void)
{    
        uint8_t success;
        uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
        uint8_t uidLength;
  
        //  V1 = analogRead(A5) * (5 / 1023.0);
        V2 = analogRead(A0) * (5 / 1023.0);
          
        if (V1 >= 0.3) 
                call_one(out1);
          
        if (V2 >= 0.3) 
                call_one(out2);
          
        delay(2);
          
        success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
               
        if (success && uidLength == 4 && cc(uid, admin, uidLength) == 0)
        {
                Serial.println("ADM");
                add = !add;
                delay(800);
        }
    
        if (success && uidLength == 7)
        {
                if (add == 0)
                {
                        for (uint8_t i = 0 ; i < 36; i++)
                        {
                                if (cc(list[i], uid, uidLength) == 0)
                                {
                                        call_one(out2);
                                        Serial.println("OKC");
                                        delay(500);
                                        break ; 
                                }
                        }
                }
                else if (add == 1)
                {
                        for (uint8_t i = 0; i < 36; i++)
                        {
                                if (cc(list[i], uid, uidLength) == 0)
                                {
                                        add = false;
                                        Serial.println("OK-");
                                        memcpy(list[i], list[ite-- + 1], uidLength);
                                        delay(500); 
                                        break ;
                                }
                        }
                        if (add == 1)
                        {
                                Serial.println("OK+");
                                add = false;
                                delay(500);
                                memcpy(list[ite++], uid, uidLength);
                        }
                }
        }
}
