#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Nim.h"
#include <string>

using std::cout;
using std::cin;
using std::endl;
using std::string;

SOCKET establishServerSocket() {
	SOCKET StudySocket = INVALID_SOCKET;
	StudySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (StudySocket == INVALID_SOCKET) {
		cout << "error at socket(): " << WSAGetLastError() << '\n';
		WSACleanup();
		return SOCKET_ERROR;
	}

	struct sockaddr_in myAddr;
	myAddr.sin_family = AF_INET;
	myAddr.sin_port = htons(DEFAULT_PORT);
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int iResult = bind(StudySocket, (SOCKADDR*)&myAddr, sizeof(myAddr));
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << '\n';
		closesocket(StudySocket);
		WSACleanup();
		return SOCKET_ERROR;
	}
	return StudySocket;
}

void hostGame(char user[]) {

}