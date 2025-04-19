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
        //서버의 정보 ip 포트
        sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);
        //수신 스레드는 계속 true 돌리기
        while (true) {
            int bytes = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0,
                (sockaddr*)&fromAddr, &fromLen);
            if (bytes > 0) {
                std::cout << "[서버 브로드캐스트] " << buffer << std::endl;
            }
            //너무 계속 확인하는것보다 브로드 캐스트 주기가 어차피 20hz니까 좀 쉬어도 됨
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        });

    //수신스레드를 메인루프와 분리해 독립실행 detach 호출 후에 스레드 종료를 join 안해도 
    //메인 스레드 종료돼도 별도로 실행중일 수 있음
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
