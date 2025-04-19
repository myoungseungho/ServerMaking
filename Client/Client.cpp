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
    std::thread recvThread([this]() {
        char buffer[BUFFER_SIZE];
        sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);
        while (true) {
            int bytes = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0,
                reinterpret_cast<sockaddr*>(&fromAddr), &fromLen);
            if (bytes > 0) {
                // RTT 에코 응답 처리
                if (debugRTT && bytes == sizeof(ClientCommand)) {
                    ClientCommand echo;
                    memcpy(&echo, buffer, sizeof(echo));
                    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    std::cout << "[Debug RTT] " << (now - echo.timestamp) << " ms" << std::endl;
                }
                else {
                    std::cout << "[서버 브로드캐스트] " << buffer << std::endl;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        });
    recvThread.detach();

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