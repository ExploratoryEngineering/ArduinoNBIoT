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

#include <Udp.h>

// IP address for the Horde backend
// #define IP "172.16.7.197"
// Default speed for the serial port
#define DEFAULT_SPEED 9600
// Maximum input buffer size.
#define BUFSIZE 255
// Maximum number of lines.
#define MAXLINES 5

/**
 * User-friendly interface to the SARA N2 module from ublox
 */
class TelenorNBIoT
{
  public:
    enum power_save_mode {
        psm_sleep_after_send = 0,
        psm_sleep_after_response,
        psm_always_on,
    };

    /**
     * Create a new TelenorNBIoT instance. Default apn is the Telenor NB-IoT
     * Developer Portal, "mda.ee", but can be overridden. Use a blank string
     * to get the network default APN. If you specify mobile country code
     * and mobile network code the device will register on the network faster.
     */
    TelenorNBIoT(String accessPointName = "mda.ee", uint16_t mobileCountryCode = 0, uint16_t mobileNetworkCode = 0);

    /**
     * Initialize the module with the specified baud rate. The default is 9600.
     */
    bool begin(Stream &serial);

    /**
     * Set the module power save mode.
     * The default power save mode is psm_sleep_after_send.
     * Available power save modes are psm_always_on, psm_sleep_after_send and
     * psm_sleep_after_response.
     * 
     */
    bool powerSaveMode(power_save_mode psm = psm_sleep_after_send);

    /**
     * Returns true when the board is online, ie there's a GPRS connection
     */
    bool isConnected();

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
    String imei();

    /**
     * Get the IMSI for the SIM chip attached to the module.
     */
    String imsi();

    /**
     * Create a new socket. Call this before attempting to send data with the
     * module.
     */
    bool createSocket();

    /**
     * Receive data.
     */
    bool receive(char *buffer, uint16_t *length, uint16_t *remain = NULL);

    /**
     * Receive any pending data. If there's no data available it will yield an
     * error. The IP address and port of the originating server will be placed
     * in the ipport parameter. If the parameter is NULL they won't be set. The
     * buffer and length parameters can not be set to NULL.
     */
    bool receiveFrom(char *ip, uint16_t *port, char *buffer, uint16_t *length, uint16_t *remain = NULL);

    /**
     * Send UDP packet to remote IP address.
     */
    bool sendBytes(IPAddress remoteIP, const uint16_t port, const char *data, const uint16_t length);
    
    /**
     * Send a string as a UDP packet to remote IP address.
     */
    bool sendString(IPAddress remoteIP, const uint16_t port, String str);

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
     * Returns the u-blox SARA firmware Version
     */
    String firmwareVersion();

    enum registrationStatus_t {
        RS_UNKNOWN = 0,
        RS_NOT_REGISTERED,
        RS_REGISTERED,
        RS_REGISTERING,
        RS_DENIED,
    };

    registrationStatus_t registrationStatus();
    bool isRegistered();
    bool isRegistering();

  private:
    int16_t _socket;
    char _imei[16];
    char _imsi[16];
    uint16_t mcc;
    uint16_t mnc;
    char apn[30];
    Stream* ublox;
    char buffer[BUFSIZE];
    char *lines[MAXLINES];
    power_save_mode m_psm;

    bool dataOn();
    uint8_t readCommand(char **lines);
    void writeCommand(const char *cmd);
    void drain();
    bool setNetworkOperator(uint8_t, uint8_t);
    bool setAccessPointName(const char *accessPointName);
    bool isOK(const char *line);
    bool isError(const char *line);
    int splitFields(char *line, char **fields);
    void writeBuffer(const char *data, uint16_t length);
    bool sendTo(const char *ip, const uint16_t port, const char *data, const uint16_t length);
};

#endif
