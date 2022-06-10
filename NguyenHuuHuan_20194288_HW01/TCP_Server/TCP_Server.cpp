#include "stdafx.h"
#include <winsock2.h>
#include <WS2tcpip.h>
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 1024
#define ENDING_DELIMITER "#"
#pragma comment(lib, "WS2_32.lib")

// convert int to char[]
void convert(int count, char *sBuff) {
	int temp1 = count, digits = 0, temp2 = 0;
	while (temp1 != 0) {
		temp1 /= 10;
		digits++;
	}
	sBuff[digits] = '\0';
	digits--;
	temp1 = count;
	while (digits >= 0) {
		temp2 = temp1 % 10;
		temp1 /= 10;
		sBuff[digits] = char(temp2 + '0');
		digits--;
	}
}
int main(int argc, char *argv[])
{
	//Step 1: Initiate Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Step 2: Construct socket
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 0;
	}

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("(Error: %d) Cannot associate a loacal address with server socket.", WSAGetLastError());
		return 0;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("(Error: %d)Connot place server socket in state LISTEN.", WSAGetLastError());
	}
	printf("Server started!\n");

	// Step 5: Communicate with client
	sockaddr_in clientAddr;
	char rBuff[BUFF_SIZE], clientIP[INET_ADDRSTRLEN], sBuff[BUFF_SIZE], sString[BUFF_SIZE];
	int ret, clientAddrLen = sizeof(clientAddr), clientPort;
	SOCKET connSock;
	// accept request
	connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen);
	if (connSock == SOCKET_ERROR) {
		printf("(Error: %d) Cannot permit incoming connection.", WSAGetLastError());
		return 0;
	}
	else {
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
		clientPort = ntohs(clientAddr.sin_port);
		printf("Accept incoming connection from %s:%d\n", clientIP, clientPort);
	}


	int count = 0; 
	while (1) {
		//receive message from client
		ret = recv(connSock, rBuff, BUFF_SIZE, 0);
		rBuff[ret] = '\0';
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot receive data.", WSAGetLastError());
			break;
		}
		if (ret == 0) {
			printf("Client disconnects.\n");
			break;
		}
		printf("Receive from client [%s:%d] %s\n", clientIP, clientPort, rBuff);
		int i = 0;
		while (rBuff[i]) {
			// get # then send to client
			if (rBuff[i] == '#') {
				if (count == -1) {
					//Echo to client
					char *sFail = "Failed: String contains non-number character.#";
					send(connSock, sFail, strlen(sFail), 0);
					if (ret == SOCKET_ERROR) {
						printf("Error %d: Cannot send data.\n", WSAGetLastError());
						break;
					}
				}
				else {
					// Convert int to char[]
					convert(count, sBuff);
					strcat(sBuff, "#");

					//Echo to client
					send(connSock, sBuff, strlen(sBuff), 0);
					if (ret == SOCKET_ERROR) {
						printf("Error %d: Cannot send data.\n", WSAGetLastError());
						break;
					}
				}
				count = 0;
				i++;
				continue;
			}
			// skip ulti the end of the message (get ENDING_DELIMITER)
			if (count == -1) {
				i++;
				continue;
			}
			// if not a number
			if (rBuff[i] < 48 || rBuff[i] > 57) {
				count = -1;
				i++;
				continue;
			}
			count += (int)(rBuff[i] - '0');
			i++;
		}
	} //end communicating

	closesocket(listenSock);
	closesocket(connSock);

	WSACleanup();
	return 0;
}

