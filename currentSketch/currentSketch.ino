#include <PN532.h>
#include <SPI.h>

int out1 = 6;
int out2 = 3;
float V1 = 0;
float V2 = 0;

#define PN532_CS 10
PN532 nfc(PN532_CS);

void setup()
{
  Serial.begin(115200);
  pinMode(out1, OUTPUT);
  pinMode(out2, OUTPUT);
  digitalWrite(out1, HIGH);
  digitalWrite(out2, HIGH);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
      Serial.println("ERROR");
      while(1);
  }
  nfc.SAMConfig();
}

void loop()
{
//  V1 = analogRead(A5) * (5 / 1023.0);
  V2 = analogRead(A0) * (5 / 1023.0);
  
  if (V1 >= 0.3) 
  {
    digitalWrite(out1, LOW);
    delay(150);
    digitalWrite(out1, HIGH);
  }
  
  if (V2 >= 0.3) 
  {
    digitalWrite(out2, LOW);
    delay(150);
    digitalWrite(out2, HIGH);
  }
  
  delay(2);
}
