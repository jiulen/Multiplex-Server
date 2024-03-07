#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <conio.h>
#include <string>

#include "packet_manager.h"

/// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define BUFSIZE 1024
#define PORT_NUMBER 7890
#define IP_ADDRESS "127.0.0.1"

void get_message_data(char Message[], char MessageData[], char MessageTime[], int& SessionID, char RoomName[]) //for all messages (might have sessionID or roomName)
{
    memset(MessageData, '\0', BUFSIZE);
    memset(MessageTime, '\0', BUFSIZE);

    //Decode packet
    char PacketID[BUFSIZE];
    char PacketData[BUFSIZE];
    int PacketLength = packet_decode(Message, PacketID, PacketData);
    //Find sessionID
    int SessionIDFound = packet_parser_data(PacketData, "SESID");
    if (SessionIDFound != 0)
        SessionID = SessionIDFound;
    //Find roomName
    char RoomNameFound[BUFSIZE];
    int roomNameSize = packet_parser_data(PacketData, "ROOM", RoomNameFound);
    if (roomNameSize != 0)
        strcpy_s(RoomName, BUFSIZE, RoomNameFound);
    //Find message
    int MessageSize = packet_parser_data(PacketData, "MSG", MessageData);
    //Find time
    int hours, minutes, seconds;
    seconds = packet_parser_data(PacketData, "STIME");
    minutes = packet_parser_data(PacketData, "MTIME");
    hours = packet_parser_data(PacketData, "HTIME");
    sprintf_s(MessageTime, BUFSIZE, "[%02d:%02d:%02d]", hours, minutes, seconds);
}

int main(int argc, char** argv)
{
    int Port = PORT_NUMBER;
    char IPAddress[16] = IP_ADDRESS;
    WSADATA WsaData;
    SOCKET ConnectSocket;
    SOCKADDR_IN ServerAddr;

    int ClientLen = sizeof(SOCKADDR_IN);

    fd_set ReadFds, TempFds;
    TIMEVAL Timeout; // struct timeval timeout;

    char Message[BUFSIZE];
    int MessageLen;
    int Return;

    char MessageData[BUFSIZE];
    char MessageTime[BUFSIZE];

    int SessionID = 0;
    char RoomName[BUFSIZE] = "";

    printf("Destination IP Address [%s], Port number [%d]\n", IPAddress, Port);

    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
    {
        printf("WSAStartup() error!");
        return 1;
    }

    ConnectSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == ConnectSocket)
    {
        printf("socket() error");
        return 1;
    }

    ///----------------------
    /// The sockaddr_in structure specifies the address family,
    /// IP address, and port of the server to be connected to.
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    ServerAddr.sin_addr.s_addr = inet_addr(IPAddress);

    ///----------------------
    /// Connect to server.
    Return = connect(ConnectSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
    if (Return == SOCKET_ERROR)
    {
        closesocket(ConnectSocket);
        printf("Unable to connect to server: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    FD_ZERO(&ReadFds);
    FD_SET(ConnectSocket, &ReadFds);

    memset(Message, '\0', BUFSIZE);

    //Welcome message
    recv(ConnectSocket, Message, BUFSIZE, 0);
    get_message_data(Message, MessageData, MessageTime, SessionID, RoomName);
    printf(MessageTime);
    printf(" %s\n", MessageData);
    memset(Message, '\0', BUFSIZE);

    //Info message
    recv(ConnectSocket, Message, BUFSIZE, 0);
    get_message_data(Message, MessageData, MessageTime, SessionID, RoomName);
    printf(MessageTime);
    printf(" %s\n", MessageData);
    memset(Message, '\0', BUFSIZE);

    printf("(%d)> ", SessionID);
    MessageLen = 0;
    while (1)
    {
        if (_kbhit())
        { // To check keyboard input.
            Message[MessageLen] = _getch();
            if (('\n' == Message[MessageLen]) || ('\r' == Message[MessageLen]))
            { // Send the message to server.
                putchar('\n');
                Message[MessageLen] = '\0'; //replace with null char
                MessageLen++;
                //Message[MessageLen] = '\0';

                //Create packet
                char DataPacket[BUFSIZE];
                memset(DataPacket, '\0', BUFSIZE);
                //Add data into packet
                packet_add_data(DataPacket, "MSG", Message);
                //Send current room
                packet_add_data(DataPacket, "ROOM", RoomName);
                //Encode packet
                char EncodedPacket[BUFSIZE];
                int result = packet_encode(EncodedPacket, "CLIMSG", DataPacket);

                //Send packet
                Return = send(ConnectSocket, EncodedPacket, strlen(EncodedPacket), 0);
                if (Return == SOCKET_ERROR)
                {
                    printf("send failed: %d\n", WSAGetLastError());
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }
                printf("Sending Message...");
                MessageLen = 0;
                memset(Message, '\0', BUFSIZE);
            }
            else if (('\b' == Message[MessageLen])) //delete typed stuff
            {
                Message[MessageLen] = '\0';
                if (MessageLen > 0)
                {
                    Message[MessageLen - 1] = '\0';

                    putchar('\b');
                    putchar(' ');
                    putchar('\b');
                    MessageLen--;
                }
            }
            else
            {
                putchar(Message[MessageLen]);
                MessageLen++;
            }
        }
        else
        {
            TempFds = ReadFds;
            Timeout.tv_sec = 0;
            Timeout.tv_usec = 1000;
            if (SOCKET_ERROR == (Return = select(0, &TempFds, 0, 0, &Timeout)))
            { // Select() function returned error.
                closesocket(ConnectSocket);
                printf("select() error\n");
                return 1;
            }
            else if (0 > Return)
            {
                printf("Select returned error!\n");
            }
            else if (0 < Return)
            {
                if (Return/*FD_ISSET(ConnectSocket, &TempFds)*/)
                {
                    memset(Message, '\0', BUFSIZE);
                    //printf("\nSelect Processed... Something to read\n");
                    Return = recv(ConnectSocket, Message, BUFSIZE, 0);
                    if (0 > Return)
                    { // recv() function returned error.
                        closesocket(ConnectSocket);
                        printf("Exceptional error :Socket Handle [%d]\n", ConnectSocket);
                        return 1;
                    }
                    else if (0 == Return)
                    { // Connection closed message has arrived.
                        closesocket(ConnectSocket);
                        printf("Connection closed :Socket Handle [%d]\n", ConnectSocket);
                        return 0;
                    }
                    else
                    {
                        // Message received.
                        //printf("Bytes received : %d\n", Return);
                        printf("\n");
                        get_message_data(Message, MessageData, MessageTime, SessionID, RoomName);
                        printf(MessageTime);
                        printf(" %s\n", MessageData);
                        if (strcmp(MessageData, "-Successfully quit server-") == 0) //Check if message received is to quit
                            break;
                        //If no quit continue normally
                        printf("(%d)> ", SessionID);
                        MessageLen = 0;
                        memset(Message, '\0', BUFSIZE);
                    }
                }
            }
        }
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}