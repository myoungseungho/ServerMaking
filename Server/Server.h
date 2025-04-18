// Server.h
#pragma once
#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8888
#define BUFFER_SIZE 512

struct ClientMessage {
    int clientId;
    int sequenceId;
    float posX;
    float posY;
    long long timestamp;
};

class CServer {
private:
    SOCKET serverSocket;
    sockaddr_in serverAddr;
    std::vector<sockaddr_in> clients;
    std::unordered_map<std::string, int> clientIds;
    std::unordered_map<int, std::pair<float, float>> clientStates;

public:
    CServer();
    ~CServer();
    void start();
    int getClientNumber(const sockaddr_in& addr);
    bool isNewClient(const sockaddr_in& addr);
    void broadcastStates();
};

#endif