/***********************************************************************

  NB-IoT Hello World

  Creates an NB-IoT class, connects and sends a message every 10
  seconds.

  - Connect the RX and TX pins on the module to pin 10 and 11 on the
    Arduino.
  - Connect the power pins (5V and GND) to the module's VDD and GND
    pins.

  This example is in the public domain.

  Read more on the Exploratory Engineering team at
  https://exploratory.engineering/

***********************************************************************/

#include "TelenorNBIoT.h"

#define RX_PIN 10
#define TX_PIN 11

TelenorNBIoT nbiot = TelenorNBIoT(RX_PIN, TX_PIN);

void setup()
{
  Serial.begin(9600);
  Serial.print("Connecting NB-IoT module...\n");
  nbiot.begin();
}

void loop()
{
  if (nbiot.connected())
  {
    Serial.print("Sending message...\n");
    nbiot.createSocket();
    const char *hello = "Hello, this is Arduino calling";
    nbiot.send(hello, strlen(hello));
    nbiot.closeSocket();
  }
  else
  {
    Serial.print("Waiting for connection...\n");
  }
  delay(10000);
}