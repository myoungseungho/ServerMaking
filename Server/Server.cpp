#include "Server.h"
#include <thread>

CServer::CServer() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);  // TCP 소켓

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    listen(serverSocket, SOMAXCONN);  //  TCP에서는 listen() 추가 필요

    std::cout << "TCP 서버 실행 중... 포트 " << SERVER_PORT << std::endl;
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
void CServer::storeClientInfo(SOCKET clientSocket) {
    clientSockets.push_back(clientSocket);  // TCP는 소켓을 기반으로 클라이언트 관리
    std::cout << "새 클라이언트 접속! 총 클라이언트 수: " << clientSockets.size() << std::endl;
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

    for (auto& clientSocket : clientSockets) {  // 변경: UDP에서는 clientList, TCP에서는 clientSockets 사용
        int sentBytes = send(clientSocket, finalMessage.c_str(), finalMessage.length() + 1, 0);
        totalBytesSent += (sentBytes > 0) ? sentBytes : 0;
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
    for (auto& client : clientSockets) {
        send(client, message.c_str(), message.length() + 1, 0);
    }
}

void CServer::handleClient(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int receivedBytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);

        if (receivedBytes <= 0) {
            std::cout << "클라이언트 연결 종료" << std::endl;
            closesocket(clientSocket);

            //  연결이 끊긴 클라이언트는 리스트에서 삭제
            clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
            return;
        }

        std::cout << "클라이언트 요청: " << buffer << std::endl;

        // 모든 클라이언트에게 메시지 브로드캐스트
        broadcastMessage(buffer);
    }
}

// 서버 실행 (메인 루프)
void CServer::start() {
    char buffer[BUFFER_SIZE];

    while (true) {
        struct sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "클라이언트 연결 실패" << std::endl;
            continue;
        }

        std::cout << "새로운 클라이언트 연결됨!" << std::endl;

        //  클라이언트 소켓 리스트에 추가
        clientSockets.push_back(clientSocket);

        // 클라이언트와 통신을 위한 스레드 실행
        std::thread clientThread(&CServer::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

