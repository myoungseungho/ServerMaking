#include "Server.h"
#include <sstream>
#include <thread>   //  sleep_for() 사용을 위한 헤더
#include <chrono>   //  milliseconds 단위 시간 지연을 위한 헤더
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
    ioctlsocket(serverSocket, FIONBIO, &mode); // 논블로킹 모드 설정
}

CServer::~CServer() {
    closesocket(serverSocket);
    WSACleanup();
}

// 클라이언트 정보를 저장하는 함수
void CServer::storeClientInfo(sockaddr_in clientAddr) {
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    std::string clientKey = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));

    if (clientList.find(clientKey) == clientList.end()) {
        int clientID = clientList.size() + 1;  // 순번 할당
        clientList[clientKey] = ClientInfo{ clientAddr, clientID };  //  구조체 명시적 초기화
        std::cout << "새 클라이언트 접속: 클라이언트 " << clientID << " (" << clientKey << ")" << std::endl;
    }
}

// 모든 클라이언트에게 메시지 브로드캐스트 (순번 포함)
void CServer::broadcastMessage(const std::string& message) {  // 수정된 함수 선언
    for (auto& client : clientList) {
        sendto(serverSocket, message.c_str(), message.length() + 1, 0,
            (struct sockaddr*)&client.second.addr, sizeof(client.second.addr));
    }
}

// 서버 실행
void CServer::start() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in clientAddr;

    std::cout << "UDP 서버 실행 중... 포트 " << SERVER_PORT << std::endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int receivedBytes = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrSize);

        if (receivedBytes > 0) {
            storeClientInfo(clientAddr);

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
            std::string clientKey = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));

            std::cout << "클라이언트 [" << clientKey << "] 요청: " << buffer << std::endl;

            // 보낸 클라이언트 정보를 포함하여 모든 클라이언트에게 메시지 전송
            int senderID = clientList[clientKey].id;
            std::ostringstream formattedMessage;
            formattedMessage << "클라이언트 " << senderID << ": " << buffer;
            broadcastMessage(formattedMessage.str());  //  인자 타입 일치
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
