// WSAEventSelectServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <conio.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <process.h>
#include <fstream>
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;
CRITICAL_SECTION criticalSection;
typedef struct session {
	SOCKET sock;
	sockaddr_in clientAddr;
	string account;
} session;

#define BUFF_SIZE 2048
#define SERVER_ADDR "127.0.0.1"
#define MAX_NUM_THREAD 1000
session clientSession[1000];
vector<pair<string, string> > userAccount;
vector<pair<string, string> >::iterator iterAccount;
int isCreated[MAX_NUM_THREAD] = { 0 };

// handle when user logs in
char *login(char *, session *);

// handle when user posts
char *post(char *, session *);

// handle when user logs out
char *logout(session *);


// slice message to get main message
char *subMessage(char *, int);

// defines how the server should process when a user makes a message
char * handle(char *, session *);

// read file into userAccount
int readFile();

/* procThread - Thread to receive the message from client and process*/
unsigned __stdcall procThread(void *);


int main(int argc, char* argv[])
{
	DWORD		nEvents = 0;
	DWORD		index;
	SOCKET		listenSock;
	WSAEVENT	eventListen[1];
	WSANETWORKEVENTS sockEvent;


	readFile();
	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Step 2: Construct LISTEN socket	
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);


	eventListen[0] = WSACreateEvent(); //create new events
	nEvents++;

	// Associate event types FD_ACCEPT and FD_CLOSE
	// with the listening socket and newEvent   
	WSAEventSelect(listenSock, eventListen[0], FD_ACCEPT | FD_CLOSE);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error %d: Cannot associate a local address with server socket.", WSAGetLastError());
		return 0;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error %d: Cannot place server socket in state LISTEN.", WSAGetLastError());
		return 0;
	}

	printf("Server started!\n");

	SOCKET connSock;
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);
	InitializeCriticalSection(&criticalSection);
	while (1) {
		//wait for network events on all socket
		index = WSAWaitForMultipleEvents(1, eventListen, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			printf("Error %d: WSAWaitForMultipleEvents() failed\n", WSAGetLastError());
			break;
		}

		WSAEnumNetworkEvents(listenSock, eventListen[0], &sockEvent);

		if (sockEvent.lNetworkEvents & FD_ACCEPT) {
			if (sockEvent.iErrorCode[FD_ACCEPT_BIT] != 0) {
				printf("FD_ACCEPT failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
				break;
			}

			if ((connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen)) == SOCKET_ERROR) {
				printf("Error %d: Cannot permit incoming connection.\n", WSAGetLastError());
				break;
			}
			//Add new socket into socks array
			int i;
			for (i = 0; i < 1000; i++)
				if (clientSession[i].sock == 0) {
					session *tempSession = new session;
					// set session parameters
					tempSession->clientAddr = clientAddr;
					tempSession->sock = connSock;
					clientSession[i].sock = connSock;
					// add to session array
					clientSession[i] = *tempSession;
					break;
				}
			// if i is a multiple of 64 and the thread has not been created then create a new thread
			int numThread = i / 64;

			if (i % 64 == 0 && !isCreated[numThread]) {
				_beginthreadex(0, 0, procThread, (void *)&i, 0, 0);	
			}

			//reset event
			WSAResetEvent(eventListen);
		}
	}
	cout << "dsada";
	DeleteCriticalSection(&criticalSection);
	closesocket(listenSock);
	WSACleanup();
	return 0;
}


char *login(char *message, session *userSession) {
	char *result = "";
	// The current session is session in
	EnterCriticalSection(&criticalSection);
	if (userSession->account != "") {
		result = "14#";
	}
	LeaveCriticalSection(&criticalSection);
	if (result != "") return result;
	// check accounts that are already session in elsewhere
	EnterCriticalSection(&criticalSection);
	for (int i = 0; i < 1000; i++) {
		if (clientSession[i].account == (string)message) {
			result = "13#";
			break;
		}
	}
	LeaveCriticalSection(&criticalSection);
	if (result != "") return result;

	// Validate loggin
	for (iterAccount = userAccount.begin(); iterAccount != userAccount.end(); iterAccount++) {
		if (iterAccount->first == (string)message) {
			// active account
			if (iterAccount->second == "0") {
				EnterCriticalSection(&criticalSection);
				userSession->account = iterAccount->first;
				LeaveCriticalSection(&criticalSection);
				return "10#";
			}
			// locked account
			if (iterAccount->second == "1") {
				return "11#";
			}
		}
	}
	return "12#";
}

// handle when user post
char *post(char *message, session *userSession) {
	char *result = "";
	EnterCriticalSection(&criticalSection);
	if (userSession->account == "") {
		result = "21#";
	}
	LeaveCriticalSection(&criticalSection);
	if (result != "") return result;
	return "20#";
}

// handle when user logs out
char *logout(session *userSession) {
	char *result = "";
	// The current session is not session in
	EnterCriticalSection(&criticalSection);
	if (userSession->account == "") {
		result = "21#";
	}
	else {
		// Log out of the current session
		userSession->account = "";
		result = "30#";
	}
	LeaveCriticalSection(&criticalSection);
	if (result != "") return result;

	return "99#";
}

// slice message to get main message
char *subMessage(char *message, int offset) {
	int i = 0;
	while (message[i + offset]) {
		message[i] = message[i + offset];
		i++;
	}
	message[i] = '\0';
	return message;
}

// defines how the server should process when a user makes a message
char *handle(char* sBuff, session *userSession) {
	if (sBuff[0] == 'U' && sBuff[1] == 'S' && sBuff[2] == 'E' && sBuff[3] == 'R' && sBuff[4] == ' ') {
		char * message;
		message = subMessage(sBuff, 5);
		char* rBuff = login(message, userSession);
		return rBuff;
	}
	if (sBuff[0] == 'P' && sBuff[1] == 'O' && sBuff[2] == 'S' && sBuff[3] == 'T' && sBuff[4] == ' ') {
		char * message;
		message = subMessage(sBuff, 5);
		char* rBuff = post(message, userSession);
		return rBuff;
	}
	if (sBuff[0] == 'B' && sBuff[1] == 'Y' && sBuff[2] == 'E') {
		char * message;
		message = subMessage(sBuff, 3);
		char* rBuff = logout(userSession);
		return rBuff;
	}
	return "99#";
}

// read file into userAccount
int readFile() {
	string temp;
	string account;
	string statusAccount;
	ifstream fileInput("account.txt");
	if (fileInput.fail()) {
		printf("Failed to open this file!\n");
		return 0;
	}
	while (!fileInput.eof())
	{
		char temp[255];
		fileInput.getline(temp, 255);
		string line = temp;
		account = line.substr(0, line.find(" "));
		statusAccount = line.substr(line.find(" ") + 1, line.length());
		userAccount.push_back(make_pair(account, statusAccount));
	}

	fileInput.close();
	return 1;
}


unsigned __stdcall procThread(void *param) {
	DWORD		nEvents = 0;
	DWORD		index;
	SOCKET		socks[WSA_MAXIMUM_WAIT_EVENTS] = {0};
	WSAEVENT	events[WSA_MAXIMUM_WAIT_EVENTS];
	WSANETWORKEVENTS sockEvent;

	char *sendBuff, message[BUFF_SIZE], rcvBuff[BUFF_SIZE];
	SOCKET connSock;
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);
	int ret, i, indexMess = 0;
	int indexClient = *(int *)param;
	int numThread = indexClient / 64;

	while (1) {
		// //Add client socket into socks array
		for (i = indexClient; i < indexClient + 64; i++) {
			int indexSock = i % 64;
			if (clientSession[i].sock != 0 && socks[indexSock] == 0) {
				socks[indexSock] = clientSession[i].sock;
				events[indexSock] = WSACreateEvent();
				WSAEventSelect(socks[indexSock], events[indexSock], FD_READ | FD_CLOSE);
				nEvents++;
			}
		}

		if (socks[0] == 0) {
			continue;
		}

		//wait for network events
		index = WSAWaitForMultipleEvents(nEvents, events, FALSE, 500, FALSE);
		if (index == WSA_WAIT_FAILED) {
			printf("Error %d: WSAWaitForMultipleEvents() failed\n", WSAGetLastError());
			break;
		}

		if (index == WSA_WAIT_TIMEOUT) {
			continue;
		}

		index = index - WSA_WAIT_EVENT_0;
		WSAEnumNetworkEvents(socks[index], events[index], &sockEvent);

		if (sockEvent.lNetworkEvents & FD_READ) {
			//Receive message from client
			if (sockEvent.iErrorCode[FD_READ_BIT] != 0) {
				printf("FD_READ failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
				break;
			}

			ret = recv(socks[index], rcvBuff, BUFF_SIZE, 0);
			if (ret <= 0) {
				closesocket(socks[index]);
				socks[index] = 0;
				WSACloseEvent(events[index]);
				nEvents--;
				printf("Error %d: Cannot receive data.", WSAGetLastError());
				break;
			}

			//Release socket and event if an error occurs
			else {									
				rcvBuff[ret] = 0;
				int indexBuff = 0;
				while (rcvBuff[indexBuff] != '\0') {
					// ENDING_DELIMITER, handle the message
					if (rcvBuff[indexBuff] == '#') {
						message[indexMess] = 0;
						// handle and send message to client
						sendBuff = handle(message, &clientSession[index]);
					
						ret = send(socks[index], sendBuff, strlen(sendBuff), 0);

						if (ret == SOCKET_ERROR) {
							printf("Error %d: Cannot send data.\n", WSAGetLastError());
							break;
						}
						indexMess = 0;
						indexBuff++;
						continue;
					}
					// not ENDING_DELIMITER, copy from rBuff to message
					message[indexMess] = rcvBuff[indexBuff];
					indexMess++;
					indexBuff++;
				}

				//reset event
				WSAResetEvent(events[index]);
			}
		}

		if (sockEvent.lNetworkEvents & FD_CLOSE) {		
			int indexClientClose = numThread * 64 + index; // close client position in clientSession array
			int indexLastClient = numThread * 64 + nEvents -1; // last client of thread in clientSession array
			int i = 0;
			// is the last element, then remove from the array
			if (index == nEvents - 1) {
				// close socket
				closesocket(socks[index]);
				WSACloseEvent(events[index]);
				socks[index] = 0;
				// reset session of client
				EnterCriticalSection(&criticalSection);
				clientSession[indexClientClose].account = "";
				clientSession[indexClientClose].sock = 0;
				clientSession[indexClientClose].clientAddr = {};
				LeaveCriticalSection(&criticalSection);
				nEvents--;
			}
			// is the middle element then remove from the array and swap the last element
			else {
				// close event and swap
				WSACloseEvent(events[index]);
				events[index] = events[nEvents - 1];
				// close socket and swap
				closesocket(socks[index]);
				socks[index] = socks[nEvents - 1];
				socks[nEvents - 1] = 0;
				// swap the last session and reset session
				EnterCriticalSection(&criticalSection);
				clientSession[indexClientClose] = clientSession[indexLastClient];
				clientSession[indexLastClient].account = "";
				clientSession[indexLastClient].sock = 0;
				clientSession[indexLastClient].clientAddr = {};
				LeaveCriticalSection(&criticalSection);

				nEvents--;
			}
		}
	}
	return 0;
}

