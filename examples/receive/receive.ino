/***********************************************************************

  Telenor NB-IoT Hello World

  Configures NB-IoT, connects and sends a message every 15 minutes.

  See our guide on how to connect the EE-NBIOT-01 to Arduino:
  https://docs.nbiot.engineering/tutorials/arduino-basic.html

  This example is in the public domain.

  Read more on the Exploratory Engineering team at
  https://exploratory.engineering/

***********************************************************************/
#include <Udp.h>
#include <TelenorNBIoT.h>

#ifdef SERIAL_PORT_HARDWARE_OPEN
/*
 * For Arduino boards with a hardware serial port separate from USB serial.
 * This is usually mapped to Serial1. Check which pins are used for Serial1 on
 * the board you're using.
 */
#define ublox SERIAL_PORT_HARDWARE_OPEN
#else
/*
 * For Arduino boards with only one hardware serial port (like Arduino UNO). It
 * is mapped to USB, so we use SoftwareSerial on pin 10 and 11 instead.
 */
#include <SoftwareSerial.h>
SoftwareSerial ublox(10, 11);
#endif

TelenorNBIoT nbiot;

// The remote IP address to send data packets to
// u-blox SARA N2 does not support DNS
IPAddress remoteIP(172, 16, 15, 14);
int REMOTE_PORT = 1234;

void setup() {
  Serial.begin(9600);
  while(!Serial);

  Serial.print("Connecting to NB-IoT module...\n");
  ublox.begin(9600);
  while(!nbiot.begin(ublox));
  
  /*
   * You neeed the IMEI and IMSI when setting up a device in our developer
   * platform: https://nbiot.engineering
   * 
   * See guide for more details on how to get started:
   * https://docs.nbiot.engineering/tutorials/getting-started.html
   */
  Serial.print("IMSI: ");
  Serial.println(nbiot.imsi());
  Serial.print("IMEI: ");
  Serial.println(nbiot.imei());

  nbiot.powerSaveMode(TelenorNBIoT::psm_always_on);
  nbiot.createSocket();

  Serial.println("Waiting for downstream messages");
}

void loop() {
  char buf[16] = { 0 };
  uint16_t length = 0;

  if (nbiot.receive(buf, &length, NULL)) {
    Serial.println("Received message:");
    Serial.print(String(buf));
    Serial.print("\nLength: ");
    Serial.println(length);
  }
 
  delay(5000);
}
