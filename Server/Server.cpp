#include "Server.h"
#include <cstring>

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

void CServer::start() {
    char buffer[BUFFER_SIZE];
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, (sockaddr*)&clientAddr, &addrLen);
        if (bytes > 0) {
            std::cout << "[클라] " << buffer << std::endl;

            if (isNewClient(clientAddr)) {
                clients.push_back(clientAddr);

                int clientNumber = clients.size();
                std::cout << "클라이언트 " << clientNumber << "번 입장" << std::endl;
            }

            broadcast(buffer, clientAddr);
        }
    }
}

void CServer::broadcast(const char* msg, sockaddr_in sender) {
    for (auto& client : clients) {
        if (memcmp(&client, &sender, sizeof(sockaddr_in)) != 0) {
            sendto(serverSocket, msg, strlen(msg) + 1, 0, (sockaddr*)&client, sizeof(client));
        }
    }
}

bool CServer::isNewClient(sockaddr_in& addr) {
    for (auto& c : clients) {
        if (memcmp(&c, &addr, sizeof(sockaddr_in)) == 0)
            return false;
    }
    return true;
}
