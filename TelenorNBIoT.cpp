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
#include <Arduino.h>
#include "func.h"
#include "TelenorNBIoT.h"
#include <Udp.h>

#define PREFIX "AT+"
#define POSTFIX "\r"
#define SOCR "NSOCR=\"DGRAM\",17,%d,1"
#define SOSTF "NSOSTF="
#define SOCL "NSOCL=%d"
#define RECVFROM "NSORF=%d,%d"
#define GPRS "CGATT?"
#define REG_STATUS "CEREG?"
#define IMSI "CIMI"
#define IMEI "CGSN=1"
#define REBOOT "NRB"
#define RADIO_ON "CFUN=1"
#define RADIO_OFF "CFUN=0"
#define SIGNAL_STRENGTH "CSQ"
#define CONNECT_DATA "CGATT=1"
#define FIRMWARE "CGMR"
#define READ_APN "CGDCONT?"
#define SET_APN "CGDCONT=%d,\"IP\",\"%s\""
#define ACTIVATE_APN "CGACT=1,%d"
#define CONFIG_AUTOCONN "NCONFIG=\"AUTOCONNECT\",\"%s\""
#define DEFAULT_TIMEOUT 2000

int splitFields(char *line, char **fields, uint8_t maxFields);
bool retry(uint8_t attempts, nonstd::function<bool ()> fn, uint16_t delayBetween = 100);

TelenorNBIoT::TelenorNBIoT(String accessPointName, uint16_t mobileCountryCode, uint16_t mobileNetworkCode)
{
    _socket = -1;
    memset(_imei, 0, 16);
    memset(_imsi, 0, 16);

    mcc = mobileCountryCode;
    mnc = mobileNetworkCode;
    strcpy(apn, accessPointName.c_str());
}

bool TelenorNBIoT::begin(Stream &serial, bool _debug)
{
    debug = _debug;
    if (debug) {
        Serial.println("NB-IoT debug enabled");
    }
    //Stream &serial
    ublox = &serial;
    // Increase timeout for the module. It might be slow to respond on certain
    // commands.
    ublox->setTimeout(DEFAULT_TIMEOUT);
    while (!ublox) {}
    drain();
    setAutoConnect(false);
    reboot();

    return online() &&
        setNetworkOperator(mcc, mnc) &&
        ensureAccessPointName(apn);
}

bool TelenorNBIoT::enableErrorCodes()
{
    // Enable error codes for u-blox SARA N2 errors
    return retry(10, [this]() {
        writeCommand("CMEE=1");
        return readCommand(lines) == 1 && isOK(lines[0]);
    });
}

bool TelenorNBIoT::setNetworkOperator(uint8_t mobileCountryCode, uint8_t mobileNetworkCode)
{
    if (mobileCountryCode > 0 && mobileNetworkCode > 0) {
        char buffer[40];
        sprintf(buffer, "COPS=1,2,\"%d%02d\"", mobileCountryCode, mobileNetworkCode);
        writeCommand(buffer);
    } else {
        writeCommand("COPS=0");
    }
    return readCommand(lines) == 1 && isOK(lines[0]);
}

bool TelenorNBIoT::ensureAccessPointName(const char *accessPointName)
{
    char *currentAPN = readAccessPointName();
    if (currentAPN != NULL && strcmp(currentAPN, accessPointName) == 0)
    {
        return true;
    }
    return setAccessPointName(accessPointName);
}

char* TelenorNBIoT::readAccessPointName()
{
    writeCommand(READ_APN);
    int count = readCommand(lines);
    if (!isOK(lines[count-1]))
    {
        return NULL;
    }

    // We only care about context ID 0, but the contexts might be out of order,
    // so we have too loop through all to make sure we find it.
    for (uint8_t i=0; i<count-1; i++)
    {
        // +CGDCONT: 0,"IP","mda.ee",,0,0,,,,,1
        char* fields[3];
        if (splitFields(lines[i] + 10, fields, 3) >=3)
        {
            int contextId = atoi(fields[0]);
            char* apn = fields[2];
            
            if (contextId == 0)
            {
                return apn;
            }
        }
    }

    return NULL;
}

bool TelenorNBIoT::setAccessPointName(const char *accessPointName)
{
    if (strlen(accessPointName) == 0)
    {
        // If the APN is blank, don't override the network default PDP context
        return true;
    }

    return retry(3, [this, accessPointName]() {
        sprintf(buffer, SET_APN, 0, accessPointName);
        writeCommand(buffer);
        return readCommand(lines) == 1 && isOK(lines[0]);
    });
}

bool TelenorNBIoT::setAutoConnect(bool enabled)
{
    sprintf(buffer, CONFIG_AUTOCONN, enabled ? "TRUE" : "FALSE");
    writeCommand(buffer);
    return readCommand(lines) == 1 && isOK(lines[0]);
}

bool TelenorNBIoT::dataOn()
{
    writeCommand(CONNECT_DATA);
    if (readCommand(lines) == 1 && isOK(lines[0]))
    {
        return true;
    }
    return false;
}

bool TelenorNBIoT::isConnected()
{
    writeCommand(GPRS);
    if (readCommand(lines) == 2 && isOK(lines[1]))
    {
        // Line contains "+CGATT: <1:available, 0:not available"
        // The GPRS status isn't strictly the online/offline indicator but
        // reasonable close. It will be available if the module is online
        return (*(lines[0] + 8) == '1');
    }
    return false;
}

TelenorNBIoT::registrationStatus_t TelenorNBIoT::registrationStatus()
{
    int statusNum = -1;
    writeCommand(REG_STATUS);
    if (readCommand(lines) == 2 && isOK(lines[1]))
    {
        // Line contains "+CEREG: <n>,<status>"
        statusNum = atoi((lines[0] + 10));
    }
    
    if (statusNum == 0) {
        return RS_NOT_REGISTERED;
    } else if (statusNum == 1) {
        return RS_REGISTERED;
    } else if (statusNum == 2) {
        return RS_REGISTERING;
    } else if (statusNum == 3) {
        return RS_DENIED;
    }
    return RS_UNKNOWN;
}

bool TelenorNBIoT::isRegistered()
{
    return registrationStatus() == RS_REGISTERED;
}

bool TelenorNBIoT::isRegistering()
{
    return registrationStatus() == RS_REGISTERING;
}

String TelenorNBIoT::imei()
{
    if (strnlen(_imei, sizeof _imei) != 15)
    {
        retry(10, [this]() {
            writeCommand(IMEI);
            if (readCommand(lines) == 2 && isOK(lines[1]))
            {
                // Line 1 contains IMEI ("+CGSN: <15-digit IMEI>")
                char *ptr = lines[0] + 7;
                if (strnlen(ptr, 16) == 15) {
                    memcpy(_imei, ptr, 16);
                    return true;
                }
            }
            
            return false;
        });
    }
    return String(_imei);
}

String TelenorNBIoT::imsi()
{
    if (strnlen(_imsi, sizeof _imsi) != 15)
    {
        retry(10, [this]() {
            writeCommand(IMSI);
            if (readCommand(lines) == 2 && isOK(lines[1]) && strnlen(lines[0], 16) == 15)
            {
                // Line contains IMSI ("<15 digit IMSI>")
                memcpy(_imsi, lines[0], strnlen(lines[0], 15) + 1);
                return true;
            }
            return false;
        });
    }
    return String(_imsi);
}

bool TelenorNBIoT::createSocket(const uint16_t listenPort)
{
    if (_socket == -1)
    {
        sprintf(buffer, SOCR, listenPort);
        writeCommand(buffer);
        if (readCommand(lines) == 2 && isOK(lines[1]))
        {
            _socket = atoi(lines[0]);
            return true;
        }
    }
    return false;
}

bool TelenorNBIoT::closeSocket()
{
    if (_socket > -1)
    {
        sprintf(buffer, SOCL, _socket);
        writeCommand(buffer);
        if (readCommand(lines) == 1 && isOK(lines[0]))
        {
            _socket = -1;
            return true;
        }
    }
    return false;
}

bool TelenorNBIoT::reboot()
{
    _socket = -1;
    return retry(3, [this]() {
        writeCommand(REBOOT);
        delay(2000);
        int ret = readCommand(lines);
        return ret > 0 && isOK(lines[ret - 1]);
    }) && enableErrorCodes();
}

bool TelenorNBIoT::online()
{
    writeCommand(RADIO_ON);
    return readCommand(lines) == 1 && isOK(lines[0]);
}

bool TelenorNBIoT::offline()
{
    writeCommand(RADIO_OFF);
    return readCommand(lines) == 1 && isOK(lines[0]);
}

int TelenorNBIoT::rssi()
{
    int rssi = 99;
    writeCommand(SIGNAL_STRENGTH);
    int ret = readCommand(lines);
    if (ret != 2 || !isOK(lines[1]))
    {
        return rssi;
    }
    char* fields[2];
    int found = splitFields(lines[0] + 6, fields, 2);
    if (found != 1)
    {
        return rssi;
    }
    rssi = atoi(fields[0]);
    if (rssi == 99) {
      return rssi;
    }
    return -113 + rssi * 2;
}

int TelenorNBIoT::errorCode()
{
    return _errCode;
}

String TelenorNBIoT::firmwareVersion()
{
    writeCommand(FIRMWARE);
    int ret = readCommand(lines);
    if (ret != 2 || !isOK(lines[1]))
    {
        return "ERROR";
    }
    return String(lines[0]);
}

void TelenorNBIoT::writeBuffer(const char *data, uint16_t length)
{
    for (int i = 0; i < length; i++)
    {
        unsigned char ch1 = (data[i] & 0xF0) >> 4;
        unsigned char ch2 = (data[i] & 0x0F);
        if (ch1 <= 9)
        {
            ch1 = (char)('0' + ch1);
        }
        else
        {
            ch1 = (char)('A' + ch1 - 10);
        }
        if (ch2 <= 9)
        {
            ch2 = (char)('0' + ch2);
        }
        else
        {
            ch2 = (char)('A' + ch2 - 10);
        }
        ublox->write(ch1);
        ublox->write(ch2);
    }
}

bool TelenorNBIoT::sendTo(const char *ip, const uint16_t port, const char *data, const uint16_t length)
{
    ublox->print(PREFIX);
    ublox->print(SOSTF);
    ublox->print(_socket);
    ublox->print(",\"");
    ublox->print(ip);
    ublox->print("\",");
    ublox->print(port);
    ublox->print(",");
    
    if (m_psm == psm_always_on) {
        ublox->print("0x000");
    } else if (m_psm == psm_sleep_after_send) {
        ublox->print("0x200");
    } else if (m_psm == psm_sleep_after_response) {
        ublox->print("0x400");
    }

    ublox->print(",");
    ublox->print(length);
    ublox->print(",\"");

    writeBuffer(data, length);

    ublox->print("\"");
    ublox->print(POSTFIX);

    if (readCommand(lines) == 2 && isOK(lines[1]))
    {
        char *fields[10];
        int count = splitFields(lines[0], fields, 10);
        if (count == 2)
        {
            // Found two fields. First is socket no
            uint16_t socketNo = atoi(fields[0]);
            uint16_t bytes = atoi(fields[1]);
            if (socketNo == _socket && bytes == length)
            {
                return true;
            }
        }
    }
    return false;
}

bool TelenorNBIoT::sendBytes(IPAddress remoteIP, const uint16_t port, const char *data, const uint16_t length)
{
    memcpy(buffer, data, length);
    char ip[16];
    sprintf(ip, "%d.%d.%d.%d", remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3]);
    return sendTo(ip, port, buffer, length);
}

bool TelenorNBIoT::sendString(IPAddress remoteIP, const uint16_t port, String str)
{
    return sendBytes(remoteIP, port, str.c_str(), str.length());
}

size_t TelenorNBIoT::receiveBytes(char *outbuf, uint16_t bufferLength)
{
    sprintf(buffer, RECVFROM, _socket, bufferLength);
    writeCommand(buffer);
    if (readCommand(lines) == 2 && isOK(lines[1]))
    {
        // Fields should be <socket>,<ip>,<port>,<length>,<data>,<remaining length>
        char *fields[10];
        int found = splitFields(lines[0], fields, 10);
        if (found == 6)
        {
            _receivedFromIP.fromString(fields[1]);
            _receivedFromPort = atoi(fields[2]);
            size_t readLength = atoi(fields[3]);
            hexToBytes(fields[4], readLength, outbuf);
            _receivedBytesRemaining = atoi(fields[5]);
            return readLength;
        }
    }
    return 0;
}

size_t TelenorNBIoT::receivedBytesRemaining()
{
    return _receivedBytesRemaining;
}

IPAddress TelenorNBIoT::receivedFromIP()
{
    return _receivedFromIP;
}

uint16_t TelenorNBIoT::receivedFromPort()
{
    return _receivedFromPort;
}

bool TelenorNBIoT::powerSaveMode(power_save_mode psm)
{
    m_psm = psm;

    if (m_psm == psm_sleep_after_send || m_psm == psm_sleep_after_response)
    {
        // disable eDRX
        writeCommand("CEDRXS=3,5");

        // enable Power Save Mode and set active time to as low as possible
        // mode (0 - disable PSM, 1 - enable PSM, 2 - disable PSM and reset all params)
        // Requested Periodic RAU (N/A)
        // Requested GPRS READY timer (N/A)
        // Requested Periodic TAU (T3412) - 
        // Requested Active Time (T3324) - 
        writeCommand("CPSMS=1,,,\"01000001\",\"00000000\"");
        return (readCommand(lines) == 1 && isOK(lines[0]));
    }
    else
    {
        // set eDRX to default value
        writeCommand("CEDRXS=0,5");
        if (readCommand(lines) != 1 || !isOK(lines[0]))
        {
            return false;
        }

        // disable Power Save Mode and reset all PSM parameters to factory values
        writeCommand("CPSMS=2");
        return (readCommand(lines) == 1 && isOK(lines[0]));
    }
}

bool TelenorNBIoT::isOK(const char *line)
{
    return (line[0] == 'O' && line[1] == 'K');
}

bool TelenorNBIoT::isError(const char *line)
{
    int i = 0;
    if (line[i] == '+') {
        i = 5;
    }
    return (line[i++] == 'E' && line[i++] == 'R' && line[i++] == 'R' && line[i++] == 'O' && line[i++] == 'R');
}

int TelenorNBIoT::parseErrorCode(const char *line)
{
    if (!isError(line)) {
        return -1;
    } else if (line[0] != '+') {
        return -2;
    }
    line += 12;
    int errCode = atoi(line);
    return errCode;
}

void TelenorNBIoT::hexToBytes(const char *hex, const uint16_t byte_count, char *bytes)
{
    const uint16_t hex_count = byte_count*2;
    for (int i=0; i<hex_count; i++) {
        char c = hex[i];
        if (c >= 48 && c <= 57) {
            c -= 48;
        } else if (c >= 65 && c <= 70) {
            c -= 55;
        } else if (c >= 97 && c <= 102) {
            c -= 87;
        } else {
            c = 0;
        }

        if (i%2 == 0) {
            bytes[i/2] = c << 4;
        } else {
            bytes[i/2] += c;
        }
    }
}

void TelenorNBIoT::drain()
{
    if (ublox->available()) {
        if (debug) Serial.println("---DRAINING---");
        while(ublox->available()) {
            char c = ublox->read();
            if (debug) Serial.print(c);
        }
        if (debug) Serial.println("--------------");
    }
}

uint8_t TelenorNBIoT::readCommand(char **lines)
{
    uint8_t read = 1;
    uint8_t lineno = 0;
    uint8_t offset = 0;
    bool completed = false;
    drain();
    while (read > 0 && !completed && lineno < MAXLINES)
    {
        read = ublox->readBytesUntil('\n', (buffer + offset), (BUFSIZE - offset));
        if (read == 1 && buffer[offset] == '\r')
        {
                offset += 1;
        }
        else if (read > 0)
        {
            lines[lineno] = (buffer + offset);
            if (buffer[offset + read - 1] == '\r')
            {
                // replace carrige-return with string terminator
                buffer[offset + read - 1] = 0;
                offset += (read);
            } else {
                // insert string terminator after last read char
                buffer[offset + read] = 0;
                offset += (read + 1);
            }

            if (debug) {
                Serial.print("Response line: ");
                Serial.println(lines[lineno]);
            }
            
            
            // Exit if line is "OK" - this is the end of the response
            if (isOK(lines[lineno]))
            {
                completed = true;
            }
            // ...or if line is "ERROR"
            if (isError(lines[lineno]))
            {
                completed = true;
                _errCode = parseErrorCode(lines[lineno]);
            }

            lineno++;
        }
        
    }
    return lineno;
}

void TelenorNBIoT::writeCommand(const char *cmd)
{
    uint8_t n = 0;
    drain();
    _errCode = -1;

    if (debug) {
        Serial.print("Write command: ");
        Serial.print(PREFIX);
        Serial.println(cmd);
    }
    
    ublox->print(PREFIX);
    ublox->print(cmd);
    ublox->print(POSTFIX);
}

/**
 * Splits a comma separated and null terminated response line from u-blox.
 * Line should be a pointer to the first character of the first field (not
 * the beginning of the line). It will modify the input line by replacing
 * comma and double quotes with `\0`. Each field will point to the first
 * character of each field. Returns the number of found fields.
 */
int splitFields(char *line, char **fields, uint8_t maxFields)
{
    fields[0] = line;
    int found = 1;
    for(char *p = line; *p != '\0'; p++) {
        if (*p == '"')
        {
            // replace double-quote with null-terminator
            *p = '\0';
            
            // if opening quote - move pointer one ahead
            int current = found-1;
            if (p == fields[current])
            {
                fields[current] = p + 1;
            }
        }
        else if (*p == ',')
        {
            *p = '\0';
            if (found < maxFields)
            {
                fields[found++] = p + 1;
            }
        }
    }
    return found;
}

bool retry(uint8_t attempts, nonstd::function<bool ()> fn, uint16_t delayBetween)
{
    while (attempts--)
    {
        if (fn()) {
            return true;
        }
        delay(delayBetween);
    }
    return false;
}
