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
#include <queue>
#include <thread>
#include <mutex>
#include <conio.h>
#include <cstdlib>      // rand(), srand()
#include <ctime>        // time()

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT          8888
#define BUFFER_SIZE          1500
#define LOGIC_HZ             60
#define BROADCAST_HZ         20
#define PACKET_LOSS_PERCENT  10     // 손실 시뮬레이션 비율 (%)

// 각 클라이언트 상태
struct PlayerState {
    float x = 0.0f;
    float y = 0.0f;
    int health = 100;
    int score = 0;
};

enum Command { CMD_UP, CMD_DOWN, CMD_LEFT, CMD_RIGHT };

// 수신된 클라이언트 명령 구조
struct ClientCommand {
    int clientId;
    int sequenceId;
    Command cmd;
    long long timestamp;
};

// ACK/NACK 패킷 구조
struct AckPacket { int clientId; int ackSequenceId; };
struct NackPacket { int clientId; int missingSequenceId; };

class CServer {
private:
    SOCKET serverSocket;
    sockaddr_in serverAddr;
    std::vector<sockaddr_in> clients;
    std::unordered_map<std::string, int> clientIds;
    std::unordered_map<int, PlayerState> clientStates;
    std::unordered_map<int, int> expectedSeqMap;
    std::unordered_map<int, int> lostPacketMap;
    std::unordered_map<int, std::queue<ClientCommand>> inputQueues;
    std::mutex queueMutex;

    // 옵션 및 디버깅 플래그
    bool simulatePacketLoss = true;   // rand()로 일부 패킷 드롭
    bool debugPacketLoss = true;  // 드롭 로그 및 손실률 출력
    bool enableAckNack = false; // ACK/NACK 기능 토글
    bool useInputQueue = false;
    bool useThreadedProc = false;

    void processCommand(const ClientCommand& msg, int id);
    void consumeInputQueue();
    void sendAck(const sockaddr_in& cl, int clientId, int seq);
    void sendNack(const sockaddr_in& cl, int clientId, int missingSeq);

public:
    CServer();
    ~CServer();
    void start();
    int getClientNumber(const sockaddr_in& addr);
    bool isNewClient(const sockaddr_in& addr);
    void broadcastStates();
};

// 콘솔 색상 유틸
inline void SetColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

#endif