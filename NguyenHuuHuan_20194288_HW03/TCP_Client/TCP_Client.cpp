// TCP_Client.cpp : Defines the entry point for the console application.
//

#include "stdio.h"
#include "string"
#include "stdlib.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "iostream"
#define BUFF_SIZE 2048
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

// process messages received from the server
char * printResult(char *);

// display user interface
void show();

int main(int argc, char * argv[])
{
	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported");
		return 0;
	}

	//Step 2: Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot construct client socket.", WSAGetLastError());
		return 0;
	}

	//Step 3: Specify server socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

	//Step 4: Request to connect server
	if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot connect server.", WSAGetLastError());
		return 0;
	}
	printf("Connected server!\n");

	//Step 5: Communicate with server
	int n, ret;
	char buff[BUFF_SIZE], username[BUFF_SIZE], article[BUFF_SIZE], message[BUFF_SIZE];
	while (true) {
		show();
		cin >> n;

		// process user input 
		cin.ignore();
		if (n == 1) {
			strcpy(buff, "USER ");
			printf("Nhap tai khoan cua ban: ");
			gets_s(username, BUFF_SIZE);
			strcat(buff, username);
			strcat(buff, "#");
		}
		else if (n == 2) {
			strcpy(buff, "POST ");
			printf("Nhap thong tin bai dang: ");
			gets_s(article, BUFF_SIZE);
			strcat(buff, article);
			strcat(buff, "#");
		}
		else if (n == 3) {
			strcpy(buff, "BYE");
			strcat(buff, "#");
		}
		int messageLen = strlen(buff);

		// Send message to server
		ret = send(client, buff, messageLen, 0);
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot send data.\n", WSAGetLastError());
		}

		//Receive message
		ret = recv(client, buff, BUFF_SIZE, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				printf("Time out!");
			}
			else printf("Error %d: Cannot receive data", WSAGetLastError());
		}
		else if (strlen(buff) > 0) {
			buff[ret] = 0;

			// process stream
			char result[BUFF_SIZE];
			int indexBuff = 0, indexResult = 0;
			while (buff[indexBuff]) {
				// ENDING_DELIMITER, handle the message
				if (buff[indexBuff] == '#') {
					result[indexResult] = 0;
					// handle message from server
					char *message = printResult(result);
					printf("Ket qua tra ve tu server : %s\n", message);
					indexBuff++;
					indexResult = 0;
					continue;
				}
				// not ENDING_DELIMITER, copy from rBuff to message
				result[indexResult] = buff[indexBuff];
				indexBuff++;
				indexResult++;
			}
		}
	}

	//Close socket
	closesocket(client);
	// Terminate winsock
	WSACleanup();
	return 0;
}

// process messages received from the server
char * printResult(char *buff) {
	if ((string)buff == "10") {
		return "Dang nhap thanh cong";
	}
	if ((string)buff == "11") {
		return "Tai khoan bi khoa";
	}
	if ((string)buff == "12") {
		return "Tai khoan khong ton tai";
	}
	if ((string)buff == "13") {
		return "Tai khoan da duoc su dung o client khac";
	}
	if ((string)buff == "14") {
		return "Ban phai dang xuat phien hien tai truoc khi dang nhap bang tai khoan khac";
	}
	if ((string)buff == "20") {
		return "Dang bai thanh cong";
	}
	if ((string)buff == "21") {
		return "Chua dang nhap";
	}
	if ((string)buff == "30") {
		return "Dang xuat thanh cong";
	}
	if ((string)buff == "99") {
		return "Khong xac dinh duoc thong diep yeu cau";
	}
}

// display user interface
void show() {
	printf("============MENU============\n");
	printf("1.Dang nhap \n");
	printf("2.Dang bai \n");
	printf("3.Dang xuat \n");
	printf("Vui long nhap lua chon cua ban: ");
	return;
}