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

// What port to listen on for receiving data
int LOCAL_PORT = 1234;

// Buffer to store received data
const uint16_t bufferLength = 16;
char buffer[bufferLength];

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.print("Connecting to NB-IoT module...\n");
  ublox.begin(9600);

  // Try to initialize the NB-IoT module until it succeeds
  while (!nbiot.begin(ublox)) {
    Serial.println("Begin failed. Retrying...");
    delay(1000);
  }
  
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

  // Try to disable power save mode until it succeeds
  while (!nbiot.powerSaveMode(TelenorNBIoT::psm_always_on)) {
    Serial.print("Error disabling power save mode. Error code: ");
    Serial.println(nbiot.errorCode(), DEC);
    delay(100);
  }

  // Try to create a socket until it succeeds
  while (!nbiot.createSocket(LOCAL_PORT)) {
    Serial.print("Error creating socket. Error code: ");
    Serial.println(nbiot.errorCode(), DEC);
    delay(100);
  }

  Serial.println("Waiting for downstream messages");
}

void loop() {
  // As long as received bytes are available
  while (int bytesReceived = nbiot.receiveBytes(buffer, bufferLength)) {
    Serial.print("Received data from ");
    Serial.print(nbiot.receivedFromIP());
    Serial.print(":");
    Serial.println(nbiot.receivedFromPort());
    
    Serial.print("Message: ");
    for (uint16_t i=0; i<bytesReceived; i++) {
      Serial.print(String(buffer[i]));
    }

    Serial.print("\nBytes received: ");
    Serial.println(bytesReceived);
    int remaining = nbiot.receivedBytesRemaining();
    if (remaining) {
      Serial.print("Remaning bytes: ");
      Serial.println(remaining);
    }
  }
 
  // Wait 5 seconds
  delay(5000);
}
