#pragma once
#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>
#include <iostream>
#include <ws2tcpip.h>  // ← 추가 (inet_pton() 사용 가능)
#include "vector"
#pragma comment(lib, "ws2_32.lib")

//기본적으로 127.0.0.1을 사용하면 "내 컴퓨터에서 실행되는 서버"에 연결 가능

// 127.0.0.1 (로컬호스트)127.0.0.1은 내 컴퓨터(로컬)에서 실행되는 서버를 의미
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUFFER_SIZE 512

class CClient
{
private:
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;

public:
    CClient();
    ~CClient();
    void start();
};

#endif // CLIENT_H
