#pragma once
#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUFFER_SIZE 512

struct ClientMessage {
    int clientId;
    int sequenceId;
    float posX;
    float posY;
    long long timestamp;
};

class CClient {
private:
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    int sequenceId = 0;
    float posX = 0.0f;
    float posY = 0.0f;
    int sendIntervalMs = 10; // 전송 주기 (ms)

public:
    CClient();
    ~CClient();
    void start();
};

#endif
