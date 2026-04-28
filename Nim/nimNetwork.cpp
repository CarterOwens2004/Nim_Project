#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Nim.h"
#include "nimGame.h"
#include <string>

using std::cout;
using std::cin;
using std::endl;
using std::string;


ServerStruct chooseServer(SOCKET s, ServerStruct serverList[]) {
	//Who? prints avaiable servers
	int num = getServers(s, serverList);

	if (num == 0) {
		cout << "None found" << endl;
		return chooseServer(s, serverList);;

	}
	for (int i = 0; i < num; i++)
		cout << i + 1 << ". " << serverList[i].name << endl;

	//choose server
	cout << "Choose a server to challenge: ";
	int choice;
	cin >> choice;
	cin.ignore();

	if (choice < 1 || choice > num)
		return chooseServer(s, serverList);

	return serverList[choice - 1];

}

bool challenge(SOCKET sock, ServerStruct target, const char* client_name) {
	char sendBuf[DEFAULT_BUFLEN] = "";
	strcpy(sendBuf, Nim_CHALLENGE);
	strncat(sendBuf, client_name, DEFAULT_BUFLEN - strlen(sendBuf) - 1);

	sendto(sock, sendBuf, (int)strlen(sendBuf) + 1, 0, (sockaddr*)&target.addr, sizeof(target.addr));

	//rename target
	if (wait(sock, 10, 0)) {
		char recvBuf[DEFAULT_BUFLEN];
		sockaddr_in recvAddr;
		int addrLen = sizeof(recvAddr);

		int bytesReceived = recvfrom(sock, recvBuf, DEFAULT_BUFLEN - 1, 0, (sockaddr*)&recvAddr, &addrLen);

		if (bytesReceived <= 0) {
			return false;
		}

		recvBuf[bytesReceived] = '\0';

		if (recvAddr.sin_addr.s_addr != target.addr.sin_addr.s_addr) {
			return false;
		}

		if (_stricmp(recvBuf, Nim_YES) == 0) {
			sendto(sock, Nim_GREAT, (int)strlen(Nim_GREAT) + 1, 0,
				(sockaddr*)&target.addr, sizeof(target.addr));
			return true;
		}

		return false;
	}
}

void clientNegotions(const char* client_name) {

	SOCKET clientSocket = INVALID_SOCKET;
	clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket == INVALID_SOCKET) {
		cout << "SOCKET() failed: " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}


	ServerStruct servers[MAX_SERVERS];

	while (true) {
		ServerStruct target = chooseServer(clientSocket, servers);

		if (challenge(clientSocket, target, client_name)) {
			
			
			break;// ties into game loop
		}
	}
}

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

void negotiateServer(char user[], GameState game) {
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
				string yesBuf = Nim_YES;
				iResult = sendto(hostSocket, yesBuf.data(), (int)sizeof(yesBuf.data()) + 1, 0, (sockaddr*)&opponent.addr, (int)sizeof(opponent.addr));
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
				string noBuf = Nim_NO;
				iResult = sendto(hostSocket, noBuf.data(), (int)sizeof(noBuf.data()) + 1, 0, (sockaddr*)&opponent.addr, (int)sizeof(opponent.addr));
				if (iResult == SOCKET_ERROR) {
					cout << "sendto failed with error: " + WSAGetLastError() << '\n';
					return;
				}
			}
		}
		iResult = recvfrom(hostSocket, recvBuf, DEFAULT_BUFLEN, 0, (sockaddr*)&addr, &addrSize);
	}
	if (iResult == SOCKET_ERROR) { // if the loop actually hit the sentinel--i.e., the pre-test loop condition was false
		cout << "recvfrom failed with error: " << WSAGetLastError() << '\n';
		return;
	}
	// If we've found a game and confirmed the 3-way handshake ("Player=" -> "YES" -> "GREAT"), initialize the gameboard and start the game loop
	initGame(game, false);
	// gameLoop(game, opponent);
}


int main(){
	char user_buf[MAX_NAME];
	cout << "What is your name?" << endl;
	cin.getline(user_buf, MAX_NAME);
	cout << "Would you prefer to host a game or to challenge some other host? \n"
		 << "Type \"Host\" to host a game or \"Challange\" to challenge a host";

	
    return 0;
}