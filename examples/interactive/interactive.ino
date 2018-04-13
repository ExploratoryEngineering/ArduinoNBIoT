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
#define RX_PIN 10
#define TX_PIN 11

TelenorNBIoT nbiot(RX_PIN, TX_PIN, 7000);

void setup() {
  Serial.begin(9600);
  while (!Serial) ;
  printHelp();

  nbiot.begin();
}

void printHelp() {
  Serial.println(F("====================================="));
  Serial.println(F("h. . . . . Display help"));
  Serial.println(F("g. . . . . Check GPRS status (aka connected)"));
  Serial.println(F("n. . . . . Create socket"));
  Serial.println(F("c. . . . . Close socket"));
  Serial.println(F("s. . . . . SendTo"));
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
        if (nbiot.connected()) {
          Serial.println(F("Module is online"));
        } else {
          Serial.println(F("Module is not connected"));
        }
        break;

      case 'i':
        tmp = nbiot.imei();
        Serial.print(F("IMEI = "));
        nbiot.i64toa(tmp, buf);
        Serial.println(buf);
        break;

      case 'I':
        tmp = nbiot.imsi();
        Serial.print(F("IMSI = "));
        nbiot.i64toa(tmp, buf);
        Serial.println(buf);
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
        if (nbiot.send("Hello", 5)) {
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
