#ifndef __SERVICE_H_
#define __SERVICE_H_

#include <Arduino.h>
#include <stdarg.h>

uint8_t decToBcd(const uint8_t val)
{
  return( (val/10*16) + (val%10) );
}

uint8_t bcdToDec(const uint8_t val)
{
  return( (val/16*10) + (val%16) );
}

String readSerialString()
{
  String inSerialData;
  char received = ' ';
  Serial.print(F("\n> "));
  while (received != '\r') {
      if (Serial.available() > 0) {
        received = Serial.read();
        inSerialData += received;
        Serial.print(received);
      }
  }
  inSerialData.trim();
  return(inSerialData);
}

uint8_t Day_of_Week(const uint16_t yr, const uint8_t m, const uint8_t d)
{
  uint8_t weekday;
  uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  uint16_t y = yr - m < 3;
  weekday = (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
  return(weekday);
}

void dstCk(int8_t &hour, int8_t &date, uint8_t &month, uint16_t &year)
{
  uint8_t dstDate[] = {10,8,14,13,12,10,9,8,14,12,11,10,9,14} // DST Mar change date 2019 - 2032, Nov date is -7 of Mar date!
  if ((month > 3 && month < 11) || (month == 3 && date > dstDate[(year - 2019)]) || (month == 3 && date == dstDate[(year - 2019)] && hour >= 2) || (month == 11 && date < (dstDate[(year - 2019)] - 7)) || (month == 11 && date == (dstDate[(year - 2019)] - 7) && hour < 2 && currentSettings.DST))
    EEPROM.update((CONFIG_START + 9), true);
  else
    EEPROM.update((CONFIG_START + 9), false);
  
  return;
}

#endif // __SERVICE_H_
