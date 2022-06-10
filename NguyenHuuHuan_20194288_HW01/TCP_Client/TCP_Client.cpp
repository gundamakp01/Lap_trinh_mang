#include "stdafx.h"
#include "stdio.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#define BUFF_SIZE 2048
#define ENDING_DELIMITER "#"
#pragma comment(lib, "WS2_32.lib")


int main(int argc, char * argv[])
{
	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Step 2: Contruct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 0;
	}

	//(optional) Set time-out for receiving
	int tv = 10000;
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char *)(&tv), sizeof(int));

	//Step 3: Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);
	
	//Step 4: Request to connect server
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot connect server.", WSAGetLastError());
		return 0;
	}
	printf("Connected server!\n");
	//Step 5: Communticate with server
	char buff[BUFF_SIZE];
	int ret, messageLen;
	while (1) {
		//Send message
		printf("Send to server: ");
		gets_s(buff, BUFF_SIZE);
		strcat(buff, ENDING_DELIMITER);

		messageLen = strlen(buff);
		if (messageLen == 0) break;

		ret = send(client, buff, messageLen, 0);
		if (ret == SOCKET_ERROR)
			printf("Error %d: Cannot send data.", WSAGetLastError());

		//Receive echo message
		ret = recv(client, buff, BUFF_SIZE, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT)
				printf("Error");
			else printf("Error %d: Cannot revceive data.", WSAGetLastError());
		}
		else if (strlen(buff) > 0) {
			// print result
			buff[ret] = 0;
			int i = 0, j = 0;
			char result[2048];
			while (buff[i]) {
				if (buff[i] == '#') {
					result[j] = '\0';
					printf("%s\n", result);
					i++;
					j = 0;
					continue;
				}
				result[j] = buff[i];
				i++; j++;
			}
		}
	}
    return 0;
}

