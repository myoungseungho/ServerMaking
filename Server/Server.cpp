// Server.cpp
#include "Server.h"
#include <sstream>

CServer::CServer() {
    srand((unsigned)time(nullptr));
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    u_long nonBlock = 1;
    ioctlsocket(serverSocket, FIONBIO, &nonBlock);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::cout << "UDP Dedicated Server 실행 중..." << std::endl;
    std::cout << "[Info] ACK/NACK 기능: " << (enableAckNack ? "ON" : "OFF") << std::endl;
}

CServer::~CServer() {
    closesocket(serverSocket);
    WSACleanup();
}

int CServer::getClientNumber(const sockaddr_in& addr) {
    char key[64];
    InetNtopA(AF_INET, &addr.sin_addr, key, sizeof(key));
    sprintf_s(key + strlen(key), sizeof(key) - strlen(key), ":%d", ntohs(addr.sin_port));
    if (clientIds.find(key) == clientIds.end()) {
        int newId = (int)clientIds.size() + 1;
        clientIds[key] = newId;
        clientStates[newId] = PlayerState();
        std::cout << "클라이언트 " << newId << "번 입장" << std::endl;
    }
    return clientIds[key];
}

bool CServer::isNewClient(const sockaddr_in& addr) {
    for (auto& c : clients) {
        if (memcmp(&c, &addr, sizeof(addr)) == 0) return false;
    }
    return true;
}



void CServer::sendAck(const sockaddr_in& cl, int clientId, int seq) {
    // ACK/NACK 기능이 꺼져 있으면 바로 리턴
    if (!enableAckNack) {
        if (debugPacketLoss) {
            SetColor(12);
            std::cout << "[Debug] ACK/NACK disabled: skip ACK seq=" << seq << std::endl;
            SetColor(7);
        }
        return;
    }

    // 👈 여기로 이동: 전송 로그보다 먼저 '손실 시뮬레이션 드롭'을 판단
    if (simulatePacketLoss && (rand() % 100) < PACKET_LOSS_PERCENT) {
        if (debugPacketLoss) {
            SetColor(12);
            std::cout << "[Debug] Dropped ACK client=" << clientId
                << " seq=" << seq << std::endl;
            SetColor(7);
        }
        return;
    }

    // 이제 실제로 '보내려 한다'는 로그를 찍습니다
    if (debugPacketLoss) {
        SetColor(10);
        std::cout << "[Debug] Sending ACK client=" << clientId
            << " seq=" << seq << std::endl;
        SetColor(7);
    }

    // 실제 ACK 전송
    AckPacket ack{ PKT_ACK, clientId, seq };
    sendto(serverSocket,
        reinterpret_cast<char*>(&ack), sizeof(ack),
        0, reinterpret_cast<const sockaddr*>(&cl), sizeof(cl));

    // 성공적으로 ACK이 전송되었을 때만 손실 카운트를 줄입니다
    if (lostPacketMap[clientId] > 0) {
        --lostPacketMap[clientId];
        if (debugPacketLoss) {
            SetColor(9);
            std::cout << "[Debug] Recovered packet via ACK seq=" << seq
                << " for client=" << clientId
                << ", lost now " << lostPacketMap[clientId] << std::endl;
            SetColor(7);
        }
    }
}




void CServer::sendNack(const sockaddr_in& cl, int clientId, int missingSeq) {
    if (!enableAckNack) {
        if (debugPacketLoss) {
            SetColor(12);
            std::cout << "[Debug] ACK/NACK disabled: skip NACK seq=" << missingSeq << std::endl;
            SetColor(7);
        }
        return;
    }
    if (debugPacketLoss) {
        SetColor(10);
        std::cout << "[Debug] Sending NACK client=" << clientId << " seq=" << missingSeq << std::endl;
        SetColor(7);
    }
    if (simulatePacketLoss && (rand() % 100) < PACKET_LOSS_PERCENT) {
        if (debugPacketLoss) {
            SetColor(12);
            std::cout << "[Debug] Dropped NACK client=" << clientId << " seq=" << missingSeq << std::endl;
            SetColor(7);
        }
        return;
    }

    NackPacket nack{ PKT_NACK, clientId, missingSeq };
    sendto(serverSocket,
        reinterpret_cast<char*>(&nack), sizeof(nack),
        0, reinterpret_cast<const sockaddr*>(&cl), sizeof(cl));
}

void CServer::broadcastStates() {
    std::ostringstream oss;
    for (auto& kv : clientStates) {
        auto& s = kv.second;
        oss << kv.first << ":(" << s.x << "," << s.y << ")"
            << ",HP=" << s.health << ",Score=" << s.score << ";";
    }
    std::string data = oss.str();
    for (auto& cl : clients) {
        if (simulatePacketLoss && (rand() % 100) < PACKET_LOSS_PERCENT) {
            if (debugPacketLoss) {
                SetColor(12);
                std::cout << "[Debug] Dropped broadcast packet" << std::endl;
                SetColor(7);
            }
            continue;
        }
        sendto(serverSocket,
            data.c_str(), (int)data.size() + 1,
            0, reinterpret_cast<const sockaddr*>(&cl), sizeof(cl));
    }
}

void CServer::start() {
    using clock = std::chrono::high_resolution_clock;
    auto frameDur = std::chrono::milliseconds(1000 / LOGIC_HZ);
    auto bcInt = std::chrono::milliseconds(1000 / BROADCAST_HZ);
    auto nextBC = clock::now();

    if (useThreadedProc)
        std::thread(&CServer::consumeInputQueue, this).detach();

    while (true) {
        // (1) 콘솔 입력 토글 처리
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'A' || ch == 'a') {
                enableAckNack = !enableAckNack;
                std::cout << "[Info] ACK/NACK 기능: "
                    << (enableAckNack ? "ON" : "OFF") << std::endl;
            }
            else if (ch == 'I' || ch == 'i') {
                useInputQueue = !useInputQueue;
            }
        }

        // (2) 한 프레임 동안 패킷 수신/버퍼링/처리
        auto startTime = clock::now();
        char buf[BUFFER_SIZE];
        sockaddr_in cl;
        int len = sizeof(cl);

        while (clock::now() - startTime < frameDur) {
            int bytes = recvfrom(serverSocket, buf, BUFFER_SIZE, 0,
                reinterpret_cast<sockaddr*>(&cl), &len);
            if (bytes <= 0)
                continue;

            // (2.1) 패킷 손실 시뮬레이션
            if (simulatePacketLoss && (rand() % 100) < PACKET_LOSS_PERCENT) {
                if (debugPacketLoss) {
                    SetColor(12);
                    std::cout << "[Debug] Dropped receive packet" << std::endl;
                    SetColor(7);
                }
                continue;
            }

            // (2.2) 신규 클라이언트 등록
            if (isNewClient(cl))
                clients.push_back(cl);

            // (2.3) 수신된 데이터 파싱
            ClientCommand cmd;
            memcpy(&cmd, buf, sizeof(cmd));
            int id = getClientNumber(cl);

            // (2.4) ACK는 받은 모든 패킷에 대해 즉시!
            sendAck(cl, id, cmd.sequenceId);

            // (2.5) 누락 감지 & NACK 요청
            int& expect = expectedSeqMap[id];
            // 처음 생긴 키는 0 → 1로 초기화
            if (expect == 0) expect = 1;

            if (enableAckNack && cmd.sequenceId > expect) {
                for (int miss = expect; miss < cmd.sequenceId; ++miss) {
                    if (!bufferedCommands[id].count(miss)) {
                        if (debugPacketLoss) {
                            SetColor(12);
                            std::cout << "[Debug] Detected missing packet "
                                << miss << " for client " << id << std::endl;
                            SetColor(7);
                        }
                        sendNack(cl, id, miss);
                        ++lostPacketMap[id];
                    }
                }
            }

            // (2.6) **순서 보정용 버퍼링 & 처리** 
            if (cmd.sequenceId < expect) {
                // 이미 처리된 과거 패킷: 무시
            }
            else if (cmd.sequenceId == expect) {
                // 딱 기대한 번호: 즉시 처리
                if (useInputQueue) {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    inputQueues[id].push(cmd);
                }
                else {
                    processCommand(cmd, id);
                }
                ++expect;

                // 버퍼에 쌓인 나머지 연속 번호도 처리
                auto& bufMap = bufferedCommands[id];
                while (bufMap.count(expect)) {
                    const ClientCommand& nextCmd = bufMap[expect];
                    if (useInputQueue) {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        inputQueues[id].push(nextCmd);
                    }
                    else {
                        processCommand(nextCmd, id);
                    }
                    bufMap.erase(expect);
                    ++expect;
                }
            }
            else {
                // 미래 패킷: 버퍼에 저장
                bufferedCommands[id][cmd.sequenceId] = cmd;
            }
        }

        // (3) 디버그용 로그
        if (debugPacketLoss) {
            SetColor(14);
            for (auto& p : expectedSeqMap) {
                int cid = p.first;
                int exp = p.second;
                int lost = lostPacketMap[cid];
                double rate = exp > 0 ? (lost * 100.0 / exp) : 0.0;
                std::cout << "[Debug] Client " << cid
                    << " Packet loss: " << lost << "/" << exp
                    << " (" << rate << "%)" << std::endl;
            }
            SetColor(7);
        }

        // (4) 상태 브로드캐스트
        if (clock::now() >= nextBC) {
            broadcastStates();
            nextBC = clock::now() + bcInt;
        }
    }
}


void CServer::consumeInputQueue() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto& kv : inputQueues) {
            int id = kv.first;
            auto& q = kv.second;
            while (!q.empty()) {
                processCommand(q.front(), id);
                q.pop();
            }
        }
    }
}

void CServer::processCommand(const ClientCommand& msg, int id) {
    std::cout << "[Apply] Executing seq=" << msg.sequenceId << " for client=" << id << std::endl;

    auto& st = clientStates[id];
    switch (msg.cmd) {
    case CMD_UP:    st.y += 1; break;
    case CMD_DOWN:  st.y -= 1; break;
    case CMD_LEFT:  st.x -= 1; break;
    case CMD_RIGHT: st.x += 1; break;
    }
}