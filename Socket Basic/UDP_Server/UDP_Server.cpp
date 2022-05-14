#include "stdio.h"
#include "stdlib.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#define ENDING_DELIMITER "#"
#pragma comment(lib, "Ws2_32.lib")

int main(int argc, char* argv[])
{
	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Step 2: Construct socket
	SOCKET server;
	server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 0;
	}

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
	if (bind(server, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot bind this address.", WSAGetLastError());
		return 0;
	}
	printf("Server start\n");

	
	//Step 4: Communicate with client
	sockaddr_in clientAddr;
	char rBuff[BUFF_SIZE], clientIP[INET_ADDRSTRLEN];
	int ret, clientAddrLen = sizeof(clientAddr), clientPort;

	while (1)
	{
		//Receive message
		ret = recvfrom(server, rBuff, BUFF_SIZE, 0, (SOCKADDR *)&clientAddr, &clientAddrLen);
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot receive data.", WSAGetLastError());
		}
		else if (strlen(rBuff) > 0) {
			rBuff[ret] = 0;
			// get to address info
			addrinfo hints;
			memset(&hints, 0, sizeof(hints));
			addrinfo *result;
			int rc;
			hints.ai_family = AF_INET;
			rc = getaddrinfo(rBuff, NULL, &hints, &result);
			char ipStr[INET_ADDRSTRLEN];
			char sBuff[BUFF_SIZE]="";
			if (rc == 0) { // Found information
				strcat(sBuff, "+");
				sockaddr_in *address;
			    // Append all ipStr to buff
				while (result != NULL) {
					address = (struct sockaddr_in *) result->ai_addr;
					inet_ntop(AF_INET, &address->sin_addr, ipStr, sizeof(ipStr));
					strcat(sBuff, ipStr);
					strcat(sBuff, ENDING_DELIMITER);
					result = result->ai_next;
				}
				int buffLen = strlen(sBuff);
				sBuff[buffLen] = 0;
				
				//Echo to client
				ret = sendto(server, sBuff, strlen(sBuff), 0, (SOCKADDR *)&clientAddr, sizeof(clientIP));
				if (ret == SOCKET_ERROR)
					printf("Error: %d", WSAGetLastError());
			}
			else {
				//Not Found. Echo to client
				ret = sendto(server, "-", 2, 0, (SOCKADDR *)&clientAddr, sizeof(clientIP));
				if (ret == SOCKET_ERROR)
					printf("Error: %d", WSAGetLastError());
			}
			// free linked-list
			freeaddrinfo(result);

			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
			clientPort = ntohs(clientAddr.sin_port);
			printf("Receive from client %s: %d %s\n", clientIP, clientPort, rBuff);
		}

	}

	//Step 5: Close socket
	closesocket(server);

	//Step 6: Terminate Winsock
	WSACleanup();
	return 0;
}