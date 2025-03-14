#pragma once
#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8888       //  사용할 포트 번호
#define BUFFER_SIZE 512        //  버퍼 크기 설정


struct ClientInfo {
    sockaddr_in addr; // 클라이언트 주소 정보
    int id;           // 클라이언트 순번
};

class CServer {
private:
    SOCKET serverSocket;
    //서버의 ip와 포트 정보 저장
    struct sockaddr_in serverAddr;
    int clientAddrSize;
    std::unordered_map<std::string, ClientInfo> clientList;  // 클라이언트 목록

public:
    CServer();
    ~CServer();
    void start();
    void storeClientInfo(sockaddr_in clientAddr);
    void broadcastMessage(const std::string& message);  // 인자 타입 수정
};

#endif // SERVER_H
