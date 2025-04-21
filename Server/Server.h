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
#include <cstdlib> // 패킷 손실 시뮬레이션용

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8888
#define BUFFER_SIZE 1500
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

    // === Debug Options ===
   // 토글 가능 디버깅 요소
    bool debugMessagesPerFrame = false;  // 한 프레임당 메시지 수 측정
    bool debugPacketLoss = true;  // 패킷 손실률 측정
    bool debugInputQueue = false;  // 입력 큐 깊이 및 대기 시간 측정
    bool useInputQueue = false; //  입력큐 사용 여부 토글
    bool useThreadedProcessing = false; // ⛓️ 스레드 분기 여부
    bool simulatePacketLoss = true;     // 🧪 패킷 손실 시뮬레이션 추가

    SOCKET serverSocket;
    sockaddr_in serverAddr;
    std::vector<sockaddr_in> clients;
    std::unordered_map<std::string, int> clientIds;
    std::unordered_map<int, PlayerState> clientStates;
    std::unordered_map<int, int> expectedSeqMap;
    std::unordered_map<int, int> lostPacketMap;
    std::unordered_map<int, std::queue<ClientCommand>> inputQueues;
    std::mutex queueMutex;

    //헤더에 클래스 내부 정의 형태로 두면 컴파일러가 인라인으로 최적화하기 더 쉬움
    //매틱 수백 수천번 호출되는 로직은 호출 오버헤드를 줄이는게 좋다.
    void processCommand(const ClientCommand& msg, int id) {
        auto& st = clientStates[id];
        switch (msg.cmd) {
        case CMD_UP:    st.y += 1; break;
        case CMD_DOWN:  st.y -= 1; break;
        case CMD_LEFT:  st.x -= 1; break;
        case CMD_RIGHT: st.x += 1; break;
        }

        // ⏱️ 부하 추가 (가벼운 연산 반복)
      /*  volatile int dummy = 0;
        for (int i = 0; i < 30000000; ++i) {
            dummy += i % 3;
        }*/
    }

    void consumeInputQueue();

public:
    CServer();
    ~CServer();
    void start();
    int getClientNumber(const sockaddr_in& addr);
    bool isNewClient(const sockaddr_in& addr);
    void broadcastStates();
};

// 콘솔 색상 유틸 함수
inline void SetColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

#endif
