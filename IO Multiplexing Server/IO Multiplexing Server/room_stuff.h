#pragma once

#include <vector>
#include <string>

struct Room {
    int roomMaster;
    std::vector<int> roomUsers;

    std::string roomName;
    std::string roomPassword;

    Room(int _roomMaster, std::string _roomName, std::string _roomPassword)
    {
        roomMaster = _roomMaster;
        roomName = _roomName;
        roomPassword = _roomPassword;
    }
};