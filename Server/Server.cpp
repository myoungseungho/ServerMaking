#include "Server.h"
#include <thread>

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

    // �̹� ��ϵ� Ŭ���̾�Ʈ���� Ȯ��
    if (clientList.find(clientKey) != clientList.end()) {
        playerId = clientList[clientKey];  // ���� �÷��̾� ID ����
    }
    else {
        playerId = nextPlayerId++;  // ���ο� �÷��̾� ID �Ҵ�
        clientList[clientKey] = playerId;  // Ŭ���̾�Ʈ ���
        players[playerId] = { playerId, 0, 0, 100, 10, false }; // �ʱ� ���� ����

        std::cout << "�� Ŭ���̾�Ʈ ����: �÷��̾� " << playerId << " (" << clientKey << ")" << std::endl;
    }
}

// Ŭ���̾�Ʈ ��ɾ� ó��
void CServer::processCommand(const PlayerCommand& command) {
    if (players.find(command.playerId) == players.end()) {
        registerPlayer(command.playerId);
    }

    std::ostringstream logMessage;

    if (command.action == "MOVE") {
        players[command.playerId].x += 1;
        logMessage << " Player " << command.playerId << " �̵�! ��ġ: ("
            << players[command.playerId].x << ", "
            << players[command.playerId].y << ")";
    }
    else if (command.action == "ATTACK") {
        logMessage << " Player " << command.playerId << " ����!";
    }
    else if (command.action == "PICKUP") {
        players[command.playerId].hasItem = true;
        logMessage << " Player " << command.playerId << " ������ ȹ��!";
    }

    std::string finalMessage = logMessage.str();

    // �ൿ�� �߻������� ��� ��� Ŭ���̾�Ʈ���� �˸�
    if (!finalMessage.empty()) {
        broadcastMessage(finalMessage);
    }

    // ��� �÷��̾� ���¸� ����ȭ
    broadcastPlayerStates();
}

// ��� Ŭ���̾�Ʈ���� �÷��̾� ���� ��ε�ĳ��Ʈ
void CServer::broadcastPlayerStates() {
    for (auto& player : players) {
        std::ostringstream message;
        message << " Player " << player.second.id
            << " ��ġ: (" << player.second.x << ", "
            << player.second.y << ") "
            << "HP: " << player.second.hp << " "
            << (player.second.hasItem ? "[������ ����]" : "[������ ����]");

        // ��� Ŭ���̾�Ʈ���� ����
        broadcastMessage(message.str());
    }
}

// �޽����� ��� Ŭ���̾�Ʈ���� ��ε�ĳ��Ʈ
void CServer::broadcastMessage(const std::string& message) {
    for (auto& player : players) {
        sendto(serverSocket, message.c_str(), message.length() + 1, 0,
            (struct sockaddr*)&serverAddr, sizeof(serverAddr));
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
