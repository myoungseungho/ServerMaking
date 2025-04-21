#include "Server.h"
#include <cstring>
#include <sstream>
#include <conio.h>



CServer::CServer() {
    WSAData wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    u_long nonBlocking = 1;
    ioctlsocket(serverSocket, FIONBIO, &nonBlocking);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    std::cout << "UDP Dedicated Server 실행 중..." << std::endl;
}

CServer::~CServer() {
    closesocket(serverSocket);
    WSACleanup();
}

//클라 주소 -> 고유번호 id 매핑
int CServer::getClientNumber(const sockaddr_in& addr) {
    char ipStr[INET_ADDRSTRLEN];
    InetNtopA(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr));
    char key[64];
    sprintf_s(key, sizeof(key), "%s:%d", ipStr, ntohs(addr.sin_port));

    // 새로운 클라이언트면 번호 할당 및 초기 Position 세팅
    if (clientIds.find(key) == clientIds.end()) {
        int newId = (int)clientIds.size() + 1;
        clientIds[key] = newId;
        clientStates[newId] = PlayerState();  // 기본 생성자로 초기 상태 설정 (x,y,health,score)  // 기본 생성자를 통해 (0,0) 초기화
        std::cout << "클라이언트 " << newId << "번 입장" << std::endl;
    }
    return clientIds[key];
}

//벡터 순회 이거 최적화 괜찮을까?
bool CServer::isNewClient(const sockaddr_in& addr) {
    for (auto& c : clients) if (memcmp(&c, &addr, sizeof(addr)) == 0) return false;
    return true;
}

void CServer::broadcastStates() {
    std::ostringstream oss;
    //모든 클라를 순회하면서
    for (auto& kv : clientStates) {
        const auto& st = kv.second;
        // PlayerState의 모든 필드(위치, 체력, 점수 등) 직렬화
        //oss로 문자열 스트림을 만들고 텍스트 형태로 하나씩 붙여줘서 oss.str()로 완성된 문자열 꺼내기
        oss << kv.first
            << ":(" << st.x << "," << st.y << ")"
            << ",HP=" << st.health
            << ",Score=" << st.score
            << ";";
    }
    // 직렬화된 문자열 생성
    std::string data = oss.str();

    // 모든 클라이언트에 브로드캐스트
    for (auto& clientAddr : clients) {
        sendto(serverSocket, data.c_str(), (int)data.size() + 1, 0,
            (sockaddr*)&clientAddr, sizeof(clientAddr));
    }
}


void CServer::sendAck(const sockaddr_in& cl, int clientId, int seq) {
    AckPacket ack{ clientId, seq };
    sendto(serverSocket, reinterpret_cast<char*>(&ack), sizeof(ack), 0,
        (sockaddr*)&cl, sizeof(cl));
}

void CServer::sendNack(const sockaddr_in& cl, int clientId, int missingSeq) {
    NackPacket nack{ clientId, missingSeq };
    sendto(serverSocket, reinterpret_cast<char*>(&nack), sizeof(nack), 0,
        (sockaddr*)&cl, sizeof(cl));
}

void CServer::consumeInputQueue() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        std::lock_guard<std::mutex> lock(queueMutex);
        for (std::unordered_map<int, std::queue<ClientCommand>>::iterator it = inputQueues.begin(); it != inputQueues.end(); ++it) {
            int id = it->first;
            std::queue<ClientCommand>& queue = it->second;
            while (!queue.empty()) {
                processCommand(queue.front(), id);
                queue.pop();
            }
        }
    }
}


void CServer::start() {
    using clock = std::chrono::high_resolution_clock;
    auto frameDur = std::chrono::milliseconds(1000 / LOGIC_HZ);
    auto bcInt = std::chrono::milliseconds(1000 / BROADCAST_HZ);
    auto nextBC = clock::now();

    int messageCount = 0;

    if (useThreadedProcessing) {
        std::thread(&CServer::consumeInputQueue, this).detach();
    }

    while (true) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'I' || ch == 'i') {
                useInputQueue = !useInputQueue;
                SetColor(11);
                std::cout << "[서버] 입력큐 사용: " << (useInputQueue ? "ON (입력큐)" : "OFF") << std::endl;
                SetColor(7);
            }
        }

        auto start = clock::now();
        char buf[BUFFER_SIZE];
        sockaddr_in cl;
        int len = sizeof(cl);

        if (debugMessagesPerFrame) messageCount = 0;

        while (clock::now() - start < frameDur) {
            int b = recvfrom(serverSocket, buf, BUFFER_SIZE, 0,
                reinterpret_cast<sockaddr*>(&cl), &len);
            if (b <= 0) continue;

            if (debugMessagesPerFrame) ++messageCount;

            if (isNewClient(cl)) clients.push_back(cl);
            ClientCommand cmd;
            memcpy(&cmd, buf, sizeof(cmd));
            int id = getClientNumber(cl);

            sendAck(cl, id, cmd.sequenceId);
            if (cmd.sequenceId != expectedSeqMap[id]) {
                for (int missed = expectedSeqMap[id]; missed < cmd.sequenceId; ++missed) {
                    sendNack(cl, id, missed);
                    lostPacketMap[id] += 1;
                }
            }
            expectedSeqMap[id] = cmd.sequenceId + 1;

            if (debugInputQueue) {
                long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                long long delay = now - cmd.timestamp;
                SetColor(13);
                std::cout << "[Debug] Client " << id << " Input delay: " << delay << " ms" << (useInputQueue ? " (입력큐)" : "") << std::endl;
                SetColor(7);
            }

            if (useInputQueue) {
                std::lock_guard<std::mutex> lock(queueMutex);
                inputQueues[id].push(cmd);
            }
            else {
                processCommand(cmd, id);
            }

            sendto(serverSocket, reinterpret_cast<char*>(&cmd), sizeof(cmd), 0,
                reinterpret_cast<sockaddr*>(&cl), len);
        }

        if (debugMessagesPerFrame) {
            SetColor(14);
            std::cout << "[Debug] Messages this frame: " << messageCount << (useInputQueue ? " (입력큐)" : "") << std::endl;
            SetColor(7);
        }

        if (debugPacketLoss) {
            for (const auto& pair : expectedSeqMap) {
                int id = pair.first;
                int expected = pair.second;
                int lost = lostPacketMap[id];
                double lossRate = expected > 0 ? (lost * 100.0 / expected) : 0.0;
                SetColor(12);
                std::cout << "[Debug] Client " << id << " Packet loss: "
                    << lost << "/" << expected << " (" << lossRate << "% )" << (useInputQueue ? " (입력큐)" : "") << std::endl;
                SetColor(7);
            }
        }

        if (useInputQueue && !useThreadedProcessing) {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (std::unordered_map<int, std::queue<ClientCommand>>::iterator it = inputQueues.begin(); it != inputQueues.end(); ++it) {
                int id = it->first;
                std::queue<ClientCommand>& queue = it->second;
                while (!queue.empty()) {
                    processCommand(queue.front(), id);
                    queue.pop();
                }
            }
        }

        if (clock::now() >= nextBC) {
            broadcastStates();
            nextBC = clock::now() + bcInt;
        }
    }
}
