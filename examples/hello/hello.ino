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

/*
 * Create an nbiot instance using default settings, which is the Telenor NB-IoT
 * Development Platform's APN (mda.ee) and 0 for both mobile country code and
 * mobile operator code (auto mode).
 * 
 * It's also possible to specify a different APN, mobile country code and
 * mobile network code. This should speed up attaching to the network.
 * 
 * Example:
 * TelenorNBIoT nbiot("mda.ee", 242, 01); // Telenor Norway
 * 
 * See list of codes here: https://www.mcc-mnc.com/
 */
TelenorNBIoT nbiot;

// The remote IP address to send data packets to
// u-blox SARA N2 does not support DNS
IPAddress remoteIP(172, 16, 15, 14);
int REMOTE_PORT = 1234;

// How often we want to send a message, specified in milliseconds
// 15 minutes = 15 (min) * 60 (sec in min) * 1000 (ms in sec)
unsigned long INTERVAL_MS = (unsigned long) 15 * 60 * 1000;

void setup() {
  // Open a serial connection over USB for Serial Monitor
  Serial.begin(9600);

  // Useful for some Arduino boards to wait for the Serial Monitor to open
  while (!Serial);

  // Open the serial connection between the Arduino board and the NB-IoT module
  ublox.begin(9600);
  
  // Try to initialize the NB-IoT module until it succeeds
  Serial.print("Connecting to NB-IoT module...\n");
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

  // Try to create a socket until it succeeds
  while (!nbiot.createSocket()) {
    Serial.print("Error creating socket. Error code: ");
    Serial.println(nbiot.errorCode(), DEC);
    delay(100);
  }
}

void loop() {
  if (nbiot.isConnected()) {
    // Successfully connected to the network

    // Send message to remote server
    if (true == nbiot.sendString(remoteIP, REMOTE_PORT, "Hello, this is Arduino calling")) {
      Serial.println("Successfully sent data");
    } else {
      Serial.println("Failed sending data");
    }

    // Wait specified time before sending again (see definition above)
    delay(INTERVAL_MS);
  } else {
    // Not connected yet. Wait 5 seconds before retrying.
    Serial.println("Connecting...");
    delay(5000);
  }
}
