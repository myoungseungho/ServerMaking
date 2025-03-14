#include "Server.h"
#include <thread>

CServer::CServer() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    clientAddrSize = sizeof(sockaddr_in);

    u_long mode = 1;
    ioctlsocket(serverSocket, FIONBIO, &mode); // 논블로킹 모드 설정

    std::cout << "UDP 서버 실행 중... 포트 " << SERVER_PORT << std::endl;
}

CServer::~CServer() {
    closesocket(serverSocket);
    WSACleanup();
}

// 새로운 플레이어 등록
void CServer::registerPlayer(int playerId) {
    players[playerId] = { playerId, 0, 0, 100, 10, false };
    std::cout << "플레이어 " << playerId << " 접속 (초기 상태 등록)" << std::endl;
}

// 클라이언트 정보를 저장하는 함수 (순번 할당)
void CServer::storeClientInfo(sockaddr_in clientAddr, int& playerId) {
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    std::string clientKey = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));

    // 이미 등록된 클라이언트인지 확인
    if (clientList.find(clientKey) != clientList.end()) {
        playerId = clientList[clientKey];  // 기존 플레이어 ID 재사용
    }
    else {
        playerId = nextPlayerId++;  // 새로운 플레이어 ID 할당
        clientList[clientKey] = playerId;  // 클라이언트 등록
        players[playerId] = { playerId, 0, 0, 100, 10, false }; // 초기 상태 설정

        std::cout << "새 클라이언트 접속: 플레이어 " << playerId << " (" << clientKey << ")" << std::endl;
    }
}

// 클라이언트 명령어 처리
void CServer::processCommand(const PlayerCommand& command) {
    if (players.find(command.playerId) == players.end()) {
        registerPlayer(command.playerId);
    }

    std::ostringstream logMessage;

    if (command.action == "MOVE") {
        players[command.playerId].x += 1;
        logMessage << " Player " << command.playerId << " 이동! 위치: ("
            << players[command.playerId].x << ", "
            << players[command.playerId].y << ")";
    }
    else if (command.action == "ATTACK") {
        logMessage << " Player " << command.playerId << " 공격!";
    }
    else if (command.action == "PICKUP") {
        players[command.playerId].hasItem = true;
        logMessage << " Player " << command.playerId << " 아이템 획득!";
    }

    std::string finalMessage = logMessage.str();

    // 행동이 발생했음을 즉시 모든 클라이언트에게 알림
    if (!finalMessage.empty()) {
        broadcastMessage(finalMessage);
    }

    // 모든 플레이어 상태를 동기화
    broadcastPlayerStates();
}

// 모든 클라이언트에게 플레이어 상태 브로드캐스트
void CServer::broadcastPlayerStates() {
    for (auto& player : players) {
        std::ostringstream message;
        message << " Player " << player.second.id
            << " 위치: (" << player.second.x << ", "
            << player.second.y << ") "
            << "HP: " << player.second.hp << " "
            << (player.second.hasItem ? "[아이템 보유]" : "[아이템 없음]");

        // 모든 클라이언트에게 전송
        broadcastMessage(message.str());
    }
}

// 메시지를 모든 클라이언트에게 브로드캐스트
void CServer::broadcastMessage(const std::string& message) {
    for (auto& player : players) {
        sendto(serverSocket, message.c_str(), message.length() + 1, 0,
            (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    }
}

// 서버 실행 (메인 루프)
void CServer::start() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in clientAddr;
    int playerId;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int receivedBytes = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0,
            (struct sockaddr*)&clientAddr, &clientAddrSize);

        if (receivedBytes > 0) {
            storeClientInfo(clientAddr, playerId);

            std::string receivedMessage(buffer);
            std::cout << "플레이어 [" << playerId << "] 요청: " << receivedMessage << std::endl;

            PlayerCommand command = { playerId, receivedMessage };
            processCommand(command); // 플레이어 명령어 처리
            broadcastPlayerStates(); // 모든 클라이언트에게 상태 업데이트
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
