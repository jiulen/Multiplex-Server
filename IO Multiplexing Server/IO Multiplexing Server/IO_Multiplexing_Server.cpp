#include <stdio.h>
#include <string.h>
#include <string>
#include <winsock2.h>
#include <iostream>
#include <ctime> //for timestamps
#include <vector>

#include "packet_manager.h"
#include "room_stuff.h"

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define BUFSIZE 1024
#define PORT_NUMBER 7890

void get_time(int& hours, int& minutes, int& seconds) //return the current time (doesnt include date)
{
    // current date/time based on current system
    time_t rawTimeNow = time(0);
    struct tm formatTimeNow; //struct that holds date and time in a format
    localtime_s(&formatTimeNow, &rawTimeNow);

    hours = formatTimeNow.tm_hour;
    minutes = formatTimeNow.tm_min;
    seconds = formatTimeNow.tm_sec;
}
// Messages sent automatically by the server
void send_welcome_message(SOCKET ClientSocket)
{
    char WelcomeMessage[100];
    sprintf_s(WelcomeMessage, "-Welcome to my I/O multiplexing server! Your Session ID is (%d)-", ClientSocket);

    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket, "MSG", WelcomeMessage);
    //sends current time
    int hours, minutes, seconds;
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket, "STIME", seconds);
    packet_add_data(DataPacket, "MTIME", minutes);
    packet_add_data(DataPacket, "HTIME", hours);
    packet_add_data(DataPacket, "SESID", ClientSocket); //sends session id of the connecting client
    //Encode packet
    char EncodedPacket[BUFSIZE];
    int result = packet_encode(EncodedPacket, "WLCMSG", DataPacket);
    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
}

void session_info_message(fd_set ReadFds, SOCKET ClientSocket)
{
    char infoMessageArray[BUFSIZE];
    std::string infoMessage = "";

    for (int i = 1; i < ReadFds.fd_count; ++i)
    {
        if (ReadFds.fd_array[i] == ClientSocket)
            continue;

        if (infoMessage == "")
            infoMessage += "-Already connected to " + std::to_string(int(ReadFds.fd_count - 2)) + " other client(s) with session ID(s)";

        infoMessage += " (";
        infoMessage += std::to_string(int(ReadFds.fd_array[i]));
        infoMessage += "),";
    }
    if (infoMessage != "")
    {
        infoMessage.pop_back();
        infoMessage += '-';
    }
    else
        infoMessage += "-No other clients are connected-";

    strcpy_s(infoMessageArray, infoMessage.c_str());

    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket, "MSG", infoMessageArray);
    //sends current time
    int hours, minutes, seconds;
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket, "STIME", seconds);
    packet_add_data(DataPacket, "MTIME", minutes);
    packet_add_data(DataPacket, "HTIME", hours);
    packet_add_data(DataPacket, "ROOM", "Lobby"); //Send new room name (send from here cos cant send 3 messages at the start)
    //Encode packet
    char EncodedPacket[BUFSIZE];
    int result = packet_encode(EncodedPacket, "INFOMSG", DataPacket);
    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
}

void send_notice_message(fd_set ReadFds, SOCKET ClientSocket)
{
    char noticeMessageArray[BUFSIZE];
    std::string noticeMessage = "-New client (" + std::to_string(int(ClientSocket)) + ") has connected-";

    strcpy_s(noticeMessageArray, noticeMessage.c_str());

    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket, "MSG", noticeMessageArray);
    //sends current time
    int hours, minutes, seconds;
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket, "STIME", seconds);
    packet_add_data(DataPacket, "MTIME", minutes);
    packet_add_data(DataPacket, "HTIME", hours);
    //Encode packet
    char EncodedPacket[BUFSIZE];
    int result = packet_encode(EncodedPacket, "NOTEMSG", DataPacket);

    for (int i = 1; i < ReadFds.fd_count; ++i)
    {
        if (ReadFds.fd_array[i] == ClientSocket)
            continue;
        send(ReadFds.fd_array[i], EncodedPacket, strlen(EncodedPacket), 0);
    }
}

//Commands
void send_to_all(fd_set ReadFds, char Message[], int MessageLength, SOCKET ClientSocket, bool cmdLong) // Broadcasts message to everybody
{
    int offset = (cmdLong) ? 5 : 3; // offset=5 if long command used, offset=3 if not
    int MessagePos = offset;

    //Create packet
    char DataPacket1[BUFSIZE] = { 0 };
    char EncodedPacket1[BUFSIZE];
    int hours, minutes, seconds;

    for (MessagePos; MessagePos < MessageLength; ++MessagePos)
    {
        if (Message[MessagePos] == ' ') //space (continue looking)
            continue;
        else //found a character that belongs in a message OR \0
            break;
    }

    char messageFound[BUFSIZE] = { 0 };
    if (MessageLength > offset)
        strcpy_s(messageFound, &Message[MessagePos]);

    char broadcastMessageArray[BUFSIZE];

    if (strlen(messageFound) <= 0)
    {
        //No message found
        sprintf_s(broadcastMessageArray, "-Failed to broadcast message, Message not found-");

        //Add data into packet
        packet_add_data(DataPacket1, "MSG", broadcastMessageArray);
        //sends current time
        get_time(hours, minutes, seconds);
        packet_add_data(DataPacket1, "STIME", seconds);
        packet_add_data(DataPacket1, "MTIME", minutes);
        packet_add_data(DataPacket1, "HTIME", hours);
        //Encode packet
        int result = packet_encode(EncodedPacket1, "ERRMSG", DataPacket1);
        send(ClientSocket, EncodedPacket1, strlen(EncodedPacket1), 0);

        return;
    }
    
    //Message found
    sprintf_s(broadcastMessageArray, "Client (%d) broadcasted : ", ClientSocket);
    strcat_s(broadcastMessageArray, messageFound);

    //Add data into packet
    packet_add_data(DataPacket1, "MSG", broadcastMessageArray);
    //sends current time
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket1, "STIME", seconds);
    packet_add_data(DataPacket1, "MTIME", minutes);
    packet_add_data(DataPacket1, "HTIME", hours);
    //Encode packet
    int result1 = packet_encode(EncodedPacket1, "ALLMSG", DataPacket1);

    for (int i = 1; i < ReadFds.fd_count; ++i)
    {
        if (ReadFds.fd_array[i] == ClientSocket)
        {
            std::string successMessage = "-Successfully broadcasted message-";

            char successMessageArray[BUFSIZE];

            strcpy_s(successMessageArray, successMessage.c_str());
            //Create packet
            char DataPacket2[BUFSIZE] = { 0 };
            //Add data into packet
            packet_add_data(DataPacket2, "MSG", successMessageArray);
            //sends current time
            packet_add_data(DataPacket2, "STIME", seconds);
            packet_add_data(DataPacket2, "MTIME", minutes);
            packet_add_data(DataPacket2, "HTIME", hours);
            //Encode packet
            char EncodedPacket2[BUFSIZE];
            int result2 = packet_encode(EncodedPacket2, "SUCMSG", DataPacket2);
            send(ClientSocket, EncodedPacket2, strlen(EncodedPacket2), 0);
        }
        else
            send(ReadFds.fd_array[i], EncodedPacket1, strlen(EncodedPacket1), 0);
    }
}
void send_help_message(SOCKET ClientSocket) // /help - sends list of commands to the user
{
    char HelpMessage[BUFSIZE];
    sprintf_s(HelpMessage, "\n*** How to use this program ***\n"
                           "> /all [Message] - Broadcast message [Message] to all connected clients\n"
                           "> /createroom OR /c [RoomName] [RoomPassword] - Create a room named [RoomName] with password [RoomPassword] and become Room Master\n"
                           "> /deleteroom OR /d [RoomPassword] - Delete current room with password [RoomPassword] (Only for Room Master)\n"
                           "> /inforoom OR /i - Show name, owner and users of current room\n"
                           "> /joinroom OR /j [RoomName] [RoomPassword] - Join a room named [RoomName] with password [RoomPassword]\n"
                           "> /leaveroom OR /l - Leave current room\n"
                           "> /help OR /h - Show this message\n"
                           "> /quit OR /q - Quit and close the connection\n"
                           "> /rooms or /r - Show a list of all rooms\n"
                           "> /users OR /u - Show a list of all connected users\n"
                           "> /whisper OR /w [SessionID] [Message] - Send direct message [Message] to user [SessionID]\n"
                           "> Text not starting with / will be sent to all connected clients in the same room\n"
                           "*** End of Help ***");

    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket, "MSG", HelpMessage);
    //sends current time
    int hours, minutes, seconds;
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket, "STIME", seconds);
    packet_add_data(DataPacket, "MTIME", minutes);
    packet_add_data(DataPacket, "HTIME", hours);
    //Encode packet
    char EncodedPacket[BUFSIZE];
    int result = packet_encode(EncodedPacket, "HELPMSG", DataPacket);
    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
}
void send_client_list(fd_set ReadFds, SOCKET ClientSocket) // /users - sends list of connected clients to the user
{
    char ListMessage[BUFSIZE];
    int numClients = ReadFds.fd_count - 1;
    char ClientID[30];

    if (numClients == 1)
        sprintf_s(ListMessage, "\n*** %d client online ***\n", numClients);
    else
        sprintf_s(ListMessage, "\n***  %d clients online ***\n", numClients);

    for (int i = 1; i < ReadFds.fd_count; ++i) // Get sessionIDs from all clients
    {
        memset(ClientID, '\0', 10);
        if (ReadFds.fd_array[i] == ClientSocket)
            sprintf_s(ClientID, "> (%d) <- You\n", ReadFds.fd_array[i]);
        else
            sprintf_s(ClientID, "> (%d)\n", ReadFds.fd_array[i]);

        strcat_s(ListMessage, ClientID);
    }
    strcat_s(ListMessage, "*** End of Client List ***");

    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket, "MSG", ListMessage);
    //sends current time
    int hours, minutes, seconds;
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket, "STIME", seconds);
    packet_add_data(DataPacket, "MTIME", minutes);
    packet_add_data(DataPacket, "HTIME", hours);
    //Encode packet
    char EncodedPacket[BUFSIZE];
    int result = packet_encode(EncodedPacket, "LISTMSG", DataPacket);
    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
}
void quit_user(fd_set ReadFds, SOCKET ClientSocket) // /quit - quit and close connection from the server for the user
{
    // Message for quitting client
    char QuitMessage1[BUFSIZE];
    sprintf_s(QuitMessage1, "-Successfully quit server-");

    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket, "MSG", QuitMessage1);
    //sends current time
    int hours, minutes, seconds;
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket, "STIME", seconds);
    packet_add_data(DataPacket, "MTIME", minutes);
    packet_add_data(DataPacket, "HTIME", hours);
    //Encode packet
    char EncodedPacket[BUFSIZE];
    int result = packet_encode(EncodedPacket, "QUITMSG", DataPacket);
    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);

    // Message for other clients
    char QuitMessage2[BUFSIZE];
    sprintf_s(QuitMessage2, "-Client (%d) left the server-", ClientSocket);

    //Create packet
    char DataPacket2[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket2, "MSG", QuitMessage2);
    //sends current time
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket2, "STIME", seconds);
    packet_add_data(DataPacket2, "MTIME", minutes);
    packet_add_data(DataPacket2, "HTIME", hours);
    //Encode packet
    char EncodedPacket2[BUFSIZE];
    int result2 = packet_encode(EncodedPacket2, "QUITMSG", DataPacket2);

    // Inform all other clients about quit
    for (int i = 1; i < ReadFds.fd_count; ++i)
    {
        if (ReadFds.fd_array[i] == ClientSocket)
            continue;
        send(ReadFds.fd_array[i], EncodedPacket2, strlen(EncodedPacket2), 0);
    }
}
void whisper_to_one(fd_set ReadFds, char Message[], int MessageLength, SOCKET ClientSocket, bool cmdLong) // /whisper - sends message to one user
{
    char whisperMessageArray[BUFSIZE];
    char TargetSocket[BUFSIZE];
    int MessagePos;

    int offset = (cmdLong) ? 9 : 3; // offset=9 if long command used, offset=3 if not
    int hours, minutes, seconds;

    //Create packet
    char DataPacket[BUFSIZE] = { 0 };

    for (MessagePos = offset; MessagePos < MessageLength; ++MessagePos) //Find target client from message
    {
        if (' ' == Message[MessagePos])
        {
            //Target socket end
            TargetSocket[MessagePos - offset] = '\0';
            break;
        }
        else
        {
            if (isdigit(Message[MessagePos]))
                TargetSocket[MessagePos - offset] = Message[MessagePos];
            else
            {
                if ('\0' == Message[MessagePos])
                {
                    //Target socket end
                    TargetSocket[MessagePos - offset] = '\0';
                    break;
                }
                else
                {
                    std::string errorMessage = "-Failed to send a message, Invalid target client-";

                    strcpy_s(whisperMessageArray, errorMessage.c_str());

                    //Add data into packet
                    packet_add_data(DataPacket, "MSG", whisperMessageArray);
                    //sends current time
                    get_time(hours, minutes, seconds);
                    packet_add_data(DataPacket, "STIME", seconds);
                    packet_add_data(DataPacket, "MTIME", minutes);
                    packet_add_data(DataPacket, "HTIME", hours);
                    //Encode packet
                    char EncodedPacket[BUFSIZE];
                    int result = packet_encode(EncodedPacket, "ONEMSG", DataPacket);
                    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
                    return;
                }
            }
        }            
    }
    if (MessagePos >= MessageLength - 2) //Message/Client not found
    {
        std::string errorMessage = "-Failed to send a message, Target Client/Message not found-";

        strcpy_s(whisperMessageArray, errorMessage.c_str());

        //Add data into packet
        packet_add_data(DataPacket, "MSG", whisperMessageArray);
        //sends current time
        get_time(hours, minutes, seconds);
        packet_add_data(DataPacket, "STIME", seconds);
        packet_add_data(DataPacket, "MTIME", minutes);
        packet_add_data(DataPacket, "HTIME", hours);
        //Encode packet
        char EncodedPacket[BUFSIZE];
        int result = packet_encode(EncodedPacket, "ONEMSG", DataPacket);

        send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
        return;
    }
    for (int i = 1; i < ReadFds.fd_count; ++i)
    {
        if (ReadFds.fd_array[i] == atoi(TargetSocket))
        {
            if (ClientSocket == atoi(TargetSocket))
            {
                std::string errorMessage = "-Failed to send a message, Cannot send message to self-";

                strcpy_s(whisperMessageArray, errorMessage.c_str());
                
                //Add data into packet
                packet_add_data(DataPacket, "MSG", whisperMessageArray);
                //sends current time
                get_time(hours, minutes, seconds);
                packet_add_data(DataPacket, "STIME", seconds);
                packet_add_data(DataPacket, "MTIME", minutes);
                packet_add_data(DataPacket, "HTIME", hours);
                //Encode packet
                char EncodedPacket[BUFSIZE];
                int result = packet_encode(EncodedPacket, "ONEMSG", DataPacket);
                send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);

                return;
            }

            //Whisper message
            std::string whisperMessage = "Client (" + std::to_string(int(ClientSocket)) + ") whispered : ";
            std::string messageString(&Message[MessagePos + 1], MessageLength - MessagePos - 1);
            whisperMessage += messageString;

            strcpy_s(whisperMessageArray, whisperMessage.c_str());
            
            //Add data into packet
            packet_add_data(DataPacket, "MSG", whisperMessageArray);
            //sends current time
            get_time(hours, minutes, seconds);
            packet_add_data(DataPacket, "STIME", seconds);
            packet_add_data(DataPacket, "MTIME", minutes);
            packet_add_data(DataPacket, "HTIME", hours);
            //Encode packet
            char EncodedPacket[BUFSIZE];
            int result = packet_encode(EncodedPacket, "ONEMSG", DataPacket);
            send(ReadFds.fd_array[i], EncodedPacket, strlen(EncodedPacket), 0);

            //Success message
            std::string successMessage = "-Successfully sent message to Client (" + std::to_string(atoi(TargetSocket)) + ")-";

            strcpy_s(whisperMessageArray, successMessage.c_str());

            //Create packet
            char DataPacket2[BUFSIZE] = { 0 };
            //Add data into packet
            packet_add_data(DataPacket2, "MSG", whisperMessageArray);
            //sends current time
            get_time(hours, minutes, seconds);
            packet_add_data(DataPacket2, "STIME", seconds);
            packet_add_data(DataPacket2, "MTIME", minutes);
            packet_add_data(DataPacket2, "HTIME", hours);
            //Encode packet
            char EncodedPacket2[BUFSIZE];
            int result2 = packet_encode(EncodedPacket2, "ONEMSG", DataPacket2);
            send(ClientSocket, EncodedPacket2, strlen(EncodedPacket2), 0);

            break;
        }
    }
}
void send_to_room(std::vector<Room> RoomList, char Message[], int MessageLength, std::string CurrentRoom, SOCKET ClientSocket)
{
    char broadcastMessageArray[BUFSIZE];
    std::string broadcastMessage = "Client (" + std::to_string(int(ClientSocket)) + ") sent : ";
    std::string messageString(Message, MessageLength);
    broadcastMessage += messageString;

    strcpy_s(broadcastMessageArray, broadcastMessage.c_str());
    int broadcastMessageLength = strlen(broadcastMessageArray);
    //Create packet
    char DataPacket1[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket1, "MSG", broadcastMessageArray);
    //sends current time
    int hours, minutes, seconds;
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket1, "STIME", seconds);
    packet_add_data(DataPacket1, "MTIME", minutes);
    packet_add_data(DataPacket1, "HTIME", hours);
    //Encode packet
    char EncodedPacket1[BUFSIZE];
    int result1 = packet_encode(EncodedPacket1, "ROOMMSG", DataPacket1);

    for (int i = 0; i < RoomList.size(); ++i)
    {
        if (RoomList[i].roomName == CurrentRoom) //Find room
        {
            for (int j = 0; j < RoomList[i].roomUsers.size(); ++j) //Find all users
            {
                if (RoomList[i].roomUsers[j] != ClientSocket)
                    send(RoomList[i].roomUsers[j], EncodedPacket1, strlen(EncodedPacket1), 0);
                else
                {
                    std::string successMessage = "-Successfully sent message to room-";

                    char successMessageArray[BUFSIZE];
                    int successMessageLength;

                    strcpy_s(successMessageArray, successMessage.c_str());
                    successMessageLength = strlen(successMessageArray);
                    //Create packet
                    char DataPacket2[BUFSIZE] = { 0 };
                    //Add data into packet
                    packet_add_data(DataPacket2, "MSG", successMessageArray);
                    //sends current time
                    get_time(hours, minutes, seconds);
                    packet_add_data(DataPacket2, "STIME", seconds);
                    packet_add_data(DataPacket2, "MTIME", minutes);
                    packet_add_data(DataPacket2, "HTIME", hours);
                    //Encode packet
                    char EncodedPacket2[BUFSIZE];
                    int result2 = packet_encode(EncodedPacket2, "SUCMSG", DataPacket2);
                    send(ClientSocket, EncodedPacket2, strlen(EncodedPacket2), 0);
                }
            }
        }            
    }
}
void send_invalid_command_error(SOCKET ClientSocket) // Sends a message notifying the user the command is invalid
{
    char InvalidMessage[100];
    sprintf_s(InvalidMessage, "-Invalid command used-");

    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket, "MSG", InvalidMessage);
    //sends current time
    int hours, minutes, seconds;
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket, "STIME", seconds);
    packet_add_data(DataPacket, "MTIME", minutes);
    packet_add_data(DataPacket, "HTIME", hours);
    //Encode packet
    char EncodedPacket[BUFSIZE];
    int result = packet_encode(EncodedPacket, "ERRMSG", DataPacket);
    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
}
// Room functions
void create_room(std::vector<Room>& RoomList, std::string RoomName, std::string RoomPassword, std::string CurrentRoom, SOCKET ClientSocket) //Creates a room with name and password, assigns creator as manager
{
    char CreateRoomMessage[200];
    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Create encoded packet
    char EncodedPacket[BUFSIZE] = { 0 };

    int hours, minutes, seconds;

    for (int i = 0; i < RoomList.size(); ++i)
    {
        if (RoomName == RoomList[i].roomName) //Check if room name taken
        {
            sprintf_s(CreateRoomMessage, "-Failed to create room, Room name (%s) is taken-", RoomName.c_str());

            //Add data into packet
            packet_add_data(DataPacket, "MSG", CreateRoomMessage);
            //sends current time
            get_time(hours, minutes, seconds);
            packet_add_data(DataPacket, "STIME", seconds);
            packet_add_data(DataPacket, "MTIME", minutes);
            packet_add_data(DataPacket, "HTIME", hours);
            //Encode packet            
            int result = packet_encode(EncodedPacket, "CREATEMSG", DataPacket);
            send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);

            return;
        }
    }

    // Check if currently in room
    if (strcmp(CurrentRoom.c_str(), "") != 0) //If in a room, leave room
    {
        bool leftRoom = false;
        for (int i = 0; i < RoomList.size() && !leftRoom; ++i)
        {
            if (CurrentRoom.c_str() == RoomList[i].roomName) //Find current room
            {
                //remove user from room
                RoomList[i].roomUsers.erase(std::remove(RoomList[i].roomUsers.begin(), 
                                                        RoomList[i].roomUsers.end(), 
                                                        ClientSocket), 
                                            RoomList[i].roomUsers.end());

                //Send leave message to others in room
                char LeaveNoticeMessage[200];
                sprintf_s(LeaveNoticeMessage, "-Client (%d) has left room (%s)-", ClientSocket, CurrentRoom.c_str());

                //Create packet
                char DataPacket2[BUFSIZE] = { 0 };
                //Add data into packet
                packet_add_data(DataPacket2, "MSG", LeaveNoticeMessage);
                //sends current time
                get_time(hours, minutes, seconds);
                packet_add_data(DataPacket2, "STIME", seconds);
                packet_add_data(DataPacket2, "MTIME", minutes);
                packet_add_data(DataPacket2, "HTIME", hours);
                //Encode packet
                char EncodedPacket2[BUFSIZE];
                int result2 = packet_encode(EncodedPacket2, "LEAVENOTEMSG", DataPacket2);

                for (int j = 0; j < RoomList[i].roomUsers.size(); ++j)
                    send(RoomList[i].roomUsers[j], EncodedPacket2, strlen(EncodedPacket2), 0);

                leftRoom = true;
            }
        }
    }

    //Make room
    Room newRoom(ClientSocket, RoomName, RoomPassword);
    //Add user into room members
    newRoom.roomUsers.push_back(ClientSocket);
    RoomList.push_back(newRoom);

    if (strcmp(RoomName.c_str(), "Lobby") != 0) //dont send if make lobby
    {
        sprintf_s(CreateRoomMessage, "-Room (%s) created, Joined room (%s)-", RoomName.c_str(), RoomName.c_str());

        //Add data into packet
        packet_add_data(DataPacket, "MSG", CreateRoomMessage);
        //sends current time
        get_time(hours, minutes, seconds);
        packet_add_data(DataPacket, "STIME", seconds);
        packet_add_data(DataPacket, "MTIME", minutes);
        packet_add_data(DataPacket, "HTIME", hours);
        //Send new room name
        packet_add_data(DataPacket, "ROOM", RoomName.c_str());
        //Encode packet            
        int result = packet_encode(EncodedPacket, "CREATEMSG", DataPacket);
        send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
    }
}
void join_room(std::vector<Room>& RoomList, std::string RoomName, std::string RoomPassword, std::string CurrentRoom, SOCKET ClientSocket, bool start)
{
    char JoinRoomMessage[200];
    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Create encoded packet
    char EncodedPacket[BUFSIZE] = { 0 };
    int hours, minutes, seconds;

    //Check if user already in this room
    if (RoomName == CurrentRoom)
    {
        sprintf_s(JoinRoomMessage, "-Already in room (%s)-", RoomName.c_str());

        //Add data into packet
        packet_add_data(DataPacket, "MSG", JoinRoomMessage);
        //sends current time
        get_time(hours, minutes, seconds);
        packet_add_data(DataPacket, "STIME", seconds);
        packet_add_data(DataPacket, "MTIME", minutes);
        packet_add_data(DataPacket, "HTIME", hours);
        //Encode packet            
        int result = packet_encode(EncodedPacket, "JOINMSG", DataPacket);
        send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);

        return;
    }
    for (int i = 0; i < RoomList.size(); ++i)
    {
        if (RoomName == RoomList[i].roomName)
        {
            //Check password
            if (RoomList[i].roomPassword == RoomPassword)
            {                
                // Check if user currently in room
                if (strcmp(CurrentRoom.c_str(), "") != 0) //If user currently in a room, leave room
                {
                    bool leftRoom = false;
                    for (int i = 0; i < RoomList.size() && !leftRoom; ++i)
                    {
                        if (CurrentRoom.c_str() == RoomList[i].roomName) //Find user current room
                        {
                            //remove user from room
                            RoomList[i].roomUsers.erase(std::remove(RoomList[i].roomUsers.begin(),
                                                                    RoomList[i].roomUsers.end(),
                                                                    ClientSocket),
                                                        RoomList[i].roomUsers.end());

                            //Send leave message to others
                            char LeaveNoticeMessage[200];
                            sprintf_s(LeaveNoticeMessage, "-Client (%d) has left room (%s)-", ClientSocket, CurrentRoom.c_str());

                            //Create packet
                            char DataPacket2[BUFSIZE] = { 0 };
                            //Add data into packet
                            packet_add_data(DataPacket2, "MSG", LeaveNoticeMessage);
                            //sends current time
                            get_time(hours, minutes, seconds);
                            packet_add_data(DataPacket2, "STIME", seconds);
                            packet_add_data(DataPacket2, "MTIME", minutes);
                            packet_add_data(DataPacket2, "HTIME", hours);
                            //Encode packet
                            char EncodedPacket2[BUFSIZE];
                            int result2 = packet_encode(EncodedPacket2, "LEAVENOTEMSG", DataPacket2);

                            for (int j = 0; j < RoomList[i].roomUsers.size(); ++j)
                                send(RoomList[i].roomUsers[j], EncodedPacket2, strlen(EncodedPacket2), 0);

                            leftRoom = true;
                        }
                    }
                }

                //Add user into room members vector
                RoomList[i].roomUsers.push_back(ClientSocket);

                if (!start) //dont send message if just join
                {
                    sprintf_s(JoinRoomMessage, "-Joined room (%s)-", RoomName.c_str());
                    //Add data into packet
                    packet_add_data(DataPacket, "MSG", JoinRoomMessage);
                    //sends current time
                    get_time(hours, minutes, seconds);
                    packet_add_data(DataPacket, "STIME", seconds);
                    packet_add_data(DataPacket, "MTIME", minutes);
                    packet_add_data(DataPacket, "HTIME", hours);
                    //Send new room name
                    packet_add_data(DataPacket, "ROOM", RoomName.c_str());
                    //Encode packet            
                    int result = packet_encode(EncodedPacket, "JOINMSG", DataPacket);
                    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);

                    //Inform other users in same room about joining
                    char JoinNoticeMessage[200];
                    sprintf_s(JoinNoticeMessage, "-Client (%d) has joined room (%s)-", ClientSocket, RoomName.c_str());

                    //Create packet
                    char DataPacket2[BUFSIZE] = { 0 };
                    //Add data into packet
                    packet_add_data(DataPacket2, "MSG", JoinNoticeMessage);
                    //sends current time
                    get_time(hours, minutes, seconds);
                    packet_add_data(DataPacket2, "STIME", seconds);
                    packet_add_data(DataPacket2, "MTIME", minutes);
                    packet_add_data(DataPacket2, "HTIME", hours);
                    //Encode packet
                    char EncodedPacket2[BUFSIZE];
                    int result2 = packet_encode(EncodedPacket2, "JOINNOTEMSG", DataPacket2);

                    for (int j = 0; j < RoomList[i].roomUsers.size(); ++j)
                    {
                        if (RoomList[i].roomUsers[j] != ClientSocket)
                            send(RoomList[i].roomUsers[j], EncodedPacket2, strlen(EncodedPacket2), 0);
                    }
                }                
            }
            else
            {
                sprintf_s(JoinRoomMessage, "-Failed to join room (%s), Wrong password given-", RoomName.c_str());

                //Add data into packet
                packet_add_data(DataPacket, "MSG", JoinRoomMessage);
                //sends current time
                get_time(hours, minutes, seconds);
                packet_add_data(DataPacket, "STIME", seconds);
                packet_add_data(DataPacket, "MTIME", minutes);
                packet_add_data(DataPacket, "HTIME", hours);
                //Encode packet            
                int result = packet_encode(EncodedPacket, "JOINMSG", DataPacket);
                send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
            }
            return;
        }
    }

    sprintf_s(JoinRoomMessage, "-Failed to join room (%s), Room not found-", RoomName.c_str()); //Room does not exist

    //Add data into packet
    packet_add_data(DataPacket, "MSG", JoinRoomMessage);
    //sends current time
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket, "STIME", seconds);
    packet_add_data(DataPacket, "MTIME", minutes);
    packet_add_data(DataPacket, "HTIME", hours);
    //Encode packet            
    int result = packet_encode(EncodedPacket, "JOINMSG", DataPacket);
    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
}
void leave_room(std::vector<Room>& RoomList, std::string CurrentRoom, SOCKET ClientSocket) // /leaveroom - leave the current room and return to lobby
{
    char LobbyName[] = "Lobby";
    int hours, minutes, seconds;
    if (strcmp(CurrentRoom.c_str(), LobbyName) != 0) //cant leave room if in lobby already
    {
        for (int i = 0; i < RoomList.size(); ++i)
        {
            if (CurrentRoom.c_str() == RoomList[i].roomName) //Find user current room
            {
                //remove user from room
                RoomList[i].roomUsers.erase(std::remove(RoomList[i].roomUsers.begin(),
                    RoomList[i].roomUsers.end(),
                    ClientSocket),
                    RoomList[i].roomUsers.end());

                //Send leave message to user that left
                char LeaveNoticeMessage[200];
                sprintf_s(LeaveNoticeMessage, "-Left room (%s)-", CurrentRoom.c_str());

                //Create packet
                char DataPacket1[BUFSIZE] = { 0 };
                //Add data into packet
                packet_add_data(DataPacket1, "MSG", LeaveNoticeMessage);
                //sends current time
                get_time(hours, minutes, seconds);
                packet_add_data(DataPacket1, "STIME", seconds);
                packet_add_data(DataPacket1, "MTIME", minutes);
                packet_add_data(DataPacket1, "HTIME", hours);
                //Send new room name
                packet_add_data(DataPacket1, "ROOM", LobbyName);
                //Encode packet
                char EncodedPacket1[BUFSIZE];
                int result1 = packet_encode(EncodedPacket1, "LEAVEMSG", DataPacket1);
                send(ClientSocket, EncodedPacket1, strlen(EncodedPacket1), 0);

                //Send leave message to other users
                memset(LeaveNoticeMessage, '\0', 100);
                sprintf_s(LeaveNoticeMessage, "-Client (%d) has left room (%s)-", ClientSocket, CurrentRoom.c_str());

                //Create packet
                memset(DataPacket1, '\0', BUFSIZE);
                //Add data into packet
                packet_add_data(DataPacket1, "MSG", LeaveNoticeMessage);
                //sends current time
                get_time(hours, minutes, seconds);
                packet_add_data(DataPacket1, "STIME", seconds);
                packet_add_data(DataPacket1, "MTIME", minutes);
                packet_add_data(DataPacket1, "HTIME", hours);
                //Encode packet
                memset(EncodedPacket1, '\0', BUFSIZE);
                result1 = packet_encode(EncodedPacket1, "LEAVENOTEMSG", DataPacket1);

                for (int j = 0; j < RoomList[i].roomUsers.size(); ++j)
                    send(RoomList[i].roomUsers[j], EncodedPacket1, strlen(EncodedPacket1), 0);

                //Inform other users in lobby about joining
                char JoinNoticeMessage[200];
                sprintf_s(JoinNoticeMessage, "-Client (%d) has joined room (%s)-", ClientSocket, LobbyName);

                //Create packet
                memset(DataPacket1, '\0', BUFSIZE);
                //Add data into packet
                packet_add_data(DataPacket1, "MSG", JoinNoticeMessage);
                //sends current time
                get_time(hours, minutes, seconds);
                packet_add_data(DataPacket1, "STIME", seconds);
                packet_add_data(DataPacket1, "MTIME", minutes);
                packet_add_data(DataPacket1, "HTIME", hours);
                //Encode packet
                memset(EncodedPacket1, '\0', BUFSIZE);
                result1 = packet_encode(EncodedPacket1, "JOINNOTEMSG", DataPacket1);

                for (int j = 0; j < RoomList[0].roomUsers.size(); ++j)
                    send(RoomList[0].roomUsers[j], EncodedPacket1, strlen(EncodedPacket1), 0);

                RoomList[0].roomUsers.push_back(ClientSocket); //Lobby is always RoomList[0]

                return;
            }
        }
    }
    else
    {
        //Send error message
        char InvalidMessage[200];
        sprintf_s(InvalidMessage, "-Failed to leave room (%s), (%s) is the main room-", LobbyName, LobbyName);

        //Create packet
        char DataPacket[BUFSIZE] = { 0 };
        //Add data into packet
        packet_add_data(DataPacket, "MSG", InvalidMessage);
        //sends current time
        get_time(hours, minutes, seconds);
        packet_add_data(DataPacket, "STIME", seconds);
        packet_add_data(DataPacket, "MTIME", minutes);
        packet_add_data(DataPacket, "HTIME", hours);
        //Encode packet
        char EncodedPacket[BUFSIZE];
        int result = packet_encode(EncodedPacket, "LEAVEMSG", DataPacket);
        send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
    }
}
void delete_room(std::vector<Room>& RoomList, std::string CurrentRoom, std::string RoomPassword, SOCKET ClientSocket)
{
    char LobbyName[] = "Lobby";
    int hours, minutes, seconds;
    if (strcmp(CurrentRoom.c_str(), LobbyName) != 0) //cant remove lobby room
    {
        for (int i = 0; i < RoomList.size(); ++i)
        {
            if (CurrentRoom.c_str() == RoomList[i].roomName) //Find user current room
            {
                //Check if user is master
                if (RoomList[i].roomMaster == ClientSocket)
                {
                    //Check if password correct
                    if (RoomList[i].roomPassword == RoomPassword)
                    {
                        //Send leave room message
                        char LeaveRoomMessage[200];
                        sprintf_s(LeaveRoomMessage, "-Room (%s) has been deleted, Joining back room (%s)-", CurrentRoom.c_str(), LobbyName);

                        //Create packet
                        char DataPacket[BUFSIZE] = { 0 };
                        //Add data into packet
                        packet_add_data(DataPacket, "MSG", LeaveRoomMessage);
                        //sends current time
                        get_time(hours, minutes, seconds);
                        packet_add_data(DataPacket, "STIME", seconds);
                        packet_add_data(DataPacket, "MTIME", minutes);
                        packet_add_data(DataPacket, "HTIME", hours);
                        //Send new room name
                        packet_add_data(DataPacket, "ROOM", LobbyName);
                        //Encode packet
                        char EncodedPacket[BUFSIZE];
                        memset(EncodedPacket, '\0', BUFSIZE);
                        int result = packet_encode(EncodedPacket, "DELNOTEMSG", DataPacket);

                        for (int j = 0; j < RoomList[i].roomUsers.size(); ++j)
                        {
                            RoomList[0].roomUsers.push_back(RoomList[i].roomUsers[j]); //Lobby is always RoomList[0]

                            send(RoomList[i].roomUsers[j], EncodedPacket, strlen(EncodedPacket), 0);

                            //Send join room message
                            char JoinNoticeMessage[200];
                            sprintf_s(JoinNoticeMessage, "-Client (%d) has joined room (%s)-", ClientSocket, LobbyName);

                            //Create packet
                            char DataPacket2[BUFSIZE] = { 0 };
                            //Add data into packet
                            packet_add_data(DataPacket2, "MSG", JoinNoticeMessage);
                            //sends current time
                            get_time(hours, minutes, seconds);
                            packet_add_data(DataPacket2, "STIME", seconds);
                            packet_add_data(DataPacket2, "MTIME", minutes);
                            packet_add_data(DataPacket2, "HTIME", hours);
                            //Encode packet
                            char EncodedPacket2[BUFSIZE];
                            int result2 = packet_encode(EncodedPacket2, "JOINNOTEMSG", DataPacket2);

                            for (int k = 0; k < RoomList[0].roomUsers.size(); k++)
                            {
                                if (RoomList[0].roomUsers[k] != RoomList[i].roomUsers[j])
                                    send(RoomList[0].roomUsers[k], EncodedPacket2, strlen(EncodedPacket2), 0);
                            }
                        }
                        //Remove room
                        RoomList.erase(RoomList.begin() + i);
                    }
                    else
                    {
                        char InvalidMessage[200];
                        sprintf_s(InvalidMessage, "-Failed to delete room (%s), Wrong password-", LobbyName);

                        //Create packet
                        char DataPacket[BUFSIZE] = { 0 };
                        //Add data into packet
                        packet_add_data(DataPacket, "MSG", InvalidMessage);
                        //sends current time
                        get_time(hours, minutes, seconds);
                        packet_add_data(DataPacket, "STIME", seconds);
                        packet_add_data(DataPacket, "MTIME", minutes);
                        packet_add_data(DataPacket, "HTIME", hours);
                        //Encode packet
                        char EncodedPacket[BUFSIZE];
                        int result = packet_encode(EncodedPacket, "DELMSG", DataPacket);
                        send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
                    }
                }
                else
                {
                    char InvalidMessage[200];
                    sprintf_s(InvalidMessage, "-Failed to delete room (%s), Not master of this room-", LobbyName);

                    //Create packet
                    char DataPacket[BUFSIZE] = { 0 };
                    //Add data into packet
                    packet_add_data(DataPacket, "MSG", InvalidMessage);
                    //sends current time
                    get_time(hours, minutes, seconds);
                    packet_add_data(DataPacket, "STIME", seconds);
                    packet_add_data(DataPacket, "MTIME", minutes);
                    packet_add_data(DataPacket, "HTIME", hours);
                    //Encode packet
                    char EncodedPacket[BUFSIZE];
                    int result = packet_encode(EncodedPacket, "DELMSG", DataPacket);
                    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
                }
                return;
            }
        }
    }
    else
    {
        //Send error message
        char InvalidMessage[200];
        sprintf_s(InvalidMessage, "-Failed to delete room (%s), (%s) is the main room-", LobbyName, LobbyName);

        //Create packet
        char DataPacket[BUFSIZE] = { 0 };
        //Add data into packet
        packet_add_data(DataPacket, "MSG", InvalidMessage);
        //sends current time
        get_time(hours, minutes, seconds);
        packet_add_data(DataPacket, "STIME", seconds);
        packet_add_data(DataPacket, "MTIME", minutes);
        packet_add_data(DataPacket, "HTIME", hours);
        //Encode packet
        char EncodedPacket[BUFSIZE];
        int result = packet_encode(EncodedPacket, "DELMSG", DataPacket);
        send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
    }
}
void show_rooms(std::vector<Room> RoomList, std::string CurrentRoom, SOCKET ClientSocket) // /rooms - show a list of rooms and where user is
{
    char ListMessage[BUFSIZE];
    int numRooms = RoomList.size();
    char RoomName[BUFSIZE];

    if (numRooms == 1)
        sprintf_s(ListMessage, "\n*** %d room is available ***\n", numRooms);
    else
        sprintf_s(ListMessage, "\n*** %d rooms are available ***\n", numRooms);

    // Get roomName from all rooms
    for (int i = 0; i < RoomList.size(); ++i)
    {
        memset(RoomName, '\0', BUFSIZE);
        if (RoomList[i].roomName == CurrentRoom) //Check if player is in the room
            sprintf_s(RoomName, "> (%s) <- You are here\n", RoomList[i].roomName.c_str());
        else
            sprintf_s(RoomName, "> (%s)\n", RoomList[i].roomName.c_str());

        strcat_s(ListMessage, RoomName);
    }
    strcat_s(ListMessage, "*** End of Room List ***");

    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket, "MSG", ListMessage);
    //sends current time
    int hours, minutes, seconds;
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket, "STIME", seconds);
    packet_add_data(DataPacket, "MTIME", minutes);
    packet_add_data(DataPacket, "HTIME", hours);
    //Encode packet
    char EncodedPacket[BUFSIZE];
    int result = packet_encode(EncodedPacket, "LISTMSG", DataPacket);
    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
}
void show_room_info(std::vector<Room> RoomList, std::string CurrentRoom, SOCKET ClientSocket) // /inforoom - show info about room
{
    char RoomInfoMessage[BUFSIZE];
    char RoomUser[BUFSIZE];

    sprintf_s(RoomInfoMessage, "\n*** Room (%s) info ***\n", CurrentRoom.c_str());

    for (int i = 0; i < RoomList.size(); ++i)
    {
        if (RoomList[i].roomName == CurrentRoom)
        {
            if (RoomList[i].roomMaster == ClientSocket) //Check if user is the room master
            {
                memset(RoomUser, '\0', BUFSIZE);
                sprintf_s(RoomUser, "> Room Password : %s\n", RoomList[i].roomPassword.c_str());
                strcat_s(RoomInfoMessage, RoomUser);
            }

            strcat_s(RoomInfoMessage, "| Room Master |\n");

            memset(RoomUser, '\0', BUFSIZE);
            if (RoomList[i].roomMaster == ClientSocket) //Check if user is the room master
                sprintf_s(RoomUser, "> (%d) <- You\n", RoomList[i].roomMaster);
            else
                sprintf_s(RoomUser, "> (%d)\n", RoomList[i].roomMaster);

            strcat_s(RoomInfoMessage, RoomUser);

            memset(RoomUser, '\0', BUFSIZE);
            sprintf_s(RoomUser, "| %d Room Users |\n", RoomList[i].roomUsers.size());
            strcat_s(RoomInfoMessage, RoomUser);

            for (int j = 0; j < RoomList[i].roomUsers.size(); ++j)
            {
                memset(RoomUser, '\0', BUFSIZE);
                if (RoomList[i].roomUsers[j] == ClientSocket) //Check if player is in the room
                    sprintf_s(RoomUser, "> (%d) <- You\n", RoomList[i].roomUsers[j]);
                else
                    sprintf_s(RoomUser, "> (%d)\n", RoomList[i].roomUsers[j]);

                strcat_s(RoomInfoMessage, RoomUser);
            }

            break;
        }
    }

    strcat_s(RoomInfoMessage, "*** End of Room Info ***");

    //Create packet
    char DataPacket[BUFSIZE] = { 0 };
    //Add data into packet
    packet_add_data(DataPacket, "MSG", RoomInfoMessage);
    //sends current time
    int hours, minutes, seconds;
    get_time(hours, minutes, seconds);
    packet_add_data(DataPacket, "STIME", seconds);
    packet_add_data(DataPacket, "MTIME", minutes);
    packet_add_data(DataPacket, "HTIME", hours);
    //Encode packet
    char EncodedPacket[BUFSIZE];
    int result = packet_encode(EncodedPacket, "LISTMSG", DataPacket);
    send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
}

// Room commands (Separating messages into variables here)
void try_create_room(std::vector<Room>& RoomList, std::string CurrentRoom, char Message[], SOCKET ClientSocket) // /createroom - creates a room with name and password
{
    char* Ptr = NULL;
    char* context = NULL;

    char NewRoomName[BUFSIZE] = { 0 };
    char NewRoomPassword[BUFSIZE] = { 0 };
    int MessagePart = 0;

    Ptr = strtok_s(Message, " ", &context); //Separate message using space as delimiter
    while (Ptr != NULL)
    {
        //Get name and password
        if (MessagePart == 1)
            strcpy_s(NewRoomName, Ptr);
        else if (MessagePart == 2)
            strcpy_s(NewRoomPassword, Ptr);

        ++MessagePart;
        Ptr = strtok_s(NULL, " ", &context); //continue to the next part
    }

    //create room if name and password given
    if (strlen(NewRoomName) > 0 && strlen(NewRoomPassword) > 0)
    {
        create_room(RoomList, NewRoomName, NewRoomPassword, CurrentRoom, ClientSocket);
    }
    else
    {
        char InvalidMessage[200];
        sprintf_s(InvalidMessage, "-Failed to create room, Missing room name and/or password-");

        //Create packet
        char DataPacket[BUFSIZE] = { 0 };
        //Add data into packet
        packet_add_data(DataPacket, "MSG", InvalidMessage);
        //sends current time
        int hours, minutes, seconds;
        get_time(hours, minutes, seconds);
        packet_add_data(DataPacket, "STIME", seconds);
        packet_add_data(DataPacket, "MTIME", minutes);
        packet_add_data(DataPacket, "HTIME", hours);
        //Encode packet
        char EncodedPacket[BUFSIZE];
        int result = packet_encode(EncodedPacket, "ERRMSG", DataPacket);
        send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
    }
}
void try_join_room(std::vector<Room>& RoomList, std::string CurrentRoom, char Message[], SOCKET ClientSocket) // /joinroom - join a room with name using a password
{
    char* Ptr = NULL;
    char* context = NULL;

    char JoinRoomName[BUFSIZE] = { 0 };
    char JoinRoomPassword[BUFSIZE] = { 0 };
    int MessagePart = 0;

    Ptr = strtok_s(Message, " ", &context); //Separate message using space as delimiter
    while (Ptr != NULL)
    {
        //Get name and password
        if (MessagePart == 1)
            strcpy_s(JoinRoomName, Ptr);
        else if (MessagePart == 2)
            strcpy_s(JoinRoomPassword, Ptr);

        ++MessagePart;
        Ptr = strtok_s(NULL, " ", &context); //continue to the next part
    }

    //create room if name, password given
    if (strlen(JoinRoomName) > 0 && strlen(JoinRoomPassword) > 0)
    {
        join_room(RoomList, JoinRoomName, JoinRoomPassword, CurrentRoom, ClientSocket, false);
    }
    else
    {
        char InvalidMessage[200];
        sprintf_s(InvalidMessage, "-Failed to join room, Missing room name and/or password-");

        //Create packet
        char DataPacket[BUFSIZE] = { 0 };
        //Add data into packet
        packet_add_data(DataPacket, "MSG", InvalidMessage);
        //sends current time
        int hours, minutes, seconds;
        get_time(hours, minutes, seconds);
        packet_add_data(DataPacket, "STIME", seconds);
        packet_add_data(DataPacket, "MTIME", minutes);
        packet_add_data(DataPacket, "HTIME", hours);
        //Encode packet
        char EncodedPacket[BUFSIZE];
        int result = packet_encode(EncodedPacket, "ERRMSG", DataPacket);
        send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
    }
}
void try_delete_room(std::vector<Room>& RoomList, std::string CurrentRoom, char Message[], SOCKET ClientSocket) // /deleteroom - delete current room using a password
{
    char* Ptr = NULL;
    char* context = NULL;

    char DeleteRoomPassword[BUFSIZE] = { 0 };
    int MessagePart = 0;

    Ptr = strtok_s(Message, " ", &context); //Separate message using space as delimiter
    while (Ptr != NULL)
    {
        //Get password
        if (MessagePart == 1)
            strcpy_s(DeleteRoomPassword, Ptr);

        ++MessagePart;
        Ptr = strtok_s(NULL, " ", &context); //continue to the next part
    }

    //create room if password given
    if (strlen(DeleteRoomPassword) > 0)
        delete_room(RoomList, CurrentRoom, DeleteRoomPassword, ClientSocket);
    else
    {
        char InvalidMessage[200];
        sprintf_s(InvalidMessage, "-Failed to delete room, Missing room password-");

        //Create packet
        char DataPacket[BUFSIZE] = { 0 };
        //Add data into packet
        packet_add_data(DataPacket, "MSG", InvalidMessage);
        //sends current time
        int hours, minutes, seconds;
        get_time(hours, minutes, seconds);
        packet_add_data(DataPacket, "STIME", seconds);
        packet_add_data(DataPacket, "MTIME", minutes);
        packet_add_data(DataPacket, "HTIME", hours);
        //Encode packet
        char EncodedPacket[BUFSIZE];
        int result = packet_encode(EncodedPacket, "ERRMSG", DataPacket);
        send(ClientSocket, EncodedPacket, strlen(EncodedPacket), 0);
    }
}
int main(int argc, char** argv)
{
    int         Port = PORT_NUMBER;
    WSADATA     WsaData;
    SOCKET      ServerSocket;
    SOCKADDR_IN ServerAddr;

    unsigned int Index;
    int         ClientLen = sizeof(SOCKADDR_IN);
    SOCKET      ClientSocket;

    SOCKADDR_IN ClientAddr;
    fd_set      ReadFds, TempFds;
    TIMEVAL     Timeout; // struct timeval timeout;

    char        Message[BUFSIZE];
    int         Return;

    std::vector<Room> roomList;

    if (2 == argc)
    {
        Port = atoi(argv[1]);
    }
    printf("Using port number : [%d]\n", Port);

    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
    {
        printf("WSAStartup() error!\n");
        return 1;
    }

    ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == ServerSocket)
    {
        printf("socket() error\n");
        return 1;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (SOCKET_ERROR == bind(ServerSocket, (SOCKADDR*)&ServerAddr,
        sizeof(ServerAddr)))
    {
        printf("bind() error\n");
        return 1;
    }

    if (SOCKET_ERROR == listen(ServerSocket, 5))
    {
        printf("listen() error\n");
        return 1;
    }

    FD_ZERO(&ReadFds);
    FD_SET(ServerSocket, &ReadFds);

    while (1)
    {
        TempFds = ReadFds;
        Timeout.tv_sec = 5;
        Timeout.tv_usec = 0;
        if (SOCKET_ERROR == (Return = select(0, &TempFds, 0, 0, &Timeout)))
        { // Select() function returned error.
            printf("select() error\n");
            return 1;
        }
        if (0 == Return)
        { // Select() function returned by timeout.
            printf("Select returned timeout.\n");
        }
        else if (0 > Return)
        {
            printf("Select returned error!\n");
        }
        else
        {
            for (Index = 0; Index < TempFds.fd_count; Index++)
            {
                if (TempFds.fd_array[Index] == ServerSocket)
                { // New connection requested by new client.
                    ClientSocket = accept(ServerSocket, (SOCKADDR*)&ClientAddr,
                        &ClientLen);
                    FD_SET(ClientSocket, &ReadFds);
                    printf("New Client Accepted : Socket Handle [%d]\n",
                        ClientSocket);
                             
                    //Check if lobby room created yet (Create/Join lobby room)
                    if (roomList.size() == 0)
                        create_room(roomList, "Lobby", "", "", ClientSocket);
                    else
                        join_room(roomList, "Lobby", "", "", ClientSocket, true);

                    send_welcome_message(ClientSocket);
                    session_info_message(ReadFds, ClientSocket);
                    send_notice_message(ReadFds, ClientSocket);
                }
                else
                { // Something to read from socket.
                    Return = recv(TempFds.fd_array[Index], Message, BUFSIZE, 0);
                    if (0 == Return)
                    { // Connection closed message has arrived.
                        closesocket(TempFds.fd_array[Index]);
                        printf("Connection closed :Socket Handle [%d]\n",
                            TempFds.fd_array[Index]);
                        FD_CLR(TempFds.fd_array[Index], &ReadFds);
                    }
                    else if (0 > Return)
                    { // recv() function returned error.
                        closesocket(TempFds.fd_array[Index]);
                        printf("Exceptional error :Socket Handle [%d]\n",
                            TempFds.fd_array[Index]);
                        FD_CLR(TempFds.fd_array[Index], &ReadFds);
                    }
                    else
                    {
                        //Decode packet
                        char MessageData[BUFSIZE] = { 0 };
                        char CurrentRoomName[BUFSIZE] = { 0 };
                        char PacketID[BUFSIZE] = { 0 };
                        char PacketData[BUFSIZE] = { 0 };
                        int PacketLength = packet_decode(Message, PacketID, PacketData);                        
                        int roomNameSize = packet_parser_data(PacketData, "ROOM", CurrentRoomName);//Find roomName
                        int MessageSize = packet_parser_data(PacketData, "MSG", MessageData);//Find message

                        if ('/' == MessageData[0]) // Command used - Check which command
                        {
                            char* cmdCheck, * cmdLongCheck;
                            int startIndex = 0;
                            bool cmdLong = false;
                            //each command checks for short command then long command
                            //then check if command has nothing directly behind
                            //then check if command is at the start of the message
                            if ((cmdCheck = strstr(MessageData, "/a")) != NULL) //check for /all command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/all")) != NULL)
                                {
                                    if (MessageData[4] == '\0' || MessageData[4] == ' ')
                                        startIndex = cmdLongCheck - MessageData;
                                    else
                                        startIndex = -1;

                                    cmdLong = true;
                                }
                                else
                                {
                                    if (MessageData[2] == '\0' || MessageData[2] == ' ')
                                        startIndex = cmdCheck - MessageData;
                                    else
                                        startIndex = -1;

                                    cmdLong = false;
                                }
                                if (startIndex == 0)
                                    send_to_all(ReadFds, MessageData, MessageSize, TempFds.fd_array[Index], cmdLong);
                            }
                            else if ((cmdCheck = strstr(MessageData, "/h")) != NULL) //check for /help command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/help")) != NULL)
                                {
                                    if (MessageData[5] == '\0' || MessageData[5] == ' ')
                                        startIndex = cmdLongCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                else
                                {
                                    if (MessageData[2] == '\0' || MessageData[2] == ' ')
                                        startIndex = cmdCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                if (startIndex == 0)
                                    send_help_message(TempFds.fd_array[Index]);
                            }
                            else if ((cmdCheck = strstr(MessageData, "/q")) != NULL) //check for /quit command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/quit")) != NULL)
                                {
                                    if (MessageData[5] == '\0' || MessageData[5] == ' ')
                                        startIndex = cmdLongCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                else
                                {
                                    if (MessageData[2] == '\0' || MessageData[2] == ' ')
                                        startIndex = cmdCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                if (startIndex == 0)
                                    quit_user(ReadFds, TempFds.fd_array[Index]);
                            }
                            else if ((cmdCheck = strstr(MessageData, "/u")) != NULL) //check for /list command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/users")) != NULL)
                                {
                                    if (MessageData[6] == '\0' || MessageData[6] == ' ')
                                        startIndex = cmdLongCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                else
                                {
                                    if (MessageData[2] == '\0' || MessageData[2] == ' ')
                                        startIndex = cmdCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                if (startIndex == 0)
                                    send_client_list(ReadFds, TempFds.fd_array[Index]);
                            }
                            else if ((cmdCheck = strstr(MessageData, "/w")) != NULL) //check for /whisper command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/whisper")) != NULL)
                                {
                                    startIndex = cmdLongCheck - MessageData;
                                    cmdLong = true;
                                }
                                else
                                {
                                    startIndex = cmdCheck - MessageData;
                                    cmdLong = false;
                                }
                                if (startIndex == 0)
                                    whisper_to_one(ReadFds, MessageData, MessageSize, TempFds.fd_array[Index], cmdLong);
                            }
                            else if ((cmdCheck = strstr(MessageData, "/c")) != NULL) //check for /createroom command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/createroom")) != NULL)
                                {
                                    if (MessageData[11] == '\0' || MessageData[11] == ' ')
                                        startIndex = cmdLongCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                else
                                {
                                    if (MessageData[2] == '\0' || MessageData[2] == ' ')
                                        startIndex = cmdCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                if (startIndex == 0)
                                    try_create_room(roomList, CurrentRoomName, MessageData, TempFds.fd_array[Index]);
                            }
                            else if ((cmdCheck = strstr(MessageData, "/d")) != NULL) //check for /deleteroom command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/deleteroom")) != NULL)
                                {
                                    if (MessageData[11] == '\0' || MessageData[11] == ' ')
                                        startIndex = cmdLongCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                else
                                {
                                    if (MessageData[2] == '\0' || MessageData[2] == ' ')
                                        startIndex = cmdCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                if (startIndex == 0)
                                    try_delete_room(roomList, CurrentRoomName, MessageData, TempFds.fd_array[Index]);
                            }
                            else if ((cmdCheck = strstr(MessageData, "/i")) != NULL) //check for /inforoom command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/inforoom")) != NULL)
                                {
                                    if (MessageData[9] == '\0' || MessageData[9] == ' ')
                                        startIndex = cmdLongCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                else
                                {
                                    if (MessageData[2] == '\0' || MessageData[2] == ' ')
                                        startIndex = cmdCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                if (startIndex == 0)
                                    show_room_info(roomList, CurrentRoomName, TempFds.fd_array[Index]);
                            }
                            else if ((cmdCheck = strstr(MessageData, "/j")) != NULL) //check for /joinroom command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/joinroom")) != NULL)
                                {
                                    if (MessageData[9] == '\0' || MessageData[9] == ' ')
                                        startIndex = cmdLongCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                else
                                {
                                    if (MessageData[2] == '\0' || MessageData[2] == ' ')
                                        startIndex = cmdCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                if (startIndex == 0)
                                    try_join_room(roomList, CurrentRoomName, MessageData, TempFds.fd_array[Index]);
                            }
                            else if ((cmdCheck = strstr(MessageData, "/l")) != NULL) //check for /leaveroom command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/leaveroom")) != NULL)
                                {
                                    if (MessageData[10] == '\0' || MessageData[10] == ' ')
                                        startIndex = cmdLongCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                else
                                {
                                    if (MessageData[2] == '\0' || MessageData[2] == ' ')
                                        startIndex = cmdCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                if (startIndex == 0)
                                    leave_room(roomList, CurrentRoomName, TempFds.fd_array[Index]);
                            }
                            else if ((cmdCheck = strstr(MessageData, "/r")) != NULL) //check for /rooms command
                            {
                                if ((cmdLongCheck = strstr(MessageData, "/rooms")) != NULL)
                                {
                                    if (MessageData[6] == '\0' || MessageData[6] == ' ')
                                        startIndex = cmdLongCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                else
                                {
                                    if (MessageData[2] == '\0' || MessageData[2] == ' ')
                                        startIndex = cmdCheck - MessageData;
                                    else
                                        startIndex = -1;
                                }
                                if (startIndex == 0)
                                    show_rooms(roomList, CurrentRoomName, TempFds.fd_array[Index]);
                            }
                            else
                                send_invalid_command_error(TempFds.fd_array[Index]);

                            if (startIndex == -1)
                                send_invalid_command_error(TempFds.fd_array[Index]);
                        }
                        else
                        {
                            send_to_room(roomList, MessageData, MessageSize, CurrentRoomName, TempFds.fd_array[Index]);
                        }
                    }
                }
            }
        }
    }

    WSACleanup();

    return 0;
}