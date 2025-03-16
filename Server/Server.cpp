#include "Server.h"
#include <thread>

CServer::CServer() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    //SOCK_DGRAM�̸� UDP, SOCK_STREAM�̸� TCP��
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    clientAddrSize = sizeof(sockaddr_in);

    u_long mode = 1;
    ioctlsocket(serverSocket, FIONBIO, &mode); // ����ŷ ��� ����

    std::cout << "UDP ���� ���� ��... ��Ʈ " << SERVER_PORT << std::endl;
}

CServer::~CServer() {
    closesocket(serverSocket);
    WSACleanup();
}

// ���ο� �÷��̾� ���
void CServer::registerPlayer(int playerId) {
    players[playerId] = { playerId, 0, 0, 100, 10, false };
    std::cout << "�÷��̾� " << playerId << " ���� (�ʱ� ���� ���)" << std::endl;
}

// Ŭ���̾�Ʈ ������ �����ϴ� �Լ� (���� �Ҵ�)
void CServer::storeClientInfo(sockaddr_in clientAddr, int& playerId) {
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    std::string clientKey = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));

    if (clientList.find(clientKey) != clientList.end()) {
        playerId = clientList[clientKey].id; //  ���� �÷��̾� ID ����
    }
    else {
        playerId = nextPlayerId++;
        clientList[clientKey] = { clientAddr, playerId }; //  Ŭ���̾�Ʈ �ּ� & ID �Բ� ����
        std::cout << "�� Ŭ���̾�Ʈ ����: �÷��̾� " << playerId << " (" << clientKey << ")" << std::endl;
    }
}
// Ŭ���̾�Ʈ ��ɾ� ó��
void CServer::processCommand(const PlayerCommand& command) {
    if (players.find(command.playerId) == players.end()) {
        registerPlayer(command.playerId);
    }

    if (command.action == "MOVE") {
        players[command.playerId].x += 1;
    }
    else if (command.action == "ATTACK") {
        // ���� ���� (�߰� ����)
    }
    else if (command.action == "PICKUP") {
        players[command.playerId].hasItem = true;
    }

    //  ���⼭ �޽��� ������ �����ϰ� ���� `broadcastPlayerStates()`������ ó��
    broadcastPlayerStates();
}


// ��� Ŭ���̾�Ʈ���� �÷��̾� ���� ��ε�ĳ��Ʈ
void CServer::broadcastPlayerStates() {
    std::ostringstream message;
    message << "[���� ������Ʈ] ";

    for (auto& player : players) {
        message << "Player " << player.second.id
            << " ��ġ: (" << player.second.x << ", "
            << player.second.y << ") "
            << "HP: " << player.second.hp << " "
            << (player.second.hasItem ? "[������ ����] " : "[������ ����] ");
    }

    std::string finalMessage = message.str();
    int totalBytesSent = 0;

    for (auto& client : clientList) {
        sockaddr_in clientAddr = client.second.addr;
        int sentBytes = sendto(serverSocket, finalMessage.c_str(), finalMessage.length() + 1, 0,
            (struct sockaddr*)&clientAddr, sizeof(clientAddr));
        totalBytesSent += sentBytes;
    }

    // 5�ʸ��� ���۵� �� �����ͷ� ���
    static auto lastPrintTime = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPrintTime).count() >= 5) {
        std::cout << "\n===== ���� ��Ʈ��ũ ��뷮 =====\n";
        std::cout << "�� ������ ���۷� (5�� ����): " << totalBytesSent / 1024.0 << " KB\n";
        std::cout << "=================================\n";
        lastPrintTime = now;
    }
}

// �޽����� ��� Ŭ���̾�Ʈ���� ��ε�ĳ��Ʈ
void CServer::broadcastMessage(const std::string& message) {
    for (auto& client : clientList) {
        sockaddr_in clientAddr = client.second.addr; //  �ùٸ��� Ŭ���̾�Ʈ �ּ� ��������

        sendto(serverSocket, message.c_str(), message.length() + 1, 0,
            (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    }
}

// ���� ���� (���� ����)
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
            std::cout << "�÷��̾� [" << playerId << "] ��û: " << receivedMessage << std::endl;

            PlayerCommand command = { playerId, receivedMessage };
            processCommand(command); // �÷��̾� ��ɾ� ó��
            broadcastPlayerStates(); // ��� Ŭ���̾�Ʈ���� ���� ������Ʈ
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
