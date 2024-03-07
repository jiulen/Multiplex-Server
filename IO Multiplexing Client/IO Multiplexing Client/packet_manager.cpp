#include "packet_manager.h"
#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>

#define BUFSIZE 1024

// Packet builder
int packet_add_data(char Buffer[], const char DataName[], const int Value)
{
    if (Buffer[0] == '\0')
        sprintf_s(Buffer, BUFSIZE, "%s%s=%d", Buffer, DataName, Value);
    else
        sprintf_s(Buffer, BUFSIZE, "%s %s=%d", Buffer, DataName, Value);

    return strlen(Buffer);
}
int packet_add_data(char Buffer[], const char DataName[], const char Value[])
{
    if (Buffer[0] == '\0')
        sprintf_s(Buffer, BUFSIZE, "%s%s=\"%s\"", Buffer, DataName, Value);
    else
        sprintf_s(Buffer, BUFSIZE, "%s %s=\"%s\"", Buffer, DataName, Value);
    return strlen(Buffer);
}
// Packet parser
int packet_parser_data(const char Packet[], const char DataName[])
{
    const char* String;
    if (NULL == (String = strstr(Packet, DataName)))
    {
        //Data name not found
        return 0;
    }

    int StringLen = strlen(String);

    for (int i = 0; i < StringLen; ++i)
    {
        if ('=' == String[i])
        {
            std::string dataValue;
            for (int j = i + 1; j < StringLen; ++j)
            {
                if (' ' == String[j] || '\n' == String[j] || '\0' == String[j] || '\r' == String[j])
                    break;
                dataValue.push_back(String[j]);
            }
            return atoi(dataValue.c_str());
        }
    }

    return 0;
}
int packet_parser_data(const char Packet[], const char DataName[], char ReturnData[])
{
    const char* String;
    if (NULL == (String = strstr(Packet, DataName)))
    {
        //Data name not found
        return 0;
    }

    int StringLen = strlen(String);

    for (int i = 0; i < StringLen; ++i)
    {
        if ('=' == String[i] && '\"' == String[++i]) //Make sure its a string
        {
            std::string dataValue;
            for (int j = i + 1; j < StringLen; ++j)
            {
                if ('\"' == String[j])
                    break;
                dataValue.push_back(String[j]);
            }
            strcpy_s(ReturnData, BUFSIZE, dataValue.c_str());
            return strlen(ReturnData);
        }
    }

    return 0;
}
// Packet encoder
int packet_encode(char Packet[], const char PacketID[], const char PacketData[])
{
    int TotalPacketLength = strlen(PacketID) + strlen(PacketData) + 7; // +2 for <>, +2 for space, +3 for PacketLength 

    sprintf_s(Packet, BUFSIZE, "<%s %03d %s>", PacketID, strlen(PacketData), PacketData);

    return TotalPacketLength;
}
// Packet decoder
int packet_decode(const char Packet[], char PacketID[], char PacketData[])
{
    int PacketDataLength;
    int PacketLength = strlen(Packet);
    int Pos = 1;

    for (int j = 0; Pos < PacketLength; ++Pos)
    {
        if (' ' == Packet[Pos])
        {
            PacketID[j] = '\0';
            ++Pos;
            break;
        }
        PacketID[j++] = Packet[Pos];
    }

    Pos += 4;
    strcpy_s(PacketData, BUFSIZE, &Packet[Pos]);
    PacketDataLength = strlen(PacketData);

    return PacketDataLength;
}