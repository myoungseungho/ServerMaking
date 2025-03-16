#include "Server.h"
#include <thread>

CServer::CServer() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);  // TCP ����

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    listen(serverSocket, SOMAXCONN);  //  TCP������ listen() �߰� �ʿ�

    std::cout << "TCP ���� ���� ��... ��Ʈ " << SERVER_PORT << std::endl;
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
void CServer::storeClientInfo(SOCKET clientSocket) {
    clientSockets.push_back(clientSocket);  // TCP�� ������ ������� Ŭ���̾�Ʈ ����
    std::cout << "�� Ŭ���̾�Ʈ ����! �� Ŭ���̾�Ʈ ��: " << clientSockets.size() << std::endl;
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

    for (auto& clientSocket : clientSockets) {  // ����: UDP������ clientList, TCP������ clientSockets ���
        int sentBytes = send(clientSocket, finalMessage.c_str(), finalMessage.length() + 1, 0);
        totalBytesSent += (sentBytes > 0) ? sentBytes : 0;
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
    for (auto& client : clientSockets) {
        send(client, message.c_str(), message.length() + 1, 0);
    }
}

void CServer::handleClient(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int receivedBytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);

        if (receivedBytes <= 0) {
            std::cout << "Ŭ���̾�Ʈ ���� ����" << std::endl;
            closesocket(clientSocket);

            //  ������ ���� Ŭ���̾�Ʈ�� ����Ʈ���� ����
            clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
            return;
        }

        std::cout << "Ŭ���̾�Ʈ ��û: " << buffer << std::endl;

        // ��� Ŭ���̾�Ʈ���� �޽��� ��ε�ĳ��Ʈ
        broadcastMessage(buffer);
    }
}

// ���� ���� (���� ����)
void CServer::start() {
    char buffer[BUFFER_SIZE];

    while (true) {
        struct sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Ŭ���̾�Ʈ ���� ����" << std::endl;
            continue;
        }

        std::cout << "���ο� Ŭ���̾�Ʈ �����!" << std::endl;

        //  Ŭ���̾�Ʈ ���� ����Ʈ�� �߰�
        clientSockets.push_back(clientSocket);

        // Ŭ���̾�Ʈ�� ����� ���� ������ ����
        std::thread clientThread(&CServer::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

