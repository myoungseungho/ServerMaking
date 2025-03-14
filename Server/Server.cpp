#include "Server.h"
#include <sstream>
#include <thread>   //  sleep_for() ����� ���� ���
#include <chrono>   //  milliseconds ���� �ð� ������ ���� ���
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
    ioctlsocket(serverSocket, FIONBIO, &mode); // ����ŷ ��� ����
}

CServer::~CServer() {
    closesocket(serverSocket);
    WSACleanup();
}

// Ŭ���̾�Ʈ ������ �����ϴ� �Լ�
void CServer::storeClientInfo(sockaddr_in clientAddr) {
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    std::string clientKey = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));

    if (clientList.find(clientKey) == clientList.end()) {
        int clientID = clientList.size() + 1;  // ���� �Ҵ�
        clientList[clientKey] = ClientInfo{ clientAddr, clientID };  //  ����ü ����� �ʱ�ȭ
        std::cout << "�� Ŭ���̾�Ʈ ����: Ŭ���̾�Ʈ " << clientID << " (" << clientKey << ")" << std::endl;
    }
}

// ��� Ŭ���̾�Ʈ���� �޽��� ��ε�ĳ��Ʈ (���� ����)
void CServer::broadcastMessage(const std::string& message) {  // ������ �Լ� ����
    for (auto& client : clientList) {
        sendto(serverSocket, message.c_str(), message.length() + 1, 0,
            (struct sockaddr*)&client.second.addr, sizeof(client.second.addr));
    }
}

// ���� ����
void CServer::start() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in clientAddr;

    std::cout << "UDP ���� ���� ��... ��Ʈ " << SERVER_PORT << std::endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int receivedBytes = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrSize);

        if (receivedBytes > 0) {
            storeClientInfo(clientAddr);

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
            std::string clientKey = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));

            std::cout << "Ŭ���̾�Ʈ [" << clientKey << "] ��û: " << buffer << std::endl;

            // ���� Ŭ���̾�Ʈ ������ �����Ͽ� ��� Ŭ���̾�Ʈ���� �޽��� ����
            int senderID = clientList[clientKey].id;
            std::ostringstream formattedMessage;
            formattedMessage << "Ŭ���̾�Ʈ " << senderID << ": " << buffer;
            broadcastMessage(formattedMessage.str());  //  ���� Ÿ�� ��ġ
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
