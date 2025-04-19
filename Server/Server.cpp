#include "Server.h"
#include <cstring>
#include <sstream>

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
    for (auto& kv : clientStates) {
        const auto& st = kv.second;
        // PlayerState의 모든 필드(위치, 체력, 점수 등) 직렬화
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


void CServer::start() {

    //구조체 clock
    using clock = std::chrono::high_resolution_clock;
    //millisecons 임시객체를 만들어 frameDuration 셋팅
    //입력은 빠르게 처리해야한다
    const auto frameDuration = std::chrono::milliseconds(1000 / LOGIC_HZ);
    //브로드캐스트는 패킷 폭주한다. 20Hz 정도로 낮춰 보내자.
    const auto broadcastInterval = std::chrono::milliseconds(1000 / BROADCAST_HZ);

    auto broadcastTimer = clock::now();

    //서버가 멈추지 않고 계속 동작
    while (true) {
        //현재 프레임 스타트
        auto frameStart = clock::now();
        //버퍼 사이즈 512
        char buffer[BUFFER_SIZE];

        //recvfrom 함수 호출 시 서버는 클라가 누구인지 알아야 한다.
        //udp는 연결이 없어서 누가 보냈는지 정보를 직접 받아와야 한다.
        //클라이언트의 ip주소와 포트 번호를 담을 공간이 필요하다.
        //이 ip 포트를 저장하는 구조체가 sockaddr_in이다.
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);

        // 로직 틱 (60Hz)
        //while이돌면서 현재와 시작점이 duration이 작다면 계속해서 호출한다.
        //한 프레임에 여러 입력이 들어오는건 다 처리해야 한다.
        while (clock::now() - frameStart < frameDuration) {
            int bytes = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0,
                (sockaddr*)&clientAddr, &addrLen);
            //아무것도 안들어왔다면
            if (bytes <= 0) continue;

            //새로운 클라이언트라면 추가
            if (isNewClient(clientAddr)) clients.push_back(clientAddr);

            ClientCommand cmd;
            memcpy(&cmd, buffer, sizeof(cmd));
            int id = getClientNumber(clientAddr);
            processCommand(cmd, id);
        }

        // 브로드캐스트 틱 (20Hz)
        auto now = clock::now();
        if (now - broadcastTimer >= broadcastInterval) {
            broadcastStates();
            broadcastTimer = now;
        }
    }
}
