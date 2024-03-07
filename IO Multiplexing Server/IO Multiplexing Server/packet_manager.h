#pragma once

// Packet builder
int packet_add_data(char Buffer[], const char DataName[], const int Value);
int packet_add_data(char Buffer[], const char DataName[], const char Value[]);
// Packet parser
int packet_parser_data(const char Packet[], const char DataName[]);
int packet_parser_data(const char Packet[], const char DataName[], char ReturnData[]);
// Packet encoder
int packet_encode(char Packet[], const char PacketID[], const char PacketData[]);
// Packet decoder
int packet_decode(const char Packet[], char PacketID[], char PacketData[]);