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

#define PREFIX "AT+"
#define POSTFIX "\r\n"
#define SOCR "NSOCR=\"DGRAM\",17,%d,1"
#define SOST "NSOST="
#define SOCL "NSOCL=%d"
#define RECVFROM "NSORF=%d,255"
#define GPRS "CGATT?"
#define IMSI "CIMI"
#define IMEI "CGSN=1"
#define REBOOT "NRB"
#define RADIO_ON "CFUN=1"
#define RADIO_OFF "CFUN=0"
#define CONNECT_DATA "CGATT=1"
#define DEFAULT_TIMEOUT 2000

// The default horde type packet
#define MESSAGE_TYPE 1

TelenorNBIoT::TelenorNBIoT(int rx, int tx, uint16_t local_port) : ublox(rx, tx)
{
    m_socket = -1;
    m_imei = 0;
    m_imsi = 0;
    m_local_port = local_port;
}

bool TelenorNBIoT::begin(uint16_t speed)
{
    ublox.begin(speed);
    // Increase timeout for the module. It might be slow to respond on certain
    // commands.
    ublox.setTimeout(DEFAULT_TIMEOUT);
    while (!ublox)
        ;

    return online();
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

bool TelenorNBIoT::connected()
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

unsigned long long TelenorNBIoT::imei()
{
    if (m_imei == 0)
    {
        writeCommand(IMEI);
        if (readCommand(lines) == 2 && isOK(lines[1]))
        {
            // Line 1 contains IMEI ("+CGSN: <15-digit IMEI>")
            m_imei = atoi64(lines[0] + 7);
        }
    }
    return m_imei;
}

unsigned long long TelenorNBIoT::imsi()
{
    if (m_imsi == 0)
    {
        writeCommand(IMSI);
        if (readCommand(lines) == 2 && isOK(lines[1]))
        {
            // Line contains IMSI ("<15 digit IMSI>")
            m_imsi = atoi64(lines[0]);
        }
    }
    return m_imsi;
}

bool TelenorNBIoT::createSocket()
{
    if (m_socket == -1)
    {
        sprintf(buffer, SOCR, m_local_port);
        writeCommand(buffer);
        if (readCommand(lines) == 2 && isOK(lines[1]))
        {
            m_socket = atoi(lines[0]);
            return true;
        }
    }
    return false;
}

bool TelenorNBIoT::closeSocket()
{
    if (m_socket > -1)
    {
        sprintf(buffer, SOCL, m_socket);
        writeCommand(buffer);
        if (readCommand(lines) == 1 && isOK(lines[0]))
        {
            m_socket = -1;
            return true;
        }
    }
    return false;
}

bool TelenorNBIoT::reboot()
{
    m_socket = -1;
    writeCommand(REBOOT);
    delay(2000);
    int ret = readCommand(lines);
    if (ret > 0 && isOK(lines[ret - 1]))
    {
        return true;
    }
    return false;
}

bool TelenorNBIoT::online()
{
    writeCommand(RADIO_ON);
    if (readCommand(lines) == 1 && isOK(lines[0]))
    {
        return dataOn();
    }
    return false;
}

bool TelenorNBIoT::offline()
{
    writeCommand(RADIO_OFF);
    if (readCommand(lines) == 1 && isOK(lines[0]))
    {
        return true;
    }
    return false;
}

void TelenorNBIoT::addHeader()
{
    unsigned long long tmp = m_imei;
    buffer[0] = MESSAGE_TYPE;
    for (int i = 0; i < 7; i++)
    {
        buffer[7 - i] = (char)(tmp & 0xFF);
        tmp >>= 8;
    }
    tmp = m_imsi;
    for (int i = 0; i < 7; i++)
    {
        buffer[14 - i] = (char)(tmp & 0xFF);
        tmp >>= 8;
    }
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
        ublox.write(ch1);
        ublox.write(ch2);
    }
}

bool TelenorNBIoT::sendTo(const char *ip, const uint16_t port, const char *data, const uint16_t length)
{
    ublox.print(PREFIX);
    ublox.print(SOST);
    ublox.print(m_socket);
    ublox.print(",\"");
    ublox.print(ip);
    ublox.print("\",");
    ublox.print(port);
    ublox.print(",");
    ublox.print(length);
    ublox.print(",\"");

    writeBuffer(data, length);

    ublox.print("\"");
    ublox.print(POSTFIX);

    if (readCommand(lines) == 2 && isOK(lines[1]))
    {
        char *fields[10];
        int field = splitFields(lines[0], fields);
        if (field == 1)
        {
            // Found two fields. First is socket no
            uint16_t socketNo = atoi(fields[0]);
            uint16_t bytes = atoi(fields[1]);
            if (socketNo == m_socket && bytes == length)
            {
                return true;
            }
        }
    }
    return false;
}

bool TelenorNBIoT::send(const char *data, const uint16_t length)
{
    // Retrieve IMEI and IMSI if required
    imei();
    imsi();

    addHeader();
    memcpy(buffer + 15, data, length);
    return sendTo(IP, PORT, buffer, length + 15);
}

bool TelenorNBIoT::receiveFrom(char *ip, uint16_t *port, char *outbuf, uint16_t *length, uint16_t *remain)
{
    sprintf(buffer, RECVFROM, m_socket);
    writeCommand(buffer);
    if (readCommand(lines) == 2 && isOK(lines[1]))
    {
        // Data should be <socket>,<ip>,<port>,<length>,<data>,<remaining length>
        char *fields[10];
        int field = splitFields(lines[0], fields);
        if (field == 6)
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
            // TODO(stalehd): Convert data into bytes
            strcpy(outbuf, fields[4]);
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
    int found = 0;
    char *p = line;
    fields[0] = line;
    while (*p++)
    {
        if (*p == ',')
        {
            *p = 0;
            found++;
            fields[found] = p + 1;
        }
    }
    return found;
}

uint8_t TelenorNBIoT::readCommand(char **lines)
{
    uint8_t read = 1;
    uint8_t lineno = 0;
    uint8_t offset = 0;
    bool completed = false;
    ublox.listen();
    while (read > 0 && !completed && lineno < MAXLINES)
    {
        read = ublox.readBytesUntil('\n', (buffer + offset), (BUFSIZE - offset));
        buffer[offset + read] = 0;
        if (read > 0)
        {
            if (strlen(buffer + offset) > 1)
            { // Line will contain a single \r
                lines[lineno++] = (buffer + offset);
            }
            // Exit if line is "OK" - this is the end of the response
            if (isOK(buffer + offset))
            {
                completed = true;
            }
            // ...or if line is "ERROR"
            if (isError(buffer + offset))
            {
                completed = true;
            }
        }
        offset += (read + 1); // +1 is for the 0-terminated char
    }
    return lineno;
}

void TelenorNBIoT::writeCommand(const char *cmd)
{
    uint8_t n = 0;
    ublox.listen();
    while (ublox.available())
    {
        buffer[n++] = ublox.read();
    }

    // TODO(stalehd): Process incoming data (if any)
    ublox.print(PREFIX);
    ublox.print(cmd);
    ublox.print(POSTFIX);
}

unsigned long long TelenorNBIoT::atoi64(const char *input)
{
    unsigned long long ret = 0;
    for (int i = 0; i < strlen(input); i++)
    {
        if (input[i] < '0' || input[i] > '9')
        {
            // ignore non-numeric characters
            continue;
        }
        ret += (unsigned long long)(input[i] - '0');
        ret *= 10;
    }
    return (ret / 10);
}

// Convert unsigned 64 bit int to a string (aka IMSI and IMEI)
void TelenorNBIoT::i64toa(unsigned long long input, char *buffer)
{
    char pos = 0;
    while (input > 0)
    {
        buffer[pos++] = ('0' + (char)(input % 10));
        input /= 10;
    }
    buffer[pos] = 0;
    for (int i = 0; i < pos / 2; i++)
    {
        buffer[i] ^= buffer[pos - 1 - i];
        buffer[pos - 1 - i] ^= buffer[i];
        buffer[i] ^= buffer[pos - 1 - i];
    }
}
