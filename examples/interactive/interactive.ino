/**
 * This is an interactive console for the uBlox SARA N2 breakout from
 * Exploratory Engineering. The module is connected to pin 10 and 11 on the
 * Arduino. Note that the module uses the uBlox convention of RX and TX markings
 * so TX on the module must be connected to pin 11 and RX on pin 10.
 *
 * Connect the GND and VCC pins on the module to the corresponding pins on the
 * Arduino (GND and 5V or GND and 3.3V) and you are ready to go.
 *
 * This example uses the Horde backend to receive data
 * (at https://nbiot.engineering/) but the send() function can be replaced with
 * sendTo() if you want to send data somewhere else.
 *
 * Use the Serial Monitor to send commands to the module.
 *
 *
 * This example is in the public domain.
 *
 * Read more on the Exploratory Engineering team at
 * https://exploratory.engineering/
 */
#include "TelenorNBIoT.h"

#ifdef SERIAL_PORT_HARDWARE_OPEN
/*
 * For Arduino boards with a hardware serial port separate from USB serial.
 * This is usually mapped to Serial1. Check which pins are used for Serial1 on
 * the board you're using.
 */
#define nbiotSerial SERIAL_PORT_HARDWARE_OPEN
#else
/*
 * For Arduino boards with only one hardware serial port (like Arduino UNO). It
 * is mapped to USB, so we use SoftwareSerial on pin 10 and 11 instead.
 */
#include <SoftwareSerial.h>
SoftwareSerial nbiotSerial(10, 11);
#endif

// Configure mobile country code, mobile network code and access point name
// See https://www.mcc-mnc.com/ for country and network codes
// Mobile Country Code: 242 (Norway)
// Mobile Network Operator: 01 (Telenor)
// Access Point Namme: mda.ee (Telenor NB-IoT Developer Platform)
TelenorNBIoT nbiot(242, 01, "mda.ee");

IPAddress remoteIP(172, 16, 15, 14);
int REMOTE_PORT = 1234;

void setup() {
  Serial.begin(9600);
  while (!Serial) ;
  printHelp();

  nbiotSerial.begin(9600);
  nbiot.begin(nbiotSerial);
}

void printHelp() {
  Serial.println(F("====================================="));
  Serial.println(F("h. . . . . Display help"));
  Serial.println(F("g. . . . . Check GPRS status (aka connected)"));
  Serial.println(F("n. . . . . Create socket"));
  Serial.println(F("c. . . . . Close socket"));
  Serial.println(F("s. . . . . Send packet \"Hello\""));
  Serial.println(F("r. . . . . ReciveFrom"));
  Serial.println(F("i. . . . . IMEI"));
  Serial.println(F("I. . . . . IMSI"));
  Serial.println(F("x. . . . . error command"));
  Serial.println(F("b. . . . . reboot"));
  Serial.println(F("o. . . . . Go online"));
  Serial.println(F("O. . . . . Go offline"));
  Serial.println(F("====================================="));
}

unsigned long long tmp;
char buf[16];
char databuf[32];
uint16_t tmplen = 32;

void loop() {
  if (Serial.available()) {
    switch (Serial.read()) {
      case 'h':
        printHelp();
        break;

      case 'g':
        if (nbiot.isConnected()) {
          Serial.println(F("Module is online"));
        } else {
          Serial.println(F("Module is not connected"));
        }
        break;

      case 'i':
        Serial.print(F("IMEI = "));
        Serial.println(nbiot.imei());
        break;

      case 'I':
        Serial.print(F("IMSI = "));
        Serial.println(nbiot.imsi());
        break;

      case 'n':
        if (nbiot.createSocket()) {
          Serial.println(F("Created socket"));
        } else {
          Serial.println(F("Unable to create socket"));
        }
        break;

      case 'c':
        if (nbiot.closeSocket()) {
          Serial.println(F("Closed socket"));
        } else {
          Serial.println(F("Unable to close socket"));
        }
        break;

      case 's':
        if (nbiot.sendString(remoteIP, REMOTE_PORT, "Hello")) {
          Serial.println(F("Data sent"));
        } else {
          Serial.println(F("Unable to send data"));
        }
        break;

      case 'r':
        if (nbiot.receiveFrom(NULL, NULL, databuf, &tmplen)) {
          Serial.println(F("Received data"));
        } else {
          Serial.println(F("No data received"));
        }

      case 'b':
        if (nbiot.reboot()) {
          Serial.println(F("Rebooted successfully"));
        } else {
          Serial.println(F("Error rebooting"));
        }
        break;

      case 'O':
        if (nbiot.offline()) {
          Serial.println(F("Radio turned off"));
        } else {
          Serial.println(F("Unable to turn off radio"));
        }
        break;

      case 'o':
        if (nbiot.online()) {
          Serial.println(F("Radio turned on"));
        } else {
          Serial.println(F("Unable to turn radio on"));
        }
        break;

      default:
        break;
    }
  }
}
