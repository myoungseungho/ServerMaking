// Client.cpp
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
    // 수신 스레드: 서버 브로드캐스트 수신 및 파싱
    std::thread recvThread([this]() {
        char buffer[BUFFER_SIZE];
        sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0,
                (sockaddr*)&fromAddr, &fromLen);
            if (bytes <= 0) continue;

            std::string data(buffer);
            std::cout << "[서버 브로드캐스트]: " << data << std::endl;

            // 파싱 예시: "1:(x,y);2:(x,y);"
            size_t start = 0;
            while (start < data.size()) {
                size_t sep = data.find(';', start);
                if (sep == std::string::npos) break;
                std::string entry = data.substr(start, sep - start);
                size_t colon = entry.find(':');
                if (colon != std::string::npos) {
                    int id = std::stoi(entry.substr(0, colon));
                    float x = std::stof(entry.substr(colon + 2, entry.find(',') - (colon + 2)));
                    float y = std::stof(entry.substr(entry.find(',') + 1, entry.find(')') - entry.find(',') - 1));
                    std::cout << "  -> 클라이언트 " << id << " 위치: (" << x << ", " << y << ")" << std::endl;
                }
                start = sep + 1;
            }
        }
        });
    recvThread.detach();

    while (true) {
        ClientMessage msg;
        msg.clientId = 0; // 서버에서 재할당
        msg.sequenceId = sequenceId++;
        msg.posX = posX += 1.0f;
        msg.posY = posY;
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        sendto(clientSocket, (char*)&msg, sizeof(msg), 0,
            (sockaddr*)&serverAddr, sizeof(serverAddr));

        std::this_thread::sleep_for(std::chrono::milliseconds(sendIntervalMs));
    }
}
