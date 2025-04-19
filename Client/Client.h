// Client.h
#pragma once
#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <conio.h>      // _kbhit, _getch
#include <windows.h>    // GetAsyncKeyState
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUFFER_SIZE 1500

enum Command { CMD_UP, CMD_DOWN, CMD_LEFT, CMD_RIGHT };

struct ClientCommand {
    int clientId;       // 서버 할당
    int sequenceId;     // 시퀀스 추적
    Command cmd;        // 이동 명령
    long long timestamp;// 타임스탬프
    char filler[1024]; //  의미 없는 데이터로 무게 키우기
};

class CClient {
private:
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    int sequenceId = 0;
    bool autoMode = false;
    Command autoCmd = CMD_UP;
    int sendIntervalMs = 1; // 자동 모드 전송 주기
    bool debugRTT = false;

    void sendCommand(Command cmd) {
        ClientCommand msg;
        msg.clientId = 0;
        msg.sequenceId = sequenceId++;
        msg.cmd = cmd;
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        sendto(clientSocket, (char*)&msg, sizeof(msg), 0,
            (sockaddr*)&serverAddr, sizeof(serverAddr));
    }

public:
    CClient();
    ~CClient();
    void start();
};

// 콘솔 색상 유틸 함수
inline void SetColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

#endif
