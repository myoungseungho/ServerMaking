#pragma once
#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUFFER_SIZE 512

class CClient {
private:
    SOCKET clientSocket;
    sockaddr_in serverAddr;

public:
    CClient();
    ~CClient();
    void start();
};

#endif
