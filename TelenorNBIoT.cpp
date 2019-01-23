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
#include "TelenorNBIoT.h"
#include <Arduino.h>
#include <Udp.h>

#define PREFIX "AT+"
#define POSTFIX "\r"
#define SOCR "NSOCR=\"DGRAM\",17,%d,1"
#define SOSTF "NSOSTF="
#define SOCL "NSOCL=%d"
#define RECVFROM "NSORF=%d,255"
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
#define DEFAULT_TIMEOUT 2000
#define LOCAL_PORT 8000

TelenorNBIoT::TelenorNBIoT(String accessPointName, uint16_t mobileCountryCode, uint16_t mobileNetworkCode)
{
    _socket = -1;
    memset(_imei, 0, 16);
    memset(_imsi, 0, 16);

    mcc = mobileCountryCode;
    mnc = mobileNetworkCode;
    strcpy(apn, accessPointName.c_str());
}

bool TelenorNBIoT::begin(Stream &serial)
{
    //Stream &serial
    ublox = &serial;
    // Increase timeout for the module. It might be slow to respond on certain
    // commands.
    ublox->setTimeout(DEFAULT_TIMEOUT);
    while (!ublox) {}
    drain();
    reboot();

    return online() &&
        setNetworkOperator(mcc, mnc) &&
        setAccessPointName(apn);
}

bool TelenorNBIoT::setNetworkOperator(uint8_t mobileCountryCode, uint8_t mobileNetworkCode)
{
    if (mobileCountryCode > 0 && mobileNetworkCode > 0) {
        char buffer[40];
        sprintf(buffer, "COPS=1,2,\"%d%02d\"", mobileCountryCode, mobileNetworkCode);
        writeCommand(buffer);
    } else {
        writeCommand("COPS=0");
        return true;
    }
    return readCommand(lines) == 1 && isOK(lines[0]);
}

bool TelenorNBIoT::setAccessPointName(const char *accessPointName)
{
    if (strlen(accessPointName) == 0) {
        // If the APN is blank, don't override the network default PDP context
        return true;
    }
    
    sprintf(buffer, "CGDCONT=1,\"IP\",\"%s\"", accessPointName);
    writeCommand(buffer);
    if (readCommand(lines) == 1 && !isOK(lines[0])) {
        return false;
    }

    writeCommand("CGACT=1,1");
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
        writeCommand(IMEI);
        if (readCommand(lines) == 2 && isOK(lines[1]))
        {
            // Line 1 contains IMEI ("+CGSN: <15-digit IMEI>")
            char *ptr = lines[0] + 7;
            memcpy(_imei, ptr, strnlen(ptr, 15) + 1);
        }
    }
    return String(_imei);
}

String TelenorNBIoT::imsi()
{
    if (strnlen(_imsi, sizeof _imsi) != 15)
    {
        writeCommand(IMSI);
        if (readCommand(lines) == 2 && isOK(lines[1]))
        {
            // Line contains IMSI ("<15 digit IMSI>")
            memcpy(_imsi, lines[0], strnlen(lines[0], 15) + 1);
        }
    }
    return String(_imsi);
}

bool TelenorNBIoT::createSocket()
{
    if (_socket == -1)
    {
        sprintf(buffer, SOCR, LOCAL_PORT);
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
    writeCommand(REBOOT);
    delay(2000);
    int ret = readCommand(lines);
    return ret > 0 && isOK(lines[ret - 1]);
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
    int found = splitFields(lines[0] + 6, fields);
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
    if (_socket == -1) {
        return false;
    }
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
        int count = splitFields(lines[0], fields);
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

bool TelenorNBIoT::receiveFrom(char *ip, uint16_t *port, char *outbuf, uint16_t *length, uint16_t *remain)
{
    if (_socket == -1) {
        return false;
    }
    if (length == NULL) {
        return false;
    }
    sprintf(buffer, RECVFROM, _socket);
    writeCommand(buffer);
    if (readCommand(lines) == 2 && isOK(lines[1]))
    {
        // Data should be <socket>,<ip>,<port>,<length>,<data>,<remaining length>
        char *fields[10];
        int found = splitFields(lines[0], fields);
        if (found == 6)
        {
            if (ip != NULL)
            {
                strcpy(ip, fields[1]);
            }
            if (port != NULL)
            {
                *port = atoi(fields[2]);
            }
            
            *length = atoi(fields[3]);
            char *pos = fields[4];
            pos++; // inc by 1 to skip double-quote char
            hexToBytes(pos, *length, outbuf);
            outbuf[*length] = 0; // end with null-terminator
            
            if (remain != NULL)
            {
                *remain = atoi(fields[5]);
            }
            return true;
        }
    }
    return false;
}

bool TelenorNBIoT::receive(char *buffer, uint16_t *length, uint16_t *remain)
{
    // TODO(stalehd): Check the originating IP address.
    return receiveFrom(NULL, NULL, buffer, length, remain);
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
        if (readCommand(lines) != 1 || !isOK(lines[1]))
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
    return (line[0] == 'E' && line[1] == 'R' && line[2] == 'R' && line[3] == 'O' && line[4] == 'R');
}

int TelenorNBIoT::splitFields(char *line, char **fields)
{
    int found = 1;
    char *p = line;
    fields[0] = line;
    while (*++p)
    {
        if (*p == ',')
        {
            *p = 0;
            fields[found++] = p + 1;
        }
    }
    return found;
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
    while(ublox->available()) {
        ublox->read();
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
            
            
            // Exit if line is "OK" - this is the end of the response
            if (isOK(lines[lineno]))
            {
                completed = true;
            }
            // ...or if line is "ERROR"
            if (isError(lines[lineno]))
            {
                completed = true;
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
    
    ublox->print(PREFIX);
    ublox->print(cmd);
    ublox->print(POSTFIX);
}
