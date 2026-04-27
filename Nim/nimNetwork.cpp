#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Nim.h"
#include "nim.cpp"
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

void negotiateServer(char user[]) {
	SOCKET hostSocket = establishServerSocket();
	if (hostSocket == SOCKET_ERROR) {
		cout << "establishServerSocket() failed.\n";
		return;
	}

	char recvBuf[DEFAULT_BUFLEN];
	int iResult = 0;
	struct sockaddr_in addr;
	int addrSize = sizeof(addr);

	ServerStruct opponent; // opponent's name and address saved and passed along

	iResult = recvfrom(hostSocket, recvBuf, DEFAULT_BUFLEN, 0, (sockaddr*)&addr, &addrSize);
	while (iResult != SOCKET_ERROR) { // We're waiting for a game
		cout << "Received " << iResult << " bytes from " << addr.sin_addr.S_un.S_addr << ": " << recvBuf << endl;

		if (_stricmp(recvBuf, Nim_QUERY)) {
			string sendBuf = Nim_NAME + (string)user;
			iResult = sendto(hostSocket, sendBuf.data(), (int)sendBuf.size() + 1, 0, (sockaddr*)&addr, addrSize);
			if (iResult == SOCKET_ERROR) {
				cout << "sendto failed with error: " + WSAGetLastError() << '\n';
				return;
			}
		}
		else if (_stricmp(recvBuf, Nim_CHALLENGE)) {
			char response[DEFAULT_BUFLEN];
			cin.getline(response, DEFAULT_BUFLEN); // or get it from the GUI

			if (_stricmp(response, Nim_YES)) {
				// parse the name from Nim_CHALLENGE
				string challenge = string(recvBuf);
				size_t pos = challenge.find('=');
				string name;
				if (pos != string::npos) name = challenge.substr(pos + 1);

				strcpy_s(opponent.name, name.data());
				opponent.addr = addr;

				// send YES to the client
				iResult = sendto(hostSocket, Nim_YES, (int)sizeof(Nim_YES) + 1, 0, (sockaddr*)&opponent.addr, (int)sizeof(opponent.addr));
				if (iResult == SOCKET_ERROR) {
					cout << "sendto failed with error: " + WSAGetLastError() << '\n';
					return;
				}

				// wait for GREAT
				while (wait(hostSocket, 2, 0) > 0) {
					iResult = recvfrom(hostSocket, recvBuf, DEFAULT_BUFLEN, 0, (sockaddr*)&addr, &addrSize);
					if (addr.sin_addr.S_un.S_addr != opponent.addr.sin_addr.S_un.S_addr) continue;
					if (iResult > 0 && _stricmp(recvBuf, Nim_GREAT)) break;
				}
			}
			else if (_stricmp(response, Nim_NO)) {
				// send NO to the client
				iResult = sendto(hostSocket, Nim_NO, (int)sizeof(Nim_NO) + 1, 0, (sockaddr*)&opponent.addr, (int)sizeof(opponent.addr));
				if (iResult == SOCKET_ERROR) {
					cout << "sendto failed with error: " + WSAGetLastError() << '\n';
					return;
				}
			}
		}
		iResult = recvfrom(hostSocket, recvBuf, DEFAULT_BUFLEN, 0, (sockaddr*)&addr, &addrSize);
	}
	if (iResult == SOCKET_ERROR) {
		cout << "recvfrom failed with error: " << WSAGetLastError() << '\n';
		return;
	}
	// If we've found a game and confirmed the 3-way handshake ("Player=" -> "YES" -> "GREAT"), initialize the gameboard and start the game loop
	GameState game;
	initGame(game, false);
	// gameLoop(game, opponent);
}