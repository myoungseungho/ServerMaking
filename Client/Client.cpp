#include "Client.h"
#include <iostream>
#include <thread>

void receiveMessages(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    std::string lastMessage = ""; //  마지막으로 받은 메시지 저장

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int receivedBytes = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, NULL, NULL);

        if (receivedBytes > 0) {
            std::string currentMessage(buffer);
            if (currentMessage != lastMessage) { //  동일한 메시지는 무시
                std::cout << "\n [서버 업데이트] " << buffer << std::endl;
                lastMessage = currentMessage;
            }
        }
    }
}

CClient::CClient() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(SERVER_PORT);
}

CClient::~CClient() {
    closesocket(clientSocket);
    WSACleanup();
}

void CClient::start() {
    char buffer[BUFFER_SIZE];

    // 서버로부터 메시지를 받을 수 있도록 수신 스레드 실행
    std::thread recvThread(receiveMessages, clientSocket);
    recvThread.detach();

    while (true) {
        std::cout << "명령어 입력 (MOVE, ATTACK, PICKUP): ";
        std::cin.getline(buffer, BUFFER_SIZE);

        // 서버에 명령어 전송
        sendto(clientSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    }
}
