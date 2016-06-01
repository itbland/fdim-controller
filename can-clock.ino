#include <Arduino.h>

#include <SPI.h>

#if defined(ESP8266)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif

#include <mcp_can.h>
#include <mcp_can_dfs.h>

#include <Wire.h>
#include <SoftwareSerial.h>

#define DEBUG 0

#define TIMER_STEP 25

#define START_COUNT 8
#define MSG_COUNT 4
#define TEXT_COUNT 12

#define TEXT_MSG_LENGTH 14

#define TZ 3
byte second, minute, hour;

SoftwareSerial mySerial(8, 9); // RX, TX

String fl, fr, rl, rr, carSpeed, rpm, temperature, message;

String dump[8];

int timer;

class CANMessage {

public:
  uint32_t started;
  uint32_t delayed;
  uint32_t repeated;
  uint32_t header;
  
  byte len;
  byte data[8];

  CANMessage()
  {
  }

  CANMessage(uint32_t _started, uint32_t _delayed, uint32_t _repeated, uint32_t _header, byte _len, byte _d0, byte _d1, byte _d2, byte _d3, byte _d4, byte _d5, byte _d6, byte _d7)
  {
    started = _started;
    delayed = _delayed;
    repeated = _repeated;
    header = _header;
    len = _len;
    data[0] = _d0;
    data[1] = _d1;
    data[2] = _d2;
    data[3] = _d3;
    data[4] = _d4;
    data[5] = _d5;
    data[6] = _d6;
    data[7] = _d7;
  }

  void set(uint32_t _started, uint32_t _delayed, uint32_t _repeated, uint32_t _header, byte _len, byte _d0, byte _d1, byte _d2, byte _d3, byte _d4, byte _d5, byte _d6, byte _d7)
  {
    started = _started;
    delayed = _delayed;
    repeated = _repeated;
    header = _header;
    len = _len;
    data[0] = _d0;
    data[1] = _d1;
    data[2] = _d2;
    data[3] = _d3;
    data[4] = _d4;
    data[5] = _d5;
    data[6] = _d6;
    data[7] = _d7;
  }
  void print()
  {
    Serial.println("****");
    Serial.print(started);
    Serial.print(" ");
    Serial.print(delayed);
    Serial.print(" ");
    Serial.print(repeated);
    Serial.print(" ");
    Serial.print(header);
    Serial.print(" ");
    Serial.print(len);
    Serial.print(" ");
    Serial.print(data[0]);
    Serial.print(" ");
    Serial.print(data[1]);
    Serial.print(" ");
    Serial.print(data[2]);
    Serial.print(" ");
    Serial.print(data[3]);
    Serial.print(" ");
    Serial.print(data[4]);
    Serial.print(" ");
    Serial.print(data[5]);
    Serial.print(" ");
    Serial.print(data[6]);
    Serial.print(" ");
    Serial.print(data[7]);
    Serial.println(" ");
  }

};

CANMessage start[START_COUNT];
CANMessage cycle[MSG_COUNT];
CANMessage text[TEXT_COUNT];

void initStartMessages()
{
  start[0].set( 0, 100, 0, 0x50c, 3, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );
  start[1].set( 0, 250, 0, 0x3e8, 8, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00 );
  start[2].set( 0,  50, 0, 0x3ef, 8, 0x32, 0x32, 0x32, 0x32, 0x03, 0x00, 0x00, 0x00 );  
  start[3].set( 0,  50, 0, 0x3f2, 8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x60, 0x00, 0x00 );
  start[4].set( 0, 100, 0, 0x50c, 3, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );
  start[5].set( 0,  50, 0, 0x3f2, 8, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00 );
  start[6].set( 0,  50, 0, 0x3f1, 8, 0xF5, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );
  start[7].set( 0, 100, 0, 0x50c, 3, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );
}

void initCycleMessages()
{
  cycle[0].set( 0,   0, 500, 0x50c, 3, 0x11, 0x02, 0x00, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE );  
  // 0x3e8: 1st byte 01 or 00, 4th byte 04 or 00; 
  //1st byte 01 - AM 02 - FM1 03 - FM2 04 - PHON 05 - SYNC 06 - DVD 07 - AUX 08 - CD 09 - EMPTY 0A - SAT1 0B - SAT2 0C - SAT3 0D - PHON OE - LINE 0F - 2 clocks
  //3th byte volume
  //4th byte - clock length?
  cycle[1].set( 0, 400, 1000, 0x3e8, 8, 0x0F, 0x00, 0x29, 0x04, 0x00, 0x00, 0x00, 0x00 );
  cycle[2].set( 0, 450, 1000, 0x3ef, 8, 0x32, 0x32, 0x32, 0x32, 0x03, 0x00, 0x00, 0x20 );
  cycle[3].set( 0, 500, 1000, 0x3f2, 8, 0x12, 0x01, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00 );
}

void initTextMessages()
{
  text[ 0].set( 1000,   0, 500, 0x336, 8, 0x03, 0x01, 0x0A, 0x01, 0xFE, 0x00, 0x00, 0x00 );
  text[ 1].set( 1000,  50, 500, 0x324, 8, 0x01, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 );
  text[ 2].set( 1000, 100, 500, 0x337, 8, 0x06, 0x20,  ' ',  '@',  ' ',  '@',  ' ', 0x00 );
  text[ 3].set( 1000, 200, 500, 0x336, 8, 0x03, 0x01, 0x05, 0x03, 0x03, 0x00, 0x00, 0x00 );
  text[ 4].set( 1000, 225, 500, 0x324, 8, 0x03, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 );
  text[ 5].set( 1000, 250, 500, 0x337, 8, 0x10, 0x2B,  ' ',  ' ',  ' ',  '@',  ' ',  ' ' );
  text[ 6].set( 1000, 275, 500, 0x337, 8, 0x21,  ' ',  'M',  'e',  'r',  'c',  '@',  'u' );
  text[ 7].set( 1000, 300, 500, 0x337, 8, 0x22,  'r',  'y',  ' ',  ' ',  ' ',  ' ',  ' ' );
  text[ 8].set( 1000, 325, 500, 0x337, 8, 0x23,  ' ',  ' ',  ' ',  ' ',  ' ',  '@',  ' ' );
  text[ 9].set( 1000, 350, 500, 0x337, 8, 0x24,  ' ',  ' ',  'M',  'a',  'r',  'i',  'n' );
  text[10].set( 1000, 375, 500, 0x337, 8, 0x25,  'e',  'r',  ' ',  ' ',  ' ',  ' ',  ' ' );
  text[11].set( 1000, 400, 500, 0x337, 8, 0x26,  ' ',  ' ', 0x00, 0x00, 0x00, 0x00, 0x00 );
}

const int SPI_CS_PIN = 10;

unsigned char rcvFlag = 0;
unsigned char rcvLen = 0;
unsigned long rcvCanId = 0x0;
unsigned char rcvBuf[8];
unsigned char sendingNow = 0;

unsigned int sentOnTick = 0;

bool firstCycle = true;
bool gotClock = false;

int currentText = 0;

MCP_CAN CAN(SPI_CS_PIN); 

void MCP2515_ISR()
{
    rcvFlag = 1;
}

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void printDebug(int timer, CANMessage msg)
{
    Serial.print("Time: ");
    Serial.print(timer);
    Serial.print(" ");
    Serial.print(msg.header, HEX);
    Serial.print(": ");
    for (int j=0;j<msg.len;j++) {
      Serial.print(msg.data[j], HEX);
      Serial.print(" ");
    }
    Serial.println("");
}

void displayText(int strNo, String str)
{
  byte curLine, curChar, numChars;

  if (DEBUG) {
    Serial.println(String(strNo) + ": " + str);
  }
  switch (strNo) {
    case 0:
      curLine = 2;
      curChar = 2;
      numChars = 3;
      break;
    case 1:
      curLine = 5;
      curChar = 2;
      numChars = 20;
      break;
    case 2:
      curLine = 8;
      curChar = 3;
      numChars = 20;
      break;
  }

  for (byte i=0;i<numChars;i++) {
    if (
          ( curLine == 2 && curChar == 3 ) ||
          ( curLine == 2 && curChar == 5 ) ||
          ( curLine == 5 && curChar == 5 ) ||
          ( curLine == 6 && curChar == 6 ) ||
          ( curLine == 8 && curChar == 6 )
       ) {
      text[curLine].data[curChar] = '@';
      curChar++;
    }

    text[curLine].data[curChar] = i<str.length() ? str[i]:' ';

    curChar++;

    if ( curChar == 8 ) {
      curLine++;
      curChar = 1;
    }
  }
}

String padLeft(String inStr, byte length)
{
  String str = inStr;
  str.trim();
  str = str.substring(0,length);
  while ( str.length() < length) {
    str = str + " ";   
  }

  return str;
}

String padRight(String inStr, byte length)
{
  String str = inStr;
  str.trim();
  str = str.substring(0,length);
  while ( str.length() < length) {
    str = " " + str;   
  }
  return str;
}


String padCenter(String inStr, byte length)
{
  String str = inStr;
  str.trim();
  inStr = str.substring(0,length);
  while ( str.length() < length) {
    str = (str.length() % 2) ? str + " " : " " +str;   
  }
  return str;
}

void sendStartSequence()
{
  delay(1000);
  timer = 0; 

  for (int i=0;i<START_COUNT;i++) {
    CAN.sendMsgBuf(start[i].header, 0, start[i].len, start[i].data);  
    printDebug(timer, start[i]);
    delay(start[i].delayed);
    timer = timer + start[i].header;
  }
  Serial.println("****");
  timer = 0;
  firstCycle = true;
  currentText = 0;
  delay(500);
}

void setup() {           
  initStartMessages();
  initCycleMessages();
  initTextMessages();
  Serial.begin(115200);
  mySerial.begin(9600);

#if defined(ESP8266)
  Wire.begin(0, 2);
#endif
  pinMode(A4, INPUT);

START_INIT:
if(CAN_OK == CAN.begin(CAN_125KBPS, MCP_8MHz))
    {
        Serial.println("CAN ok!");
    }
    else
    {
        Serial.println("CAN fail");
        delay(100);
        goto START_INIT;
    }

  attachInterrupt(0, MCP2515_ISR, FALLING); // start interrupt

  CAN.init_Mask(0, 0, 0x7FF << 18);                         // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(1, 0, 0x7FF << 18);
  CAN.init_Filt(0, 0, 0x423 << 18);   // Speed data
  CAN.init_Filt(1, 0, 0x3B5 << 18);   // TPMS data
  CAN.init_Filt(2, 0, 0x466 << 18);   // GPS

//  CAN.init_Filt(3, 0, 0x43a << 18);   // ???
//  CAN.init_Filt(3, 0, 0x3a1 << 18);   // HVAC?
//  CAN.init_Filt(4, 0, 0x3a4 << 18);   // HVAC?
  
//  CAN.init_Filt(0, 0, 0x2db << 18);   // SYNC buttons
//  CAN.init_Filt(2, 0, 0x398 << 18);   // HVAC

  timer = 0;
  delay(500);

}

void loop() {
  String inSerialData;

  if (rcvFlag == 1) {
    rcvFlag = 0;
    while (CAN_MSGAVAIL == CAN.checkReceive()) {

      CAN.readMsgBuf(&rcvLen, rcvBuf);
      rcvCanId = CAN.getCanId();
      switch (rcvCanId) {
        case 0x3b5: { // TPMS
            fl = String(rcvBuf[0]);
            fr = String(rcvBuf[1]);
            rr = String(rcvBuf[2]);
            rl = String(rcvBuf[3]);
        }
          break;
        case 0x423: { // Speed, RPM
          byte rpm1 = rcvBuf[2];
          byte rpm2 = rcvBuf[3];
          rpm = String(( ( rpm1 << 8 ) + rpm2 ) / 4);
                  
          byte speed1 = rcvBuf[0];
          byte speed2 = rcvBuf[1];
          carSpeed = String(round((( speed1 << 8) + speed2)/100) - 100, 0);       

          byte t = rcvBuf[4];
          temperature = String(t-40);
          
          if ( ((rpm == "0") || (rpm == "")) && (sendingNow == 1)) {
            carSpeed = "";
            rpm = "";
            temperature = "";
            message = "";
            sendingNow = 0;
            gotClock = false;
          }

          if ((rpm != "0") && (rpm != "") && (sendingNow == 0)) {
            sendingNow = 1;
            sendStartSequence();
          }
        }
          break;
        case 0x466: {  // GPS clock
            hour = (((rcvBuf[0] & 0xF8) >> 3) + TZ ) % 24;
            minute = (rcvBuf[1] & 0xFC) >> 2;
            second = (rcvBuf[2] & 0xFC) >> 2;
            gotClock = true;
            //message=String(hour, DEC) + " " + String(minute, DEC) + " " + String(second, DEC);
        }
          break;
      }
    }
  }

  if ((sendingNow == 1) || (DEBUG)) {
    for (int currentCycle = 0; currentCycle < MSG_COUNT; currentCycle ++ ) {
      if ( ( (timer >= cycle[currentCycle].started ) || (!firstCycle) ) && ((timer % cycle[currentCycle].repeated) - cycle[currentCycle].delayed) == 0) {
        if (cycle[currentCycle].header == 0x3f2) {
          cycle[currentCycle].data[0] = decToBcd(hour);
          cycle[currentCycle].data[1] = decToBcd(minute);
          if (gotClock) {
            CAN.sendMsgBuf(cycle[currentCycle].header, 0, cycle[currentCycle].len, cycle[currentCycle].data);
            sentOnTick = timer;  
            printDebug(timer, cycle[currentCycle]);
          }
        } else {
          CAN.sendMsgBuf(cycle[currentCycle].header, 0, cycle[currentCycle].len, cycle[currentCycle].data);
          sentOnTick = timer;  
          printDebug(timer, cycle[currentCycle]);
        }
      }
    }    
    inSerialData = "";

    while (mySerial.available() > 0) {
        char recieved = mySerial.read();
        inSerialData += recieved; 
  
        if (recieved == '\n')
        {
          message = inSerialData;
          inSerialData = "";
        }
    }
    
    // For MQ135
    /*int sensorValue = analogRead(4);
    Serial.println(sensorValue);
    message = String(sensorValue, DEC);*/
    if (message == "%MTRACK") message = "";
    
    //Serial.println(">>>>>> " + String(timer) + " " + String(i) + " " + String(timer % text[i].repeated) + " " + String(firstCycle ? text[i].started : 0) + " " + String(text[i].delayed)) + " " + String(((timer % text[i].repeated) - (firstCycle ? text[i].started : 0 ) - text[i].delayed));
    if ( ( (timer >= text[currentText].started ) || (!firstCycle) ) && ((timer % text[currentText].repeated) - text[currentText].delayed) == 0) {
      if (currentText == 0) {
        displayText(0, padRight(carSpeed,3));
        displayText(1, padRight(fl, 2) + " RPM:" + padRight(rpm, 4) + " T:" + padRight(temperature, 3) + " " + padRight(fr, 2));
        displayText(2, padRight(rl, 2) + " " + padCenter(message, TEXT_MSG_LENGTH) + " " + padRight(rr, 2));
      }
      if (sentOnTick == timer) delay(TIMER_STEP/2);
      CAN.sendMsgBuf(text[currentText].header, 0, text[currentText].len, text[currentText].data);
      printDebug(timer, text[currentText]);
      //delay(text[currentText].delayed);
      currentText = (currentText + 1) % TEXT_COUNT;
    }

    delay(TIMER_STEP);
    timer += TIMER_STEP;
    if (timer >= 32000) {
      timer = 0;
      firstCycle = false;
    }
  }
}




