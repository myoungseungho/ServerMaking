﻿#include "Client.h"
#include <iostream>
#include <thread>
#include <numeric>  // std::accumulate 사용을 위한 헤더 추가

using namespace std;

void receiveMessages(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    int receivedPackets = 0;
    int lostPackets = 0;
    vector<long long> rttSamples; // RTT 저장용

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        auto startTime = std::chrono::high_resolution_clock::now(); // 전송 시작 시간 기록

        int receivedBytes = recv(clientSocket, buffer, BUFFER_SIZE, 0); // TCP에서는 recv() 사용
        auto endTime = std::chrono::high_resolution_clock::now(); // 수신 시간 기록

        if (receivedBytes > 0) {
            receivedPackets++;

            // RTT 계산
            long long rtt = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            rttSamples.push_back(rtt);

            //cout << "\n[서버 응답] " << buffer << endl;
        }
        else if (receivedBytes == 0) {
            cout << "[서버 연결 종료]" << endl;
            break;  // 서버가 연결을 종료한 경우 루프 탈출
        }
        else {
            lostPackets++;
        }

        // 10번마다 네트워크 성능 출력
        if (receivedPackets % 10 == 0) {
            long long avgRTT = 0;
            if (!rttSamples.empty()) {
                avgRTT = accumulate(rttSamples.begin(), rttSamples.end(), 0LL) / rttSamples.size();
            }

            cout << "\n===== 네트워크 성능 분석 =====\n";
            cout << "총 패킷 수신: " << receivedPackets << "\n";
            cout << "손실된 패킷 수: " << lostPackets << "\n";
            cout << "패킷 손실률: " << (lostPackets * 100.0 / (receivedPackets + lostPackets)) << "%\n";
            cout << "평균 RTT: " << avgRTT << " ms\n";
            cout << "=================================\n";
        }
    }
}

CClient::CClient() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);  // TCP 소켓 생성

    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(SERVER_PORT);

    // TCP에서는 connect() 호출하여 서버와 연결 필요
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "서버 연결 실패!" << endl;
        closesocket(clientSocket);
        WSACleanup();
        exit(1);
    }
    cout << "서버에 연결되었습니다!" << endl;
}

CClient::~CClient() {
    closesocket(clientSocket);
    WSACleanup();
}

void CClient::start() {
    std::thread recvThread(receiveMessages, clientSocket);
    recvThread.detach();

    while (true) {
        // 초당 10번 "MOVE" 명령을 서버로 전송하여 부하를 발생시킴
        for (int i = 0; i < 50; i++) {
            const char* message = "MOVE";
            send(clientSocket, message, strlen(message) + 1, 0); // TCP에서는 send() 사용
            std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 100ms 마다 전송
        }
    }
}
