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

// Configure mobile country code, mobile network code and access point name
// See https://www.mcc-mnc.com/ for country and network codes
// Mobile Country Code: 242 (Norway)
// Mobile Network Operator: 01 (Telenor)
// Access Point Namme: mda.ee (Telenor NB-IoT Developer Platform)
TelenorNBIoT nbiot(242, 01, "mda.ee");

// The remote IP address to send data packets to
// u-blox SARA N2 does not support DNS
IPAddress remoteIP(172, 16, 7, 197);
int REMOTE_PORT = 31415;

void setup() {
  Serial.begin(9600);

  Serial.print("Connecting to NB-IoT module...\n");
  nbiotSerial.begin(9600);
  nbiot.begin();
  
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
}

void loop() {
  if (nbiot.isConnected()) {
    // Successfully connected to the network

    // Send message to remote server
    nbiot.createSocket();
    if (true == nbiot.sendString(remoteIP, REMOTE_PORT, "Hello, this is Arduino calling")) {
      Serial.println("Successfully sent data");
    } else {
      Serial.println("Failed sending data");
    }
    nbiot.closeSocket();

    // Wait 15 minutes before sending again
    delay(15 * 60 * 1000);
  } else {
    // Not connected yet. Wait 5 seconds before retrying.
    Serial.println("Connecting...");
    delay(5000);
  }
}
