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
    AckPacket ack{ clientId, seq };
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
    NackPacket nack{ clientId, missingSeq };
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

    if (useThreadedProc) std::thread(&CServer::consumeInputQueue, this).detach();

    while (true) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'A' || ch == 'a') {
                enableAckNack = !enableAckNack;
                std::cout << "[Info] ACK/NACK 기능: " << (enableAckNack ? "ON" : "OFF") << std::endl;
            }
            else if (ch == 'I' || ch == 'i') {
                useInputQueue = !useInputQueue;
            }
        }
        auto startTime = clock::now();
        char buf[BUFFER_SIZE];
        sockaddr_in cl;
        int len = sizeof(cl);
        while (clock::now() - startTime < frameDur) {
            int bytes = recvfrom(serverSocket, buf, BUFFER_SIZE, 0,
                reinterpret_cast<sockaddr*>(&cl), &len);
            if (bytes <= 0) continue;
            if (simulatePacketLoss && (rand() % 100) < PACKET_LOSS_PERCENT) {
                if (debugPacketLoss) {
                    SetColor(12);
                    std::cout << "[Debug] Dropped receive packet" << std::endl;
                    SetColor(7);
                }
                continue;
            }
            if (isNewClient(cl)) clients.push_back(cl);
            ClientCommand cmd;
            memcpy(&cmd, buf, sizeof(cmd));
            int id = getClientNumber(cl);

            // 누락 감지 & 요청
            if (cmd.sequenceId != expectedSeqMap[id]) {
                for (int m = expectedSeqMap[id]; m < cmd.sequenceId; ++m) {
                    ++lostPacketMap[id];
                    if (debugPacketLoss) {
                        SetColor(12);
                        std::cout << "[Debug] Detected missing packet " << m
                            << " for client " << id
                            << ", lost count now " << lostPacketMap[id] << std::endl;
                        SetColor(7);
                    }
                    if (enableAckNack) sendNack(cl, id, m);
                }
            }
            expectedSeqMap[id] = cmd.sequenceId + 1;

            // ACK 전송 (내부에서 복구)
            sendAck(cl, id, cmd.sequenceId);

            if (useInputQueue) {
                std::lock_guard<std::mutex> lock(queueMutex);
                inputQueues[id].push(cmd);
            }
            else {
                processCommand(cmd, id);
            }
        }

        if (debugPacketLoss) {
            SetColor(14);
            for (auto& p : expectedSeqMap) {
                int cid = p.first;
                int exp = p.second;
                int lost = lostPacketMap[cid];
                double rate = exp > 0 ? (lost * 100.0 / exp) : 0.0;
                std::cout << "[Debug] Client " << cid
                    << " Packet loss: " << lost << "/" << exp
                    << " (" << rate << "% )" << std::endl;
            }
            SetColor(7);
        }
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