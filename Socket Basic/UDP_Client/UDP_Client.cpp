#include "stdafx.h"
#include "stdio.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#define BUFF_SIZE 2048
#define ENDING_DELIMITER "#"
#pragma comment(lib, "Ws2_32.lib")

int main(int argc, char * argv[])
{
	// Step 1: Khoi tao Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	// Step 2: Khoi tao socket
	SOCKET client;
	client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 0;
	}

	printf("Client started!\n");

	// Thiet lap tuy chon cho socket
	int tv = 10000;
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));

	//Step 3: Xac dinh dia chi may chu
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

	//Step 4: Giao tiep voi server
	char buff[BUFF_SIZE];
	int ret, serverAddrLen = sizeof(serverAddr), messageLen;
	while (1) {
		// Send message
		printf("Send to server: ");
		gets_s(buff, BUFF_SIZE);
		messageLen = strlen(buff);
		if (messageLen == 0) break;

		ret = sendto(client, buff, messageLen, 0, (sockaddr *)&serverAddr, serverAddrLen);
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot send message.", WSAGetLastError());
		}

		ret = recvfrom(client, buff, BUFF_SIZE, 0, (sockaddr *)&serverAddr, &serverAddrLen);
		buff[ret] = 0;
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				printf("Time-out!");
			}
			else printf("Error %d: Cannot receive message.", WSAGetLastError());
		}
		else if (buff[0] == '+') { //Success
			for (int i = 1; i < ret; i++) {
				if (buff[i] == '#') {
					printf("\n");
					continue;
				}
				printf("%c", buff[i]);
			}
		}
		else if (buff[0] == '-') { // Fail
			printf("Not found information\n");
		}
	} //end while

	// Step 5: Close socket
	closesocket(client);

	//Step 6: Terminate Winsock
	WSACleanup();

	return 0;
}

