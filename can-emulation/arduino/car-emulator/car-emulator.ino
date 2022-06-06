// Developer: 
//        Adam Varga, 2020, All rights reserved.
// Licence: 
//        Licenced under the MIT licence. See LICENCE file in the project root.
// Usage of this code: 
//        This code is roughly based on the can sniffer from Adam Varga
//        https://github.com/adamtheone/canDrive
//        Project job is to emulate how CAN bus in car works, for the 
//        fiat Continental radios to turn on and work.
// Required arduino packages (copy from libraries folder to your local arduino lib dir): 
//        - CAN by Sandeep Mistry (https://github.com/sandeepmistry/arduino-CAN)
//        - DateTime library
// Required modifications: 
//        - MCP2515.h: 16e6 clock migth need frequency reduced to 8e6 (depending on MCP2515 clock)
//        - MCP2515.cpp: extend CNF_MAPPER with your desired CAN speeds
//------------------------------------------------------------------------------
#include <CAN.h>
#include <EEPROM.h>
#include <DateTime.h>
//------------------------------------------------------------------------------
// Settings
#define CAN_SPEED (50E3) //LOW=50E3, HIGH=125E3 (there are two speeds, for my VP2 model 50kbps works)
#define AUTH_CONFIG_BYTE_0 0x00
#define AUTH_CONFIG_BYTE_1 0x01
#define ACC_PIN A0
//#define SERIAL_COMM
#define GIULIETTA //CAN simulation for my own car model
//------------------------------------------------------------------------------
// Inits, globals
typedef struct {
  long id;
  byte rtr;
  byte ide;
  byte dlc;
  byte dataArray[20];
} packet_t;

#ifdef SERIAL_COMM
const char SEPARATOR = ',';
const char TERMINATOR = '\n';
const char RXBUF_LEN = 100;
#endif

byte auth_bytes[2];
bool acc_on = false;
bool acc_last = false;
bool radio_on = false;
byte radio_status_prev = 0x00;
bool once = false;
byte tickler_count = 0x00;
int button_state_prev;
time_t time;
unsigned long every50;
unsigned long every100;
unsigned long every200;
unsigned long every300;
unsigned long every500;
unsigned long every1000;
//------------------------------------------------------------------------------

byte* getCode(bool conf, byte* input);
void advanceClock();
void onCANReceive(int packetSize);
void sendPacketToCan(packet_t * packet);
void hexCharacterStringToBytes(byte* byteArray, char* hexString);

#ifdef SERIAL_COMM
void printHex(long num);
void printPacket(packet_t * packet);
char getNum(char c);
char* strToHex(char * str, byte * hexArray, byte * len);
void rxParse(char * buf, int len);
void RXcallback(void);
#endif

//------------------------------------------------------------------------------
// Setup
void setup() 
{
    auth_bytes[0] = EEPROM.read(AUTH_CONFIG_BYTE_0);
    auth_bytes[1] = EEPROM.read(AUTH_CONFIG_BYTE_1);

    time = DateTime.makeTime(0x00, 0x00, 0x00, 0x01, 0x01, 2015);
    DateTime.sync(time);
    DateTime.available();

    pinMode(ACC_PIN, INPUT);

#ifdef SERIAL_COMM
    Serial.begin(250000);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
#endif

    if (!CAN.begin(CAN_SPEED)) {
#ifdef SERIAL_COMM
        Serial.println("Starting CAN failed!");
#endif
        while (1);
    }
    // register the receive callback
    CAN.onReceive(onCANReceive);
#ifdef SERIAL_COMM
    Serial.println("CAN RX TX Started");
#endif
}

//------------------------------------------------------------------------------
// CAN RX, TX
void onCANReceive(int packetSize) {
    // received a CAN packet
    packet_t rxPacket;
    rxPacket.id = CAN.packetId();
    rxPacket.rtr = CAN.packetRtr() ? 1 : 0;
    rxPacket.ide = CAN.packetExtended() ? 1 : 0;
    rxPacket.dlc = CAN.packetDlc();
    byte i = 0;
    while (CAN.available()) {
        rxPacket.dataArray[i++] = CAN.read();
        if (i >= (sizeof(rxPacket.dataArray) / (sizeof(rxPacket.dataArray[0])))) {
            break;
        }
    }
#ifdef SERIAL_COMM
    printPacket(&rxPacket);
#endif

    //immo logic
    if(rxPacket.id == 0xA094005)
    {
        packet_t auth_res = {0xA114000, 0x00, 0x01, 0x03, {0x00, 0x00, 0x00}};

        if(rxPacket.dataArray[0] == 0x00)
        {
            //calc code
            byte buff[2] = { rxPacket.dataArray[1], rxPacket.dataArray[2] };
            byte* code = getCode(false, buff);

            //return response
            auth_res.dataArray[0] = rxPacket.dataArray[0];
            auth_res.dataArray[1] = code[0];
            auth_res.dataArray[2] = code[1];
            delete[] code;

            sendPacketToCan(&auth_res);
#ifdef SERIAL_COMM
            printPacket(&auth_res);
#endif
        }
        else if(rxPacket.dataArray[0] == 0x01)
        {
            //calculate init
            byte buff[2] = { rxPacket.dataArray[1], rxPacket.dataArray[2] };
            byte* result = getCode(true, buff);

            //set and save init
            auth_bytes[0] = result[0];
            auth_bytes[1] = result[1];
            EEPROM.write(AUTH_CONFIG_BYTE_0, auth_bytes[0]);
            EEPROM.write(AUTH_CONFIG_BYTE_1, auth_bytes[1]);
            delete[] result;

            //return ack
            auth_res.dataArray[0] = rxPacket.dataArray[0];
            auth_res.dataArray[1] = rxPacket.dataArray[1];
            auth_res.dataArray[2] = rxPacket.dataArray[2];
            sendPacketToCan(&auth_res);
#ifdef SERIAL_COMM
            printPacket(&auth_res);
#endif
        }
    }
#ifdef GIULIETTA
    else if(rxPacket.id == 0xE094024)
        radio_on = true;
    else if(rxPacket.id == 0x6314024)
    {
        if(radio_status_prev > 0x10 && rxPacket.dataArray[2] == 0x10)
            radio_on = false;
        else if(radio_status_prev < 0x20 && rxPacket.dataArray[2] >= 0x20)
            radio_on = true;
        radio_status_prev = rxPacket.dataArray[2];
    }
    //C214024,00,01,000001012015
    else if(rxPacket.id == 0xC214024)
        setTime(rxPacket.dataArray);
#endif
}


//------------------------------------------------------------------------------
// Main
void loop() 
{
#ifdef SERIAL_COMM
    RXcallback();
#endif
    DateTime.now();
    int button_state = digitalRead(ACC_PIN);
    if(button_state != button_state_prev && button_state)
    {
        once = true;
        acc_on = !acc_on;
    }
        
    button_state_prev = button_state;

    packet_t packet;
    
    //if(!acc_on && !radio_on) { return; }
#ifdef GIULIETTA    
    if(once && !radio_on && acc_on) { //starting sequence
        // E094018,00,01,02,00,04
        packet = {0xE094018, 0x00, 0x01, 0x02, {0x00,0x04}};
        sendPacketToCan(&packet);
        // 6314018,00,01,08,00,0F,0F,0F,0F,00,00,00
        packet = {0x6314018, 0x00, 0x01, 0x08, {0x00,0x0F,0x0F,0x0F,0x0F,0x00,0x00,0x00}};
        sendPacketToCan(&packet);
        delay(10);
        // E094000,00,01,06,00,18,00,00,00,01
        packet = {0xE094000, 0x00, 0x01, 0x06, {0x00,0x18,0x00,0x00,0x00,0x01}};
        sendPacketToCan(&packet);
        delay(50);
        // E094000,00,01,06,00,1E,01,00,00,01
        packet = {0xE094000, 0x00, 0x01, 0x06, {0x00,0x1E,0x01,0x00,0x00,0x01}};
        sendPacketToCan(&packet);
        // E094018,00,01,02,00,06
        packet = {0xE094018, 0x00, 0x01, 0x02, {0x00,0x06}};
        sendPacketToCan(&packet);
        delay(10);
        // 6214000,00,01,08,00,08,48,00,00,00,0F,00
        packet = {0x6214000, 0x00, 0x01, 0x08, {0x00,0x08,0x48,0x00,0x00,0x00,0x0F,0x00}};
        sendPacketToCan(&packet);
        // E094003,00,01,02,00,0A
        packet = {0xE094003, 0x00, 0x01, 0x02, {0x00,0x0A}};
        sendPacketToCan(&packet);
        // C394003,00,01,08,00,10,00,10,80,00,40,00
        packet = {0xC394003, 0x00, 0x01, 0x08, {0x00,0x10,0x00,0x10,0x80,0x00,0x40,0x00}};
        sendPacketToCan(&packet);
        delay(20);
        // 2214000,00,01,06,28,00,00,00,00,00
        packet = {0x2214000, 0x00, 0x01, 0x06, {0x00,0x28,0x00,0x00,0x00,0x00}};
        sendPacketToCan(&packet);
        // E09401A,00,01,02,00,0C
        packet = {0xE09401A, 0x00, 0x01, 0x02, {0x00,0x0C}};
        sendPacketToCan(&packet);
        // 621401A,00,01,08,A0,00,00,00,80,00,00,00
        packet = {0x621401A, 0x00, 0x01, 0x08, {0xA0,0x00,0x00,0x00,0x80,0x00,0x00,0x00}};
        sendPacketToCan(&packet);
        // 6314018,00,01,08,C0,0F,0F,0F,0F,00,00,00
        packet = {0x6314018, 0x00, 0x01, 0x08, {0xC0,0x0F,0x0F,0x0F,0x0F,0x00,0x00,0x00}};
        sendPacketToCan(&packet);
        // 6314003,00,01,08,A2,00,70,00,10,08,00,00
        packet = {0x6314003, 0x00, 0x01, 0x08, {0xA2,0x00,0x70,0x00,0x10,0x08,0x00,0x00}};
        sendPacketToCan(&packet);
        //4214001,00,01,08,00,00,90,28,00,00,00,00
        packet = {0x4214001, 0x00, 0x01, 0x08, {0x00,0x00,0x90,0x28,0x00,0x00,0x00,0x00}};
        sendPacketToCan(&packet);
        //4294001,00,01,08,00,07,00,48,7F,10,78,00
        packet = {0x4294001, 0x00, 0x01, 0x08, {0x00,0x07,0x00,0x48,0x7F,0x10,0x78,0x00}};
        sendPacketToCan(&packet);

        every50 = every100 = every200 = every300 = every500 = every1000 = millis();
        once = false;
    }

    if(once && radio_on && !acc_on) {
        // E094000,00,01,06,00,18,00,00,00,01
        packet = {0xE094000, 0x00, 0x01, 0x06, {0x00,0x18,0x00,0x00,0x00,0x01}};
        sendPacketToCan(&packet);
        // E094000,00,01,06,00,1E,01,00,00,01
        packet = {0xE094000, 0x00, 0x01, 0x06, {0x00,0x1E,0x01,0x00,0x00,0x01}};
        sendPacketToCan(&packet);
        // E094003,00,01,02,00,0A
        packet = {0xE094003, 0x00, 0x01, 0x02, {0x00,0x0A}};
        sendPacketToCan(&packet);
        // C394003,00,01,08,00,10,00,10,80,00,40,00
        packet = {0xC394003, 0x00, 0x01, 0x08, {0x00,0x10,0x00,0x10,0x80,0x00,0x40,0x00}};
        sendPacketToCan(&packet);
        // 2214000,00,01,06,28,00,00,00,00,00
        packet = {0x2214000, 0x00, 0x01, 0x06, {0x00,0x00,0x00,0x00,0x00,0x00}};
        sendPacketToCan(&packet);
        // 6214000,00,01,08,00,08,48,00,00,00,0F,00
        packet = {0x6214000, 0x00, 0x01, 0x08, {0x00,0x08,0x48,0x00,0x00,0x00,0x0F,0x00}};
        sendPacketToCan(&packet);
        every50 = every100 = every200 = every300 = every500 = every1000 = millis();
        once = false;
    }

    unsigned long current = millis();

    if(current - every50 > 50)
    {
        // 4214001,00,01,08,01,83,91,38,00,00,00,00        < only when on
        packet = {0x4214001, 0x00, 0x01, 0x08, {0x01,0x83,0x91,0x38,0x00,0x00,0x00,0x00}};
        if(acc_on) sendPacketToCan(&packet);
        // 4294001,00,01,08,00,07,00,48,7F,10,00,00        < only when on
        packet = {0x4294001, 0x00, 0x01, 0x08, {0x00,0x07,0x00,0x48,0x7F,0x10,0x00,0x00}};
        if(acc_on) sendPacketToCan(&packet);

        every50 = current;
    }

    if(current - every100 > 100)
    {
        // 4214002,00,01,02,00,00                          < only when on
        packet = {0x4214002, 0x00, 0x01, 0x02, {0x00,0x00}};
        if(acc_on) sendPacketToCan(&packet);

        every100 = current;
    }

    if(current - every200 > 200 && radio_on)
    {
        // 6214000,00,01,08,08,08,48,00,02,5C,0B,00        < when on
        // 6214000,00,01,08,00,08,18,00,00,00,0F,00        < when off
        if(acc_on) packet = {0x6214000, 0x00, 0x01, 0x08, {0x08,0x08,0x48,0x00,0x02,0x5C,0x0B,0x00}};
        else packet = {0x6214000, 0x00, 0x01, 0x08, {0x00,0x08,0x18,0x00,0x00,0x00,0x0F,0x00}};
        sendPacketToCan(&packet);
        // 2214000,00,01,06,00,28,00,00,00,00              < 00 when off, 28 when on
        packet = {0x2214000, 0x00, 0x01, 0x06, {0x00,(byte)(acc_on?0x28:0x00),0x00,0x00,0x00,0x00}};
        sendPacketToCan(&packet);
        // 621401A,00,01,08,A0,00,00,00,80,00,00,00
        packet = {0x621401A, 0x00, 0x01, 0x08, {0xA0,0x00,0x00,0x00,0x80,0x00,0x00,0x00}};
        if(acc_on) sendPacketToCan(&packet);
        // 6314000,00,01,08,00,00,00,00,00,00,00,00        < 40 when on, 00 when off
        packet = {0x6314000, 0x00, 0x01, 0x08, {(byte)(acc_on?0x40:0x00),0x00,0x00,0x00,0x00,0x00,0x00,0x00}};
        sendPacketToCan(&packet);

        every200 = current;
    }

    if(current - every300 > 300 && radio_on)
    {
        // 6254006,00,01,08,10,00,00,00,00,00,00,00        < tickler
        packet = {0x6254006, 0x00, 0x01, 0x08, {tickler_count,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};
        if(acc_on) 
        {
            sendPacketToCan(&packet);
            tickler_count += 0x10;
            if(tickler_count > 0x30)
                tickler_count = 0x00;
        }
        else
            if(tickler_count > 0) tickler_count = 0;

        every300 = current;
    }

    if(current - every500 > 500 && radio_on)
    {
        // 621400A,00,01,02,00,00                          < only when on
        packet = {0x621400A, 0x00, 0x01, 0x02, {0x00,0x00}};
        if(acc_on) sendPacketToCan(&packet);
        // 6314003,00,01,08,A2,00,70,00,A0,08,00,00        < when on
        if(acc_on) packet = {0x6314003, 0x00, 0x01, 0x08, {0xA2,0x00,0x70,0x00,0xA0,0x08,0x00,0x00}};
        else packet = {0x6314003, 0x00, 0x01, 0x08, {0x00,0x00,0x70,0x00,0x20,0x08,0x00,0x00}};
        sendPacketToCan(&packet);
        // C394003,00,01,08,00,10,00,10,B0,00,40,00        < only when on
        packet = {0xC394003, 0x00, 0x01, 0x08, {0x00,0x10,0x00,0x10,0xB0,0x00,0x40,0x00}};
        if(acc_on) sendPacketToCan(&packet);
        // C394003,00,01,08,00,10,00,10,80,00,40,00
        packet = {0xC394003, 0x00, 0x01, 0x08, {0x00,0x10,0x00,0x10,0x80,0x00,0x40,0x00}};
        sendPacketToCan(&packet);
        
        every500 = current;
    }

    if(current - every1000 > 1000 && radio_on)
    {
        // E094018,00,01,02,00,06                          < only when on
        packet = {0xE094018, 0x00, 0x01, 0x02, {0x00,0x06}};
        if(acc_on) sendPacketToCan(&packet);
        // E09400A,00,01,02,00,0E                          < only when on
        packet = {0xE09400A, 0x00, 0x01, 0x02, {0x00,0x0E}};
        if(acc_on) sendPacketToCan(&packet);
        // 6314018,00,01,08,C0,0F,0F,0F,0F,00,00,00        < only when on
        packet = {0x6314018, 0x00, 0x01, 0x08, {0xC0,0x0F,0x0F,0x0F,0x0F,0x00,0x00,0x00}};
        if(acc_on) sendPacketToCan(&packet);
        // E094000,00,01,06,00,1E,25,40,0C,4F
        packet = {0xE094000, 0x00, 0x01, 0x06, {0x00,0x1E,0x25,0x40,0x0C,0x4F}};
        sendPacketToCan(&packet);
        // E094003,00,01,02,00,0E                          < 0A when off, 0E when on
        packet = {0xE094003, 0x00, 0x01, 0x02, {0x00,(byte)(acc_on?0x0E:0x0A)}};
        sendPacketToCan(&packet);
        // E09401A,00,01,02,00,0E                          < only when on
        packet = {0xE09401A, 0x00, 0x01, 0x02, {0x00,0x0E}};
        if(acc_on) sendPacketToCan(&packet);
        
        sendTimePakcet();

        every1000 = current;
    }
#endif
    acc_last = acc_on;
}

void sendTimePakcet()
{
    char buffer[5];
    byte bbuffer[2];
    DateTime.available();
#ifdef GIULIETTA
    sprintf(buffer, "%02i", DateTime.Hour);
    hexCharacterStringToBytes(bbuffer, buffer);
    byte hour = bbuffer[0];

    sprintf(buffer, "%02i", DateTime.Minute);
    hexCharacterStringToBytes(bbuffer, buffer);
    byte minute = bbuffer[0];

    sprintf(buffer, "%02i", DateTime.Day);
    hexCharacterStringToBytes(bbuffer, buffer);
    byte day = bbuffer[0];

    sprintf(buffer, "%02i", DateTime.Month);
    hexCharacterStringToBytes(bbuffer, buffer);
    byte month = bbuffer[0];
    
    unsigned short _year = (unsigned short)(DateTime.Year) + 1900;
    sprintf(buffer, "%04i", _year);
    hexCharacterStringToBytes(bbuffer, buffer);
    byte year[2] = {bbuffer[0], bbuffer[1]};
  
    // C214003,00,01,06,14,57,02,06,20,22              < clock
    packet_t packet = {0xC214003, 0x00, 0x01, 0x06, {hour, minute, day, month, year[0], year[1]}};
    sendPacketToCan(&packet);
#endif
}

void setTime(byte* buffer)
{
    char hour[3];
    char minute[3];
    char day[3];
    char month[3];
    char year[5];
#ifdef GIULIETTA
    sprintf(hour, "%02X", buffer[0]);
    sprintf(minute, "%02X", buffer[1]);
    sprintf(day, "%02X", buffer[2]);
    sprintf(month, "%02X", buffer[3]);
    sprintf(year, "%04X", (((unsigned short)buffer[4]) << 8) | buffer[5]);

    byte _hour = (byte)(String(hour).toInt());
    byte _minute = (byte)(String(minute).toInt());
    byte _day = (byte)(String(day).toInt());
    byte _month = (byte)(String(month).toInt());
    int _year = String(year).toInt();

    time = DateTime.makeTime(DateTime.Second, _minute, _hour, _day, _month, _year);
    DateTime.sync(time);
    DateTime.available();
#endif
}

//------------------------------------------------------------------------------
// Functions

byte nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;  // Not a valid hexadecimal character
}

void hexCharacterStringToBytes(byte* byteArray, char* hexString)
{
  bool oddLength = strlen(hexString) & 1;

  byte currentByte = 0;
  byte byteIndex = 0;

  for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++)
  {
    bool oddCharIndex = charIndex & 1;

    if (oddLength)
    {
      // If the length is odd
      if (oddCharIndex)
      {
        // odd characters go in high nibble
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        // Even characters go into low nibble
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
    else
    {
      // If the length is even
      if (!oddCharIndex)
      {
        // Odd characters go into the high nibble
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        // Odd characters go into low nibble
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
  }
}

void sendPacketToCan(packet_t * packet) {
  for (int retries = 10; retries > 0; retries--) {
    bool rtr = packet->rtr ? true : false;
    if (packet->ide){
      CAN.beginExtendedPacket(packet->id, packet->dlc, rtr);
    } else {
      CAN.beginPacket(packet->id, packet->dlc, rtr);
    }
    CAN.write(packet->dataArray, packet->dlc);
    if (CAN.endPacket()) {
      // success
      break;
    } else if (retries <= 1) {
      return;
    }
  }
}

//------------------------------------------------------------------------------
// Serial parser
#ifdef SERIAL_COMM
// Printing a packet to serial
void printHex(long num) {
    if ( num < 0x10 ){ Serial.print("0"); }
    Serial.print(num, HEX);
}

void printPacket(packet_t * packet) {
    // packet format (hex string): [ID],[RTR],[IDE],[DATABYTES 0..8B]\n
    // example: 014A,00,00,1A002B003C004D\n
    printHex(packet->id);
    Serial.print(SEPARATOR);
    printHex(packet->rtr);
    Serial.print(SEPARATOR);
    printHex(packet->ide);
    Serial.print(SEPARATOR);
    // DLC is determinded by number of data bytes, format: [00]
    for (int i = 0; i < packet->dlc; i++) {
    printHex(packet->dataArray[i]);
    }
    Serial.print(TERMINATOR);
}

char getNum(char c) {
    if (c >= '0' && c <= '9') { return c - '0'; }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
    return 0;
}

char * strToHex(char * str, byte * hexArray, byte * len) {
    byte *ptr = hexArray;
    char * idx;
    for (idx = str ; *idx != SEPARATOR && *idx != TERMINATOR; ++idx, ++ptr ) {
        *ptr = (getNum( *idx++ ) << 4) + getNum( *idx );
    }
    *len = ptr - hexArray;
    return idx;
}

void rxParse(char * buf, int len) {
    packet_t rxPacket;
    char * ptr = buf;
    // All elements have to have leading zero!

    // ID
    byte idTempArray[8], tempLen;
    ptr = strToHex(ptr, idTempArray, &tempLen);
    rxPacket.id = 0;
    for (int i = 0; i < tempLen; i++) 
        rxPacket.id |= (long)idTempArray[i] << ((tempLen - i - 1) * 8);

    // RTR
    ptr = strToHex(ptr + 1, &rxPacket.rtr, &tempLen);

    // IDE
    ptr = strToHex(ptr + 1, &rxPacket.ide, &tempLen);

    // DATA
    ptr = strToHex(ptr + 1, rxPacket.dataArray, &rxPacket.dlc);

    if(rxPacket.id == 0x00) // control packet for demo purposes
    {
        once = true;

        if(rxPacket.dataArray[0] == 0x00)
            acc_on = !acc_on;
        else if(rxPacket.dataArray[0] == 0x01)
            acc_on = true;
        else if(rxPacket.dataArray[0] == 0x02)
            acc_on = false;
    }
    else
        sendPacketToCan(&rxPacket);
}

void RXcallback(void) {
    static int rxPtr = 0;
    static char rxBuf[RXBUF_LEN];

    while (Serial.available() > 0) {
        if (rxPtr >= RXBUF_LEN) {
            rxPtr = 0;
        }
        char c = Serial.read();
        rxBuf[rxPtr++] = c;

        // if  (c == '.')
        //     acc_on = !acc_on;

        if (c == TERMINATOR) {
            rxParse(rxBuf, rxPtr);
            rxPtr = 0;
        }
    }
}
#endif

unsigned long powLong(unsigned int x, unsigned int n)
{
  unsigned int number = 1;
    number = 1;
    for (unsigned int i=0; i<n; ++i)
        number *= x;
    return number;
}

bool* shortToBits(unsigned short input) {
    bool* result = new bool[16];
    for(int i=15; i>=0; i--) {
        result[i] = (input & 1);
        input >>= 1;
    }
    return result;
}

unsigned short bitsToShort(bool* input) {
    unsigned short result = 0;
    int j = 15;
    for (int i=0; i<=15; ++i) {
        if(input[i])
          result += powLong(2, j);       
        j--;
    }
    return result;
}

byte* getCode(bool conf, byte* input)
{     
    unsigned short value_short = (((unsigned short)input[0]) << 8) | input[1];
    bool* value = shortToBits(value_short);
    bool* init_bits; 
    bool res[16];

    if(not conf)
        init_bits = shortToBits((((unsigned short)auth_bytes[0]) << 8) | auth_bytes[1]);
    else
        init_bits = new bool[16];
//
//# BIT 0
    res[0] = value[0xb] != value[0xc];

    if(value[8])
        res[0] = not res[0];

    if(value[4])
        res[0] = not res[0];

    if(value[0])
        res[0] = not res[0];

    if(not conf and init_bits[0])
        res[0] = not res[0];
//# BIT 0 END
//# BIT 1
    res[1] = value[0xc] != value[0xd];

    if(value[9])
        res[1] = not res[1];

    if(value[5])
        res[1] = not res[1];

    if(value[1])
        res[1] = not res[1];

    if(not conf and init_bits[1])
        res[1] = not res[1];
//# BIT 1 END
//# BIT 2
    res[2] = value[0xd] != value[0xe];

    if(value[0xa])
        res[2] = not res[2];

    if(value[6])
        res[2] = not res[2];

    if(value[2])
        res[2] = not res[2];

    if(not conf and init_bits[2])
        res[2] = not res[2];
//# BIT 2 END
//# BIT 3
    res[3] = value[0xe] != value[0xf];

    if(value[0xb] or value_short == 0x60f)
        res[3] = not res[3];

    if(value[7])
        res[3] = not res[3];

    if(value[3])
        res[3] = not res[3];

    if(not conf and init_bits[3])
        res[3] = not res[3];
//# BIT 3 END
//# BIT 4
    res[4] = value[0xF];

    if(value[0xc] or value_short == 0x60f)
        res[4] = not res[4];

    if(value[8])
        res[4] = not res[4];

    if(value[4])
        res[4] = not res[4];

    if(not conf and init_bits[4])
        res[4] = not res[4];
//# BIT 4 END
//# BIT 5
    res[5] = value[0xc] != value[0xd];

    if(value[0xb] or value_short == 0x60f)
        res[5] = not res[5];

    if(value[9])
        res[5] = not res[5];

    if(value[8])
        res[5] = not res[5];

    if(value[5])
        res[5] = not res[5];

    if(value[4])
        res[5] = not res[5];

    if(value[0])
        res[5] = not res[5];

    if(not conf and init_bits[5])
        res[5] = not res[5];
//# BIT 5 END
//# BIT 6
    res[6] = value[0xd] != value[0xe];

    if(value_short == 0x60f)
        res[6] = not res[6];

    if(value[0xc])
        res[6] = not res[6];
    
    if(value[0xa])
        res[6] = not res[6];

    if(value[9])
        res[6] = not res[6];

    if(value[6])
        res[6] = not res[6];

    if(value[5])
        res[6] = not res[6];

    if(value[1])
        res[6] = not res[6];

    if(not conf and init_bits[6])
        res[6] = not res[6];
//# BIT 6 END
//# BIT 7
    res[7] = value[0xe] != value[0xf];

    if(value[0xd])
        res[7] = not res[7];

    if(value[0xb])
        res[7] = not res[7];

    if(value[0xa])
        res[7] = not res[7];

    if(value[7])
        res[7] = not res[7];

    if(value[6])
        res[7] = not res[7];

    if(value[2])
        res[7] = not res[7];

    if(not conf and init_bits[7])
        res[7] = not res[7];
//# BIT 7 END
//# BIT 8
    res[8] = value[0xe] != value[0xf];

    if(value[0xc])
        res[8] = not res[8];

    if(value[0xb])
        res[8] = not res[8];

    if(value[8])
        res[8] = not res[8];

    if(value[7])
        res[8] = not res[8];

    if(value[3])
        res[8] = not res[8];

    if(not conf and init_bits[8])
        res[8] = not res[8];
//# BIT 8 END
//# BIT 9
    res[9] = value[0xd] != value[0xf];

    if(value_short == 0x60f)
        res[9] = not res[9];

    if(value[0xc])
        res[9] = not res[9];

    if(value[9])
        res[9] = not res[9];

    if(value[8])
        res[9] = not res[9];

    if(value[4])
        res[9] = not res[9];

    if(not conf and init_bits[9])
        res[9] = not res[9];
//# BIT 9 END
//# BIT 10
    res[0xA] = value[0xd] != value[0xe];

    if(value[0xa])
        res[0xA] = not res[0xA];

    if(value[9])
        res[0xA] = not res[0xA];

    if(value[5])
        res[0xA] = not res[0xA];

    if(not conf and init_bits[0xA])
        res[0xA] = not res[0xA];
//# BIT 10 END
//# BIT 11
    res[0xB] = value[0xe] != value[0xf];

    if(value_short == 0x60f)
        res[0xB] = not res[0xB];

    if(value[0xb])
        res[0xB] = not res[0xB];

    if(value[0xa])
        res[0xB] = not res[0xB];

    if(value[6])
        res[0xB] = not res[0xB];

    if(not conf and init_bits[0xA])
        res[0xB] = not res[0xB];
//# BIT 11 END
//# BIT 12
    res[0xC] = value[0xF];

    if(value[7] != value[8] or value_short == 0x60F)
        res[0xC] = not res[0xC];

    if(value[4])
        res[0xC] = not res[0xC];

    if(value[0])
        res[0xC] = not res[0xC];

    if(not conf and init_bits[0xC])
        res[0xC] = not res[0xC];
//# BIT 12 END
//# BIT 13
    res[0xD] = value[8] != value[9];
    
    if(value[5] or value_short == 0x60f)
        res[0xD] = not res[0xD];

    if(value[1])
        res[0xD] = not res[0xD];

    if(not conf and init_bits[0xD])
        res[0xD] = not res[0xD];
//# BIT 13 END
//# BIT 14
    res[0xE] = value[9] != value[0xA];

    if(value[6] or value_short == 0x60f)
        res[0xE] = not res[0xE];

    if(value[2])
        res[0xE] = not res[0xE];

    if(not conf and init_bits[0xE])
        res[0xE] = not res[0xE];
//# BIT 14 END
//# BIT 15
    res[0xF] = value[0xa] != value[0xb];
    
    if(value[7] or value_short == 0x60f)
        res[0xF] = not res[0xF];

    if(value[3])
        res[0xF] = not res[0xF];

    if(not conf and init_bits[0xF])
        res[0xF] = not res[0xF];
//# BIT 15 END

    delete[] value;
    delete[] init_bits;
    unsigned short code = bitsToShort(res);
    return new byte[2] { (byte)((code & 0xFF00) >> 8), (byte)(code & 0x00FF) };
}