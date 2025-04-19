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
#define LOGIC_HZ 60
#define BROADCAST_HZ 20

// 각 클라이언트의 상태를 저장할 구조체 (확장 가능)
struct PlayerState {
    float x = 0.0f;        // 위치 X
    float y = 0.0f;        // 위치 Y
    int health = 100;      // 체력
    int score = 0;         // 점수
};

enum Command { CMD_UP, CMD_DOWN, CMD_LEFT, CMD_RIGHT };

struct ClientCommand {
    int clientId;
    int sequenceId;
    Command cmd;
    long long timestamp;
};

class CServer {
private:
    SOCKET serverSocket;
    sockaddr_in serverAddr;
    std::vector<sockaddr_in> clients;
    std::unordered_map<std::string, int> clientIds;
    std::unordered_map<int, PlayerState> clientStates;

    void processCommand(const ClientCommand& msg, int id) {
        auto& pos = clientStates[id];
        switch (msg.cmd) {
        case CMD_UP:    pos.y += 1.0f; break;
        case CMD_DOWN:  pos.y -= 1.0f; break;
        case CMD_LEFT:  pos.x -= 1.0f; break;
        case CMD_RIGHT: pos.x += 1.0f; break;
        }
    }

public:
    CServer();
    ~CServer();
    void start();
    int getClientNumber(const sockaddr_in& addr);
    bool isNewClient(const sockaddr_in& addr);
    void broadcastStates();
};

#endif
