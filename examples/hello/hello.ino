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

/**********************************************************************
   Selecting u-blox serial port
   
   The communication between the Arduino board and the u-blox NB-IoT
   module uses serial communication. When you write Serial.print(...)
   in Arduino it sends a message over USB (Universal Serial Bus) to
   your computer. Arduino UNO only have the USB serial port, so then
   we have to use SoftwareSerial to create extra serial ports. Most
   other boards have extra hardware serial ports (Serial1, Serial2,
   etc). So for those it's better to use a hardware serial port. Check
   your board documentation what ports you have and what pins they are
   mapped to, then comment/uncomment below.
***********************************************************************/
// For Arduino Uno and other boards with only USB serial:
#include <SoftwareSerial.h>
SoftwareSerial ublox(10, 11);
// When the Arduino board has extra hardware serial ports, comment out
// the 2 SoftwareSerial-lines above and uncomment the line below:
//#define ublox Serial1

TelenorNBIoT nbiot;

// The remote IP address to send data packets to
// u-blox SARA N2 does not support DNS
IPAddress remoteIP(172, 16, 15, 14);
int REMOTE_PORT = 1234;

void setup() {
  Serial.begin(9600);

  Serial.print("Connecting to NB-IoT module...\n");
  ublox.begin(9600);
  nbiot.begin(ublox);
  
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

  nbiot.createSocket();
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

    // Wait 15 minutes before sending again
    delay(15 * 60 * 1000);
  } else {
    // Not connected yet. Wait 5 seconds before retrying.
    Serial.println("Connecting...");
    delay(5000);
  }
}
