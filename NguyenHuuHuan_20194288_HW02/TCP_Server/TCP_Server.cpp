#include "stdio.h"
#include "conio.h"
#include "string.h"
#include "ws2tcpip.h"
#include "winsock2.h"
#include "process.h"
#include "vector"
#include "fstream"
#include "string"
#include "list"
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#define ENDING_DELIMITER "#"
CRITICAL_SECTION criticalSection;
#pragma comment(lib, "Ws2_32.lib")
using namespace std;
typedef struct logged {
	SOCKET sock;
	sockaddr_in clientAddr;
	string account;
} logged;
vector<pair<string, string> > userAccount; // vector to save data in file
vector<pair<string, string> >::iterator iterAccount;
list<logged> loggedAccount; // list to save session
list<logged> ::iterator iterLogged;

// handle when user logs in
char *login(char *, logged *);

// handle when user posts
char *post(char *, logged *);

// handle when user logs out
char *logout(logged *);

// slice message to get main message
char *subMessage(char *, int );

// defines how the server should process when a user makes a message
char * handle(char *, logged *); 

// read file into userAccount
int readFile();

/* procThread - Thread to receive the message from client and echo*/
unsigned __stdcall procThread(void * );

void communicateWithClient(SOCKET);

int main(int argc, char* argv[])
{
	readFile();
	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Step 2: Construct socket	
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
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

	//Step 5: Communicate with client
	while (1) {
		communicateWithClient(listenSock);
	}

	closesocket(listenSock);
	WSACleanup();
	return 0;
}

// handle when user logs in
char *login(char *message, logged *session) {
	char * result = "";

	// The current session is logged in
	if (session->account != "") {
		return "14#";
	}

	// check accounts that are already logged in elsewhere
	EnterCriticalSection(&criticalSection);
	for (iterLogged = loggedAccount.begin(); iterLogged != loggedAccount.end(); iterLogged++) {
		if (iterLogged->account == (string)message) {
			result = "13#";
		}
	}
	LeaveCriticalSection(&criticalSection);
	if (result != "") return result;
	// Validate loggin
	for (iterAccount = userAccount.begin(); iterAccount != userAccount.end(); iterAccount++) {
		if (iterAccount->first == (string)message) {
			// active account
			if (iterAccount->second == "0") {
				session->account = iterAccount->first;
				EnterCriticalSection(&criticalSection);
				loggedAccount.push_back(*session);
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
char *post(char *message, logged *session) {
	if (session->account == "") {
		return "21#";
	}
	return "20#";
}

// handle when user logs out
char *logout(logged *session) {
	char *result = "";

	// The current session is not logged in
	if (session->account == "") {
		return "21#";
	}

	// Log out of the current session
	EnterCriticalSection(&criticalSection);
	for (iterLogged = loggedAccount.begin(); iterLogged != loggedAccount.end(); iterLogged++) {
		if (iterLogged->sock == session->sock && iterLogged->clientAddr.sin_addr.S_un.S_addr == session->clientAddr.sin_addr.S_un.S_addr
			&& iterLogged->clientAddr.sin_port == session->clientAddr.sin_port) {
			session->account = "";
			loggedAccount.erase(iterLogged);
			result = "30#";
			break;
		}
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
char *handle(char* sBuff, logged *session) {
	if (sBuff[0] == 'U' && sBuff[1] == 'S' && sBuff[2] == 'E' && sBuff[3] == 'R' && sBuff[4] == ' ') {
		char * message;
		message = subMessage(sBuff, 5);
		char* rBuff = login(message, session);
		return rBuff;
	}
	if (sBuff[0] == 'P' && sBuff[1] == 'O' && sBuff[2] == 'S' && sBuff[3] == 'T' && sBuff[4] == ' ') {
		char * message;
		message = subMessage(sBuff, 5);
		char* rBuff = post(message, session);
		return rBuff;
	}
	if (sBuff[0] == 'B' && sBuff[1] == 'Y' && sBuff[2] == 'E') {
		char * message;
		message = subMessage(sBuff, 3);
		char* rBuff = logout(session);
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

/* procThread - Thread to receive the message from client and echo*/
unsigned __stdcall procThread(void * param) {
	char rBuff[BUFF_SIZE];
	char message[BUFF_SIZE]; char* sBuff;
	int indexMess = 0;
	int ret;
	logged* session = (logged *)param;
	SOCKET connectedSocket = (SOCKET)session->sock;
	while (1) {
		ret = recv(connectedSocket, rBuff, BUFF_SIZE, 0);
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot receive data.\n", WSAGetLastError());
			break;
		}
		else if (ret == 0) {
			printf("Client disconnects.\n");
			break;
		}
		else if (strlen(rBuff) > 0) {
			rBuff[ret] = 0;
			// process stream
			int indexBuff = 0;
			while (rBuff[indexBuff] != '\0') {
				// ENDING_DELIMITER, handle the message
				if (rBuff[indexBuff] == '#') { 
					message[indexMess] = 0;
					// handle and send message to client
					sBuff = handle(message, session);
					ret = send(connectedSocket, sBuff, strlen(sBuff), 0);
					if (ret == SOCKET_ERROR) {
						printf("Error %d: Cannot send data.\n", WSAGetLastError());
						break;
					}
					indexMess = 0;
					indexBuff++;
					continue;
				}
				// not ENDING_DELIMITER, copy from rBuff to message
				message[indexMess] = rBuff[indexBuff];
				indexMess++;
				indexBuff++;
			}
		}
	}
	// end the thread then logout the session
	logout(session);
	closesocket(connectedSocket);
	return 0;
}

// communicate with client
void communicateWithClient(SOCKET listenSock) {
	SOCKET connSock;
	sockaddr_in clientAddr;
	char clientIP[INET_ADDRSTRLEN];
	int clientAddrLen = sizeof(clientAddr), clientPort;
	InitializeCriticalSection(&criticalSection);
	while (1) {
		connSock = accept(listenSock, (sockaddr *)& clientAddr, &clientAddrLen);
		if (connSock == SOCKET_ERROR) {
			printf("Error %d: Cannot permit incoming connection.\n", WSAGetLastError());
			return;
		}
		else {
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
			clientPort = ntohs(clientAddr.sin_port);
			printf("Accept incoming connection from %s:%d\n", clientIP, clientPort);
			// session initialization and pass session to thread
			logged *session = new logged;
			session->clientAddr.sin_addr = clientAddr.sin_addr;
			session->clientAddr.sin_port = clientAddr.sin_port;
			session->sock = connSock;
			_beginthreadex(0, 0, procThread, (void *)session, 0, 0); //start thread
		}
	}
	DeleteCriticalSection(&criticalSection);
}