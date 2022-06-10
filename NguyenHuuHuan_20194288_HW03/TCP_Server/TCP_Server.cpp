#include "stdafx.h"
#include <stdio.h>
#include <conio.h>
#include <string>
#include <vector>
#include <fstream>
#include "process.h"
#include <iostream>
#define FD_SETSIZE 1024
#include <winsock2.h>

#pragma comment (lib,"ws2_32.lib")

#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#define MAX_NUM_THREAD 10
CRITICAL_SECTION criticalSection;
using namespace std;
typedef struct session {
	SOCKET sock;
	sockaddr_in clientAddr;
	string account;
} session;
// client[i] has session[i]
session clientSession[10000];
SOCKET client[10000] = {0};
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

// when clients close the connection, remove the session from the session array
void resetSession(int i);

int main(int argc, char* argv[])
{
	readFile();
	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	//Step 2: Construct socket	
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error! Cannot bind this address.");
		_getch();
		return 0;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 1000)) {
		printf("Error! Cannot listen.");
		_getch();
		return 0;
	}

	printf("Server started!\n");

	SOCKET connSock;
	sockaddr_in clientAddr;
	int ret, nEvents, clientAddrLen;
	fd_set initfds, readfds; //use initfds to initiate readfds at the begining of every loop step
	

	FD_ZERO(&initfds);
	FD_SET(listenSock, &initfds);

	//Step 5: Communicate with clients
	InitializeCriticalSection(&criticalSection);
	while (1) {
		readfds = initfds;		/* structure assignment */
		nEvents = select(0, &readfds, 0, 0, 0);
		if (nEvents < 0) {
			printf("\nError! Cannot poll sockets: %d", WSAGetLastError());
			break;
		}

		//new client connection
		if (FD_ISSET(listenSock, &readfds)) {
			clientAddrLen = sizeof(clientAddr);
			if ((connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen)) < 0) {
				printf("\nError! Cannot accept new connection: %d", WSAGetLastError());
				break;
			}
			else {
				printf("You got a connection from %s\n", inet_ntoa(clientAddr.sin_addr)); /* prints client's IP */

				// when has a connSock, initialize session and add to session array
				int i;
				for (i = 0; i < 10000; i++)
					if (client[i] == 0) {
						session *tempSession = new session;
						// set session parameters
						tempSession->clientAddr = clientAddr;
						tempSession->sock = connSock;
						client[i] = connSock;
						// add to session array
						EnterCriticalSection(&criticalSection);
						clientSession[i] = *tempSession;
						LeaveCriticalSection(&criticalSection);
						break;
					}
				// if i is a multiple of 1024 and the thread has not been created then create a new thread
				int numThread = i / 1024;
				if (i % 1024 == 0 && !isCreated[numThread]) {
					_beginthreadex(0, 0, procThread, (void *)&i, 0, 0);
				}

				if (--nEvents == 0)
					continue; //no more event
			}
		}
	}
	DeleteCriticalSection(&criticalSection);
	closesocket(listenSock);
	WSACleanup();
	return 0;
}

/* The processData function copies the input string to output
* @param in Pointer to input string
* @param out Pointer to output string
* @return No return value
*/

char *login(char *message, session *userSession) {
	char *result ="";
	// The current session is session in
	EnterCriticalSection(&criticalSection);
	if (userSession->account != "") {
		result = "14#";
	}
	LeaveCriticalSection(&criticalSection);
	if (result != "") return result;
	// check accounts that are already session in elsewhere
	EnterCriticalSection(&criticalSection);
	for (int i = 0; i < 10000; i++) {
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
	ifstream fileInput("../TCP_Server/account.txt");
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

// when clients close the connection, remove the session from the session array
void resetSession(int i) {
	client[i] = 0;
	closesocket(client[i]);
	EnterCriticalSection(&criticalSection);
	clientSession[i].clientAddr = {};
	clientSession[i].sock = 0;
	clientSession[i].account = "";
	LeaveCriticalSection(&criticalSection);
}

/* procThread - Thread to receive the message from client and process*/
unsigned __stdcall procThread(void * param) {
	int ret, nEvents, indexMess = 0;
	char *sendBuff, message[BUFF_SIZE], rcvBuff[BUFF_SIZE];
	int indexClient = *(int *)param;
	fd_set readfds;
	timeval timeout;  // time out for select function
	timeout.tv_usec = 10;
	FD_ZERO(&readfds);
	//receive data from clients
	while (1) {
		// copy client[i] non-zero into readfds
		for (int i = indexClient; i < indexClient + 1024; i++) {
			if (client[i]) {
				FD_SET(client[i], &readfds);
			}
		}	
		// when readfds is empty, thread ends
		if (readfds.fd_count == 0) {
			break;
		}

		nEvents = select(0, &readfds, 0, 0, &timeout);
		if (nEvents < 0) {
			printf("\nError! Cannot poll sockets: %d", WSAGetLastError());
			break;
		}

		for (int i = indexClient; i < indexClient + 1024; i++) {
			if (client[i] == 0)
				continue;
			// if the connection has data
			if (FD_ISSET(client[i], &readfds)) {
				ret = recv(client[i], rcvBuff, BUFF_SIZE, 0);
				if (ret == SOCKET_ERROR) {
					FD_CLR(client[i], &readfds);
					resetSession(i);
					break;
				}
				if (ret <= 0) {
					FD_CLR(client[i], &readfds);
					resetSession(i);
					break;
				}
				else if (ret > 0) {
					rcvBuff[ret] = 0;
					int indexBuff = 0;
					while (rcvBuff[indexBuff] != '\0') {
						// ENDING_DELIMITER, handle the message
						if (rcvBuff[indexBuff] == '#') {
							message[indexMess] = 0;
							// handle and send message to client
							
							sendBuff = handle(message, &clientSession[i]);
							ret = send(client[i], sendBuff, strlen(sendBuff), 0);

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
				}
			}
			if (--nEvents <= 0)
				continue; //no more event
		}
	}
	// mark thread not created
	int numThread = indexClient / 1024;
	isCreated[numThread] = 0;
}



