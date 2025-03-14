#pragma once
#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8888       //  ����� ��Ʈ ��ȣ
#define BUFFER_SIZE 512        //  ���� ũ�� ����


struct ClientInfo {
    sockaddr_in addr; // Ŭ���̾�Ʈ �ּ� ����
    int id;           // Ŭ���̾�Ʈ ����
};

class CServer {
private:
    SOCKET serverSocket;
    //������ ip�� ��Ʈ ���� ����
    struct sockaddr_in serverAddr;
    int clientAddrSize;
    std::unordered_map<std::string, ClientInfo> clientList;  // Ŭ���̾�Ʈ ���

public:
    CServer();
    ~CServer();
    void start();
    void storeClientInfo(sockaddr_in clientAddr);
    void broadcastMessage(const std::string& message);  // ���� Ÿ�� ����
};

#endif // SERVER_H
