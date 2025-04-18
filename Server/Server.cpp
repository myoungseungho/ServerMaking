#include "Server.h"
#include <cstring>
#include <sstream>

CServer::CServer() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    u_long nonBlocking = 1;
    ioctlsocket(serverSocket, FIONBIO, &nonBlocking);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    std::cout << "UDP Dedicated Server 실행 중..." << std::endl;
}

CServer::~CServer() {
    closesocket(serverSocket);
    WSACleanup();
}

int CServer::getClientNumber(const sockaddr_in& addr) {
    char ipStr[INET_ADDRSTRLEN];
    InetNtopA(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr));
    char key[64]; sprintf_s(key, sizeof(key), "%s:%d", ipStr, ntohs(addr.sin_port));
    if (clientIds.find(key) == clientIds.end()) {
        clientIds[key] = (int)clientIds.size() + 1;
        clientStates[clientIds[key]] = { 0,0 };
        std::cout << "클라이언트 " << clientIds[key] << "번 입장" << std::endl;
    }
    return clientIds[key];
}

bool CServer::isNewClient(const sockaddr_in& addr) {
    for (auto& c : clients) if (memcmp(&c, &addr, sizeof(addr)) == 0) return false;
    return true;
}

void CServer::broadcastStates() {
    std::ostringstream oss;
    for (auto& kv : clientStates) {
        oss << kv.first << ":(" << kv.second.first << "," << kv.second.second << ");";
    }
    std::string data = oss.str();
    for (auto& clientAddr : clients) {
        sendto(serverSocket, data.c_str(), data.size() + 1, 0,
            (sockaddr*)&clientAddr, sizeof(clientAddr));
    }
}

void CServer::start() {
    using clock = std::chrono::high_resolution_clock;
    const auto frameDuration = std::chrono::milliseconds(1000 / LOGIC_HZ);
    const auto broadcastInterval = std::chrono::milliseconds(1000 / BROADCAST_HZ);

    auto broadcastTimer = clock::now();

    while (true) {
        auto frameStart = clock::now();
        char buffer[BUFFER_SIZE];
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);

        // 로직 틱 (60Hz)
        while (clock::now() - frameStart < frameDuration) {
            int bytes = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0,
                (sockaddr*)&clientAddr, &addrLen);
            if (bytes <= 0) continue;

            if (isNewClient(clientAddr)) clients.push_back(clientAddr);

            ClientCommand cmd;
            memcpy(&cmd, buffer, sizeof(cmd));
            int id = getClientNumber(clientAddr);
            processCommand(cmd, id);
        }

        // 브로드캐스트 틱 (20Hz)
        auto now = clock::now();
        if (now - broadcastTimer >= broadcastInterval) {
            broadcastStates();
            broadcastTimer = now;
        }
    }
}
