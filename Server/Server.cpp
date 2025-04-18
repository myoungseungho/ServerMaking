#include "Server.h"
#include <cstring>
#include <chrono>

CServer::CServer() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    std::cout << "UDP 서버 실행 중..." << std::endl;
}

CServer::~CServer() {
    closesocket(serverSocket);
    WSACleanup();
}

int CServer::getClientNumber(sockaddr_in& addr) {
    char ipStr[INET_ADDRSTRLEN];
    InetNtopA(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr));
    char key[64];
    sprintf_s(key, sizeof(key), "%s:%d", ipStr, ntohs(addr.sin_port));

    if (clientIds.find(key) == clientIds.end()) {
        clientIds[key] = (int)clientIds.size() + 1;
        std::cout << "클라이언트 " << clientIds[key] << "번 입장" << std::endl;
    }
    return clientIds[key];
}

bool CServer::isNewClient(sockaddr_in& addr) {
    for (auto& c : clients) {
        if (memcmp(&c, &addr, sizeof(sockaddr_in)) == 0)
            return false;
    }
    return true;
}

void CServer::start() {
    using clock = std::chrono::high_resolution_clock;
    const auto frameDuration = std::chrono::milliseconds(1000 / 60);

    while (true) {
        auto frameStart = clock::now();
        std::vector<std::string> frameLogs;

        // 프레임 동안 들어오는 모든 메시지 수신
        while (clock::now() - frameStart < frameDuration) {
            char buffer[BUFFER_SIZE];
            sockaddr_in clientAddr;
            int addrLen = sizeof(clientAddr);

            int bytes = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0,
                (sockaddr*)&clientAddr, &addrLen);
            if (bytes <= 0) continue;

            if (isNewClient(clientAddr)) clients.push_back(clientAddr);

            ClientMessage msg;
            memcpy(&msg, buffer, sizeof(msg));
            int id = getClientNumber(clientAddr);

            char logBuf[128];
            sprintf_s(logBuf, sizeof(logBuf),
                "[서버][Frame] 클라이언트 %d 메시지 수신 → Seq:%d, Pos(%.1f,%.1f)",
                id, msg.sequenceId, msg.posX, msg.posY);

            frameLogs.push_back(logBuf);
        }

        // 프레임 종료 시 한꺼번에 로그 출력
        std::cout << "----- 한 프레임 동안 수신된 메시지들 -----" << std::endl;
        for (auto& line : frameLogs) std::cout << line << std::endl;
        std::cout << "---------------------------------------" << std::endl;
    }
}
