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
    bool begin(Stream &serial, bool debug = false);

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
     * Create a new socket. Call this before attempting to send or receive data
     * with the module. Optionally specify what port to listen on.
     */
    bool createSocket(const uint16_t listenPort = 1234);

    /**
     * Receive bytes. Pass in a char array to store the bytes and the length of
     * the array. Returns the number of bytes received. If there's no data
     * available it will return 0. Check receivedBytesRemaining() to see if
     * there are more bytes available in case the buffer was too small to fill
     * all data received.
     * Check where the data originated from by calling receivedFromIP() and
     * receivedFromPort().
     */
    size_t receiveBytes(char *outbuf, uint16_t bufferLength);

    /**
     * Number of remaining bytes received
     */
    size_t receivedBytesRemaining();

    /**
     * Get the remote IP address of the last received package
     */
    IPAddress receivedFromIP();

    /**
     * Get the remote port of the last received package
     */
    uint16_t receivedFromPort();

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
     * Get the error code of the previous command. See Appendix A in the SARA
     * N2 AT Commands guide for the description of each error code.
     */
    int errorCode();

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
    bool debug;
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
    int _errCode = -1;
    IPAddress _receivedFromIP;
    uint16_t _receivedFromPort = 0;
    size_t _receivedBytesRemaining = 0;

    bool enableErrorCodes();
    bool setAutoConnect(bool enabled);
    bool dataOn();
    uint8_t readCommand(char **lines);
    void writeCommand(const char *cmd);
    void drain();
    bool setNetworkOperator(uint8_t, uint8_t);
    bool ensureAccessPointName(const char *accessPointName);
    char* readAccessPointName();
    bool setAccessPointName(const char *accessPointName);
    bool isOK(const char *line);
    bool isError(const char *line);
    int parseErrorCode(const char *line);
    void hexToBytes(const char *hex, const uint16_t byte_count, char *bytes);
    void writeBuffer(const char *data, uint16_t length);
    bool sendTo(const char *ip, const uint16_t port, const char *data, const uint16_t length);
};

#endif
