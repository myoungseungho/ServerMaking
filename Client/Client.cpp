#include "Client.h"
#include <thread>
#include <cstring>

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

    // 1초 간격으로 메시지 전송
    while (true) {
        ClientMessage msg;
        msg.clientId = 0; // 일단 0으로 고정 (서버에서 처리)
        msg.sequenceId = sequenceId++;
        msg.posX = posX += 1.0f;
        msg.posY = posY;
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        sendto(clientSocket, (char*)&msg, sizeof(msg), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

        std::this_thread::sleep_for(std::chrono::milliseconds(sendIntervalMs));
    }
}