#pragma once
#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <string>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8888  // ����� ��Ʈ ��ȣ
#define BUFFER_SIZE 512   // ���� ũ�� ����

// �÷��̾� ���� ����ü
struct PlayerState {
    int id;         // �÷��̾� ���� ID
    int x, y;       // ���� ��ǥ
    int hp;         // ü��
    int attack;     // ���ݷ�
    bool hasItem;   // ������ ���� ����
};

// Ŭ���̾�Ʈ ��ɾ� ����ü
struct PlayerCommand {
    int playerId;       // ��� �÷��̾��� �Է��ΰ�?
    std::string action; // "MOVE", "ATTACK", "PICKUP"
};

class CServer {
private:
    SOCKET serverSocket;
    struct sockaddr_in serverAddr;
    int clientAddrSize;
    int nextPlayerId = 1;  // �÷��̾� ID �ڵ� ���� ����

    std::unordered_map<std::string, int> clientList;  // Ŭ���̾�Ʈ ��� (IP:PORT �� PlayerID ����)
    std::unordered_map<int, PlayerState> players;  // �÷��̾� ���� ����

public:
    CServer();
    ~CServer();
    void start();
    void registerPlayer(int playerId); // ���ο� �÷��̾� ���
    void processCommand(const PlayerCommand& command); // Ŭ���̾�Ʈ �Է� ó��
    void broadcastPlayerStates(); // ��� Ŭ���̾�Ʈ���� ���� ����ȭ
    void storeClientInfo(sockaddr_in clientAddr, int& playerId); // Ŭ���̾�Ʈ ����
    void broadcastMessage(const std::string& message); // Ŭ���̾�Ʈ���� �޽��� ����
};

#endif // SERVER_H
