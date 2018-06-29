/*
   Copyright 2018 Telenor Digital AS

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef TELENOR_NBIOT_H
#define TELENOR_NBIOT_H

#include "SoftwareSerial.h"
// IP address for the Horde backend
#define IP "54.77.39.136"
// Horde port
#define PORT 31415
// Default speed for the serial port
#define DEFAULT_SPEED 9600
// Maximum input buffer size.
#define BUFSIZE 255
// Maximum number of lines.
#define MAXLINES 5
// The default local port. If more than one instance of the class is created
// and used each instance should have its own unique port number.
#define LOCAL_PORT 8000

/**
 * User-friendly interface to the SARA N2 module from ublox
 */
class TelenorNBIoT
{
  public:
    /**
     * Attach to module connected to specified pins. Consult the documentation
     * to see which pins your board supports.
     */
    TelenorNBIoT(int rx, int tx, uint16_t local_port = LOCAL_PORT);

    /**
     * Initialize the module with the specified baud rate. The default is 9600.
     */
    bool begin(uint16_t speed = DEFAULT_SPEED);

    /**
     * Returns true when the board is online, ie there's a GPRS connection
     */
    bool connected();

    /**
     * Register the module on the mobile network. This sets the operator
     * selection to automatic, ie it will connect automatically.
     */
    bool online();

    /**
     * Unregister the module from the mobile network. It might take a few seconds
     * before the module unregisters. When the module is offline any attempts
     * to send data will yield errors.
     */
    bool offline();

    /**
     * Get the IMEI for the module. This is also printed on top of the
     * module itself.
     */
    unsigned long long imei();

    /**
     * Get the IMSI for the SIM chip attached to the module.
     */
    unsigned long long imsi();

    /**
     * Create a new socket. Call this before attempting to send data with the
     * module.
     */
    bool createSocket();

    /**
     * Send UDP packet to an IP address. Note that the module doesn't support
     * DNS so you have to specify an IP address.
     */
    bool sendTo(const char *ip, const uint16_t port, const char *data, const uint16_t length);

    /**
     * Receive any pending data. If there's no data available it will yield an
     * error. The IP address and port of the originating server will be placed
     * in the ipport parameter. If the parameter is NULL they won't be set. The
     * buffer and length parameters can not be set to NULL.
     */
    bool receiveFrom(char *ip, uint16_t *port, char *buffer, uint16_t *length, uint16_t *remain = NULL);

    /**
     * Send data to the Horde backend.
     */
    bool send(const char *data, const uint16_t length);

    /**
     * Receive data.
     */
    bool receive(char *buffer, uint16_t *length, uint16_t *remain = NULL);

    /**
     * Close the socket. This will release any resources allocated on the
     * module. When the socket is closed you can't send or receive data.
     */
    bool closeSocket();

    /**
     * Reboot the module. This normally takes three-four seconds. The module
     * comes back online a few seconds after is has rebooted.
     */
    bool reboot();

    /**
     * Returns the Received Signal Strength Indication from the mobile terminal.
     * In dedicated mode, during the radio channel reconfiguration (e.g.
     * handover), invalid measurements may be returned for a short transitory
     * because the mobile terminal must compute them on the newly assigned
     * channel.
     * Returns 99 if RSSI is not detectable
     */
    int rssi();

    /**
     * Helper function to convert IMSI and IMEI strings into 64 bit integers.
     */
    unsigned long long atoi64(const char *str);

    /**
     * Helper function to convert IMSI and IMEI numbers into strings.
     */
    void i64toa(unsigned long long val, char *buffer);

  private:
    int16_t m_socket;
    unsigned long long m_imei;
    unsigned long long m_imsi;
    uint16_t m_local_port;
    SoftwareSerial ublox;
    char buffer[BUFSIZE];
    char *lines[MAXLINES];

    bool dataOn();
    uint8_t readCommand(char **lines);
    void writeCommand(const char *cmd);
    bool isOK(const char *line);
    bool isError(const char *line);
    int splitFields(char *line, char **fields);
    void addHeader();
    void writeBuffer(const char *data, uint16_t length);
};

#endif
