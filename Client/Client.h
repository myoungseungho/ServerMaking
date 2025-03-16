#pragma once
#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>
#include <iostream>
#include <ws2tcpip.h>  // �� �߰� (inet_pton() ��� ����)
#include "vector"
#pragma comment(lib, "ws2_32.lib")

//�⺻������ 127.0.0.1�� ����ϸ� "�� ��ǻ�Ϳ��� ����Ǵ� ����"�� ���� ����

// 127.0.0.1 (����ȣ��Ʈ)127.0.0.1�� �� ��ǻ��(����)���� ����Ǵ� ������ �ǹ�
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
