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

void gameLoop(SOCKET sock, ServerStruct opponent, GameState& game) {

	//check to see if host or client
	if(!game.iAmClient){
		// if host build and send the board
		char boardBuf[32];
		buildBoardDatagram(game, boardBuf);
		sendto( sock,
				boardBuf, 
				(int)strlen(boardBuf) + 1, 
				0, 
				(sockaddr*)&opponent.addr, 
				sizeof(opponent.addr));
		
	}
	else {
		//client wait and receives the board

		if(!wait(sock, 30, 0)){		// 30 second timer waiting for board
			cout << "No board received. You win by default.\n";
			endGame(game, STATUS_WIN_DEFAULT);
			return;
		}
		// if board is actually sent
		// client parses the board
		char boardBuf[32];
		sockaddr_in addr;
		int addrLen = sizeof(addr);
		int bytes = recvfrom(sock, 
							boardBuf, 
							sizeof(boardBuf)-1, 
							0, 
							(sockaddr*)&addr, 
							&addrLen);
		boardBuf[bytes] = '\0';
		
		// check to see if the board is valid
		if (!parseBoardDatagram(boardBuf, game.piles, game.numPiles)){
			cout << "Invalid board received. You win by default.\n";
			endGame(game, STATUS_WIN_DEFAULT);
			return;
		}
	}

	// Enter the main game loop
	// AKA the game has finally started
	while(game.status == STATUS_ACTIVE){
		// show the board
        for (int i = 0; i < game.numPiles; i++){
    			cout << "Pile " << i+1 << ": " << game.piles[i] << " rocks\n";
		}


		if(game.myTurn){
			
			//When it is their turn, give them the three options
			cout << "Chat (c), Forfeit (f), or Move(m)?";
			char action[4];
			cin.getline(action, 4);
			
			// Forfeit
			if (_stricmp(action, "f") == 0){
				sendto(sock, "F", 2, 0 , (sockaddr*)&opponent.addr, sizeof(opponent.addr));
				endGame(game, STATUS_FORFEIT_LOSE);
				break;
			}
			// Chat
			else if (_stricmp(action, "c") == 0){
				char message[80];
				cout << "Message: ";
				cin.getline(message, 80);

				char chatBuf[82];
				chatBuf[0] = 'C';
				strncpy(chatBuf + 1, message, 80);
				chatBuf[81] = '\0';
				sendto(sock, chatBuf, (int)strlen(chatBuf) + 1, 0, (sockaddr*)&opponent.addr, sizeof(opponent.addr));
				continue;
			} 
			// If it is not a move, sends user back to the top
			else if (_stricmp(action, "m") != 0) {
   				 cout << "Invalid option.\n";
    			continue;
			}

			int pile = 0, count = 0;
			// or get move
			cout << "Pile: ";
			cin >> pile;
			cout << "Rocks to remove: ";
			cin >> count;
			cin.ignore();

            MoveResult r = validateOutgoingMove(game, pile, count);
            if (r == MOVE_BAD_PILE)  { 
				/* show "invalid pile" error */ 
				cout << "Not a valid pile\n";
				continue; 
			}
            if (r == MOVE_BAD_COUNT) {
				 /* show "invalid count" error */ 
				 cout << "Not a valid count\n";
				 continue; 
			}

            char moveBuf[4];
            buildMoveDatagram(pile, count, moveBuf);
            // send moveBuf as UDP datagram to opponent
			sendto( sock, 
					moveBuf, 
					(int)strlen(moveBuf) + 1, 
					0, 
					(sockaddr*)&opponent.addr, 
					sizeof(opponent.addr)
				);

            applyMove(game, pile, count);
            // update the board display

            if (checkWin(game)) {
                endGame(game, STATUS_WIN);
                // show winning message
				cout << "YOU WIN!!!!\n";
                break;
            }
		} else {
			//receive and validate oppponent moves
			
			//gui shwos a waiting message
			char recvBuf[256] = {0};
            int pile = 0, count = 0;

			if (!wait(sock, 30, 0)) {
  				cout << "Opponent timed out. You win by default.\n";
   				endGame(game, STATUS_WIN_DEFAULT);
    			break;
			}

			sockaddr_in addr;
			int addrLen = sizeof(addr);
			int bytes = recvfrom(sock, 
							recvBuf, 
							sizeof(recvBuf)-1, 
							0, 
							(sockaddr*)&addr, 
							&addrLen
						);
			recvBuf[bytes] = '\0';
			
			// make sure message is only from our opp
			if (addr.sin_addr.s_addr != opponent.addr.sin_addr.s_addr){
   				continue;
			}
            MoveResult r = validateIncomingMove(game, recvBuf, pile, count);

            if (r == MOVE_CHAT) {
				cout << "Opponent: " << (recvBuf + 1) << "\n";
                continue; // keep waiting, don't switch turns, just chat
            }

            if (r == MOVE_FORFEIT) {
                endGame(game, STATUS_FORFEIT_WIN);
				cout << "Opponent forfeited. YOU WIN!!!\n";
                break;
            }

            if (r != MOVE_OK) {
                endGame(game, STATUS_WIN_DEFAULT);
                cout << "Opponent made an invalid move. YOU WIN!!!\n";
                break;
            }

            applyMove(game, pile, count);

            if (checkWin(game)) {
                endGame(game, STATUS_LOSE);
                // gui show losing message
				cout << "YOU LOSE!!!\n";
                break;
            }

		}

	}

}

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
	return false;
}

void clientNegotions(const char* client_name, GameState& game) {

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
			
			// start a game as client
			initGame(game, true);
			gameLoop(clientSocket, target, game);
			break;	// where the game will be played
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

void negotiateServer(char user[], GameState& game) {
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
	cout << "Waiting for challengers...\n";
	iResult = recvfrom(hostSocket, recvBuf, DEFAULT_BUFLEN, 0, (sockaddr*)&addr, &addrSize);
	while (iResult != SOCKET_ERROR) { // We're waiting for a game
		// cout << "Received " << iResult << " bytes from " << addr.sin_addr.S_un.S_addr << ": " << recvBuf << endl;

		if (_stricmp(recvBuf, Nim_QUERY)) {
			string sendBuf = Nim_NAME + (string)user;
			iResult = sendto(hostSocket, sendBuf.data(), (int)sendBuf.size() + 1, 0, (sockaddr*)&addr, addrSize);
			if (iResult == SOCKET_ERROR) {
				cout << "sendto failed with error: " + WSAGetLastError() << '\n';
				return;
			}
		}
		else if (_stricmp(recvBuf, Nim_CHALLENGE)) {
			// parse name first
    		string challengeMsg = string(recvBuf);
    		size_t pos = challengeMsg.find('=');
    		string challengerName;
    		if (pos != string::npos) {
       			challengerName = challengeMsg.substr(pos + 1);
			}
			// prompt
			cout << "You've been challenged by " << challengerName << "! Accept? (YES/NO): ";
			
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
	gameLoop(hostSocket, opponent, game);
}

int main(){
	// WSAStartup
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	//Get User's name
	char user_buf[MAX_NAME];
	cout << "What is your name?" << endl;
	cin.getline(user_buf, MAX_NAME);
	
	// Host and/or Challenge Loop
	// Loops until either user chooses host or challenge,
	// And loops so that when game finishes, it asks to host or challenge again
	while (true) {
        cout << "Host (h) or Challenge (c)? ";
        char choice[8];
        cin.getline(choice, 8);

        GameState game = {};

        if (_stricmp(choice, "h") == 0) {
            negotiateServer(user_buf, game);
        } else if (_stricmp(choice, "c") == 0) {
            clientNegotions(user_buf, game);
        } else {
            cout << "Invalid choice.\n";
			continue;
		}
    }

	// Cleanup
	WSACleanup();
    return 0;
}