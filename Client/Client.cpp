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
    // 브로드캐스트 수신 스레드
    std::thread recvThread([this]() {
        char buffer[BUFFER_SIZE];
        sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);
        while (true) {
            int bytes = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0,
                (sockaddr*)&fromAddr, &fromLen);
            if (bytes > 0) {
                std::cout << "[서버 브로드캐스트] " << buffer << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        });
    recvThread.detach();

    while (true) {
        // 모드 토글: 'T' 키
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'T' || ch == 't') {
                autoMode = !autoMode;
                std::cout << (autoMode ? "[Auto mode ON]" : "[Interactive mode ON]") << std::endl;
            }
        }

        if (autoMode) {
            // 자동 모드: 지정 명령 반복 전송
            sendCommand(autoCmd);
            std::this_thread::sleep_for(std::chrono::milliseconds(sendIntervalMs));
        }
        else {
            // 인터랙티브 모드: WASD 키 입력 시 전송
            if (GetAsyncKeyState('W') & 0x8000) sendCommand(CMD_UP);
            if (GetAsyncKeyState('S') & 0x8000) sendCommand(CMD_DOWN);
            if (GetAsyncKeyState('A') & 0x8000) sendCommand(CMD_LEFT);
            if (GetAsyncKeyState('D') & 0x8000) sendCommand(CMD_RIGHT);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}
