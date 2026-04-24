#pragma once

#define MAX_NAME         80
#define MAX_SERVERS      100
#define DEFAULT_BUFLEN   512
#define DEFAULT_PORT     29333
#define Nim_QUERY		 		 "Who?"
#define Nim_NAME	   	   "Name="
#define Nim_CHALLENGE    "Player="
#define Nim_YES					 "YES"
#define Nim_NO					 "NO"
#define Nim_GREAT				 "GREAT!"

struct ServerStruct {
	char name[MAX_NAME];
	sockaddr_in addr;
};

int getServers(SOCKET s, ServerStruct server[]);
int wait(SOCKET s, int seconds, int msec);
sockaddr_in GetBroadcastAddress(char* IPAddress, char* subnetMask);
sockaddr_in GetBroadcastAddressAlternate(char* IPAddress, char* subnetMask);