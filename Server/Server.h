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

#define SERVER_PORT 8888  // 사용할 포트 번호
#define BUFFER_SIZE 512   // 버퍼 크기 설정

// 플레이어 상태 구조체
struct PlayerState {
    int id;         // 플레이어 고유 ID
    int x, y;       // 현재 좌표
    int hp;         // 체력
    int attack;     // 공격력
    bool hasItem;   // 아이템 보유 여부
};

// 클라이언트 명령어 구조체
struct PlayerCommand {
    int playerId;       // 어느 플레이어의 입력인가?
    std::string action; // "MOVE", "ATTACK", "PICKUP"
};

class CServer {
private:
    SOCKET serverSocket;
    struct sockaddr_in serverAddr;
    int clientAddrSize;
    int nextPlayerId = 1;  // 플레이어 ID 자동 증가 변수

    std::unordered_map<std::string, int> clientList;  // 클라이언트 목록 (IP:PORT → PlayerID 매핑)
    std::unordered_map<int, PlayerState> players;  // 플레이어 상태 관리

public:
    CServer();
    ~CServer();
    void start();
    void registerPlayer(int playerId); // 새로운 플레이어 등록
    void processCommand(const PlayerCommand& command); // 클라이언트 입력 처리
    void broadcastPlayerStates(); // 모든 클라이언트에게 상태 동기화
    void storeClientInfo(sockaddr_in clientAddr, int& playerId); // 클라이언트 저장
    void broadcastMessage(const std::string& message); // 클라이언트에게 메시지 전송
};

#endif // SERVER_H
