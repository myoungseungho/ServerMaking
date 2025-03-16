#include "Server.h"
#include <thread>

CServer::CServer() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    //SOCK_DGRAM이면 UDP, SOCK_STREAM이면 TCP임
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

    if (clientList.find(clientKey) != clientList.end()) {
        playerId = clientList[clientKey].id; //  기존 플레이어 ID 재사용
    }
    else {
        playerId = nextPlayerId++;
        clientList[clientKey] = { clientAddr, playerId }; //  클라이언트 주소 & ID 함께 저장
        std::cout << "새 클라이언트 접속: 플레이어 " << playerId << " (" << clientKey << ")" << std::endl;
    }
}
// 클라이언트 명령어 처리
void CServer::processCommand(const PlayerCommand& command) {
    if (players.find(command.playerId) == players.end()) {
        registerPlayer(command.playerId);
    }

    if (command.action == "MOVE") {
        players[command.playerId].x += 1;
    }
    else if (command.action == "ATTACK") {
        // 공격 로직 (추가 가능)
    }
    else if (command.action == "PICKUP") {
        players[command.playerId].hasItem = true;
    }

    //  여기서 메시지 전송을 제거하고 오직 `broadcastPlayerStates()`에서만 처리
    broadcastPlayerStates();
}


// 모든 클라이언트에게 플레이어 상태 브로드캐스트
void CServer::broadcastPlayerStates() {
    std::ostringstream message;
    message << "[서버 업데이트] ";

    for (auto& player : players) {
        message << "Player " << player.second.id
            << " 위치: (" << player.second.x << ", "
            << player.second.y << ") "
            << "HP: " << player.second.hp << " "
            << (player.second.hasItem ? "[아이템 보유] " : "[아이템 없음] ");
    }

    std::string finalMessage = message.str();
    int totalBytesSent = 0;

    for (auto& client : clientList) {
        sockaddr_in clientAddr = client.second.addr;
        int sentBytes = sendto(serverSocket, finalMessage.c_str(), finalMessage.length() + 1, 0,
            (struct sockaddr*)&clientAddr, sizeof(clientAddr));
        totalBytesSent += sentBytes;
    }

    // 5초마다 전송된 총 데이터량 출력
    static auto lastPrintTime = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPrintTime).count() >= 5) {
        std::cout << "\n===== 서버 네트워크 사용량 =====\n";
        std::cout << "총 데이터 전송량 (5초 기준): " << totalBytesSent / 1024.0 << " KB\n";
        std::cout << "=================================\n";
        lastPrintTime = now;
    }
}

// 메시지를 모든 클라이언트에게 브로드캐스트
void CServer::broadcastMessage(const std::string& message) {
    for (auto& client : clientList) {
        sockaddr_in clientAddr = client.second.addr; //  올바르게 클라이언트 주소 가져오기

        sendto(serverSocket, message.c_str(), message.length() + 1, 0,
            (struct sockaddr*)&clientAddr, sizeof(clientAddr));
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
