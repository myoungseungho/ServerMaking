#include "Client.h"
#include <chrono>
#include <string>
#include <thread>

CClient::CClient() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(SERVER_PORT);

    std::cout << "UDP 클라이언트 시작됨!" << std::endl;
}

CClient::~CClient() {
    closesocket(clientSocket);
    WSACleanup();
}

void CClient::start() {
    std::thread recvThread([this]() {
        char buffer[BUFFER_SIZE];
        sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, (sockaddr*)&fromAddr, &fromLen);
            if (bytes > 0)
                std::cout << "[서버]: " << buffer << std::endl;
        }
        });
    recvThread.detach();

    // ✅ 여기에 JOIN 전송
    std::string join = "JOIN";
    int sent = sendto(clientSocket,
        join.c_str(),
        (int)join.size() + 1,
        0,
        (sockaddr*)&serverAddr,
        sizeof(serverAddr));
    std::cout << "[클라] JOIN 전송: " << (sent > 0 ? "성공" : "실패(" + std::to_string(WSAGetLastError()) + ")") << std::endl;

}