#include "Client.h"
#include <thread>

// 클라이언트 메시지 수신 전용 함수
void receiveMessages(SOCKET clientSocket) {
	char buffer[BUFFER_SIZE];

	while (true) {
		memset(buffer, 0, BUFFER_SIZE);
		int receivedBytes = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, NULL, NULL);

		if (receivedBytes > 0) {
			std::cout << "\n" << buffer << std::endl;  // 서버에서 보내는 형식 그대로 출력
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));  // CPU 부하 방지
	}
}

CClient::CClient() {

	//윈도우에서 소켓 프로그래밍을 사용할 수 있도록 WINSOCK 라이브러리 초기화
	//소켓 준비하는 과정임
	WSAData wsa;
	clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

	//클라가 데이터 전송할 서버의 네트워크 주소 (IP와 포트) 설정
	//클라가 어떤 서버로 보낼지 지정
	serverAddr.sin_family = AF_INET; //IP 주소 체계 설정 IPv4 주소 체계 사용한다는 의미 IPv6이라면 AF_INET6을 사용해야 함
	inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr); //✔ 서버의 IP 주소를 설정하는 부분
	//SERVER_IP는 문자열 형태("127.0.0.1" 같은)로 저장되어 있지만,
	//네트워크 프로그래밍에서는** "숫자 형태의 이진 데이터" * *로 변환해야 함.
	// inet_pton() 함수가 이 변환을 수행함.
	// "SERVER_IP 문자열을 네트워크 주소 형식으로 변환하여 serverAddr.sin_addr에 저장한다."
	// 즉, "이 클라이언트는 SERVER_IP로 데이터를 전송할 준비를 한다."
	serverAddr.sin_port = htons(SERVER_PORT);
	//서버 포트 번호 설정
}

CClient::~CClient() {
	closesocket(clientSocket);
	WSACleanup();
}

void CClient::start() {
	char buffer[BUFFER_SIZE];

	// 서버로부터 메시지를 실시간으로 받을 수 있도록 수신 스레드 실행
	std::thread recvThread(receiveMessages, clientSocket);
	//메인스레드와 독립적으로 실행되도록 설정
	recvThread.detach();

	while (true) {
		std::cout << "명령어 입력: ";
		std::cin.getline(buffer, BUFFER_SIZE);

		// 서버에 메시지 전송
		sendto(clientSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	}
}
