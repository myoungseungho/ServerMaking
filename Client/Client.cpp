// Client.cpp
#include "Client.h"

CClient::CClient() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(SERVER_PORT);

    std::cout << "UDP 클라이언트 시작됨! (T 눌러 모드 토글)" << std::endl;
}

CClient::~CClient() {
    closesocket(clientSocket);
    WSACleanup();
}

void CClient::start() {
    // 수신 처리 스레드
    std::thread recvThread([this]() {
        char buffer[BUFFER_SIZE];
        sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);

        while (true) {
            int bytes = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0,
                reinterpret_cast<sockaddr*>(&fromAddr), &fromLen);

            if (bytes <= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            // 최소한 1바이트는 타입 정보여야 한다
            uint8_t type = buffer[0];

            switch (type) {
            case PKT_ACK: {
                if (bytes < sizeof(AckPacket)) break; // 방어 코드
                AckPacket ack;
                memcpy(&ack, buffer, sizeof(ack));
                std::lock_guard<std::mutex> lock(ackMutex);
                pendingCommands.erase(ack.ackSequenceId);
                break;
            }
            case PKT_NACK: {
                if (bytes < sizeof(NackPacket)) break;
                NackPacket nack;
                memcpy(&nack, buffer, sizeof(nack));
                std::lock_guard<std::mutex> lock(ackMutex);
                auto it = pendingCommands.find(nack.missingSequenceId);
                if (it != pendingCommands.end()) {
                    sendto(clientSocket,
                        reinterpret_cast<char*>(&it->second), sizeof(it->second),
                        0, reinterpret_cast<sockaddr*>(&serverAddr),
                        sizeof(serverAddr));
                }
                break;
            }
            default: {
                // 이건 RTT 디버그 혹은 일반 메시지
                if (debugRTT && bytes == sizeof(ClientCommand)) {
                    ClientCommand echo;
                    memcpy(&echo, buffer, sizeof(echo));
                    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    SetColor(10);
                    std::cout << "[Debug RTT] " << (now - echo.timestamp) << " ms" << std::endl;
                    SetColor(7);
                }
                else {
                    std::cout << "[서버 브로드캐스트] ";
                    SetColor(11);
                    std::cout << buffer << std::endl;
                    SetColor(7);
                }
                break;
            }
            }
        }
        });
    recvThread.detach();

    // 메인 루프: 키 입력 또는 자동 모드
    while (true) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'T' || ch == 't') {
                autoMode = !autoMode;
                std::cout << (autoMode ? "[Auto ON]" : "[Interactive ON]") << std::endl;
            }
        }

        if (autoMode) {
            sendCommand(autoCmd);
            std::this_thread::sleep_for(std::chrono::milliseconds(sendIntervalMs));
        }
        else {
            if (GetAsyncKeyState('W') & 0x8000) sendCommand(CMD_UP);
            if (GetAsyncKeyState('S') & 0x8000) sendCommand(CMD_DOWN);
            if (GetAsyncKeyState('A') & 0x8000) sendCommand(CMD_LEFT);
            if (GetAsyncKeyState('D') & 0x8000) sendCommand(CMD_RIGHT);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}