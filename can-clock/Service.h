#ifndef __SERVICE_H_
#define __SERVICE_H_

#include <Arduino.h>
#include <stdarg.h>
#include <EEPROM.h>

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

void dstCk(int8_t h, int8_t d, uint8_t m, uint16_t y)
{
  uint8_t dstDate[] = {10,8,14,13,12,10,9,8,14,12,11,10,9,14} // DST Mar change date 2019 - 2032, Nov date is -7 of Mar date!
  if ((m > 3 && m < 11) || (m == 3 && d > dstDate[(y - 2019)]) || (m == 3 && d == dstDate[(y - 2019)] && h >= 2) || (m == 11 && d < (dstDate[(y - 2019)] - 7)) || (m == 11 && d == (dstDate[(y - 2019)] - 7) && h < 2 && currentSettings.DST))
    EEPROM.update((CONFIG_START + 9), true);
  else
    EEPROM.update((CONFIG_START + 9), false);
  
  return;
}

void timeShift(int8_t &h, uint8_t &d, uint8_t &m, uint16_t &y)
{
  uint8_t mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31}; //days in month, padded with a zero
  if (y % 4 == 0)       // need to account for leapyear, good until the year 2100
    mdays[2] = 29;
  else
    mdays[2] = 28;
  
  if (h < 0) {  //tz or DST put us before midnight
    h += 24;
    d--;
    if (d == 0) {
      m--;
      if (m == 0) {
        m = 12;
        y--;
      }
      d = mdays[m];
    }
  }
  else if (h > 24) {  //tz put us after midnight
    h %= 24;
    d++;
    if (d > mdays[m]) {
      d = 1;
      m++;
      if (m == 13) {
        y++;
        m = 1;
      }
    }
  }
  return;
}

#endif // __SERVICE_H_
