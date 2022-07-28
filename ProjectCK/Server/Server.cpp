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
#include <set>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;
/* Session user being created when user login app and status's login = 1
Session user contains all user information
*/
typedef struct session {
	SOCKET sock;
	sockaddr_in clientAddr;
	string account;
	int status;
} session;

/*
Room contains all property status(status), examination time(time) , number of question(numberOfQuestion), id of examination(idOfExam)
the creator of the exam room(admin), the score of each test taker in that exam room(resultOfExam)
*/
typedef struct room {
	string status; // 1: Not yet started, 2: exam in process, 3: Finished
	string time;
	string numberOfQuestion;
	string idOfExam;
	session * admin;
	vector<pair<session *, float>> resultOfExam;
} room;

// Information about one question
typedef struct questionInfo {
	string question;  // question with 3 option to choose
	char result; // result of this question
} questionInfo;

// Information about exam
typedef struct exam {
	string numberOfQuestion; // number of questions in the exam
	vector<questionInfo> questions; // array questions
} exam;

#define BUFF_SIZE 2048
#define SERVER_ADDR "127.0.0.1"
#define MAX_NUM_THREAD 1000
// 1 array session user that use system
session clientSession[1000];

// The user account that has logged into the system
vector<pair<string, string> > userAccount;

// Pointer used to array vector userAccount
vector<pair<string, string> >::iterator iterAccount; 

// List exam room
vector<room> rooms; 

// Array stored question from file contains information about questions
vector<questionInfo> questions; 

// Array contains exams
vector<exam> exams;

// exam questions for practice mode
exam *examPractice;

// Var isCreated used to check if the thread already exists
int isCreated[MAX_NUM_THREAD] = { 0 };

// Function handle convert 1 string to char array
char * convertStringToCharArray(string);

// Handle send data to client
void sendMessage(SOCKET, char *);

// Handle the user's account registration
void registerAccount(string, session *);

// handle when user logs in
void login(string ,session *);

// Handle when user choose mode practice
void practice(string ,session *);

// Handle random question from the bank question
exam *randomQuestion(int);

// handle when user logs out
void logout(session *);

// defines how the server should process when a user makes a message
void handle(char *, session *);

// Choose exam
void choose(string, session *);

// Handle creating exam room for exam
void createRoom(string data, session *);

// Get list room 
void getListRoom(session *);

// Submit the exam
void submit(string data, session *);

// Join the exam room
void join(string data, session *);

// Start an exam room
void start(string data, session *);

// Result of the exam room
void result(string data, session *);

// read file into userAccount
int readFileAccount();

// Read file into questionInfo
int readFileQuestion();

// Read file into rooms
int readFileResult();

/* procThread - Thread to receive the message from client and process*/
unsigned __stdcall procThread(void *);

// Handle counting time
unsigned __stdcall clockThread(void *);

int main(int argc, char* argv[])
{
	DWORD		nEvents = 0;
	DWORD		index;
	SOCKET		listenSock;
	WSAEVENT	eventListen[1];
	WSANETWORKEVENTS sockEvent;

	// Read file to get data 
	readFileAccount();
	readFileQuestion();
	readFileResult();

	/* Initial exam mode practice has 10 questions
		When client turn on app, exam practice get only question from the bank question once
	*/
	examPractice = randomQuestion(10);
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
	serverAddr.sin_port = htons(5500);
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
	while (1) {
		//wait for network events on all socket
		index = WSAWaitForMultipleEvents(1, eventListen, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			printf("Error %d: WSAWaitForMultipleEvents() failed\n", WSAGetLastError());
			break;
		}
		// Define event happen on socket
		WSAEnumNetworkEvents(listenSock, eventListen[0], &sockEvent);

		/*
		Check mask be define on socket and handle
		*/
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
	closesocket(listenSock);
	WSACleanup();
	return 0;
}
char * convertStringToCharArray(string data) {

	// Var i to traverse array
	int i = 0;

	// Create 1 char array to store each character of the data
	char result[10000];

	// While loop to handle copy each charactor of the data info array result
	while (data[i]) {
		result[i] = data[i];
		i++;
	}
	result[i] = 0;
	return result;
}

void sendMessage(SOCKET sock, char *sendBuff) {

	// Send message to client
	int ret = send(sock, sendBuff, strlen(sendBuff), 0);

	// If not send message to client, print error on Server
	if (ret == SOCKET_ERROR) {
		printf("Error %d: Cannot send data.\n", WSAGetLastError());
		return;
	}
}

void registerAccount(string data, session *userSession) {
	// Index of character spacing
	int temp = data.find(' ');

	// Slice data to get user's account
	string user = data.substr(0, temp);

	// Slice data to get user's password
	string password = data.substr(temp + 1);

	// Check if user's account does exist system, send to client message 20 and register failed
	for (iterAccount = userAccount.begin(); iterAccount != userAccount.end(); iterAccount++) {
		if (iterAccount->first == user) {
			sendMessage(userSession->sock, "20#");
			return;
		}
	}

	// Create 1 variable newAccount to store information account user which is taken from the data
	pair<string, string> newAccount;
	newAccount.first = user;
	newAccount.second = password;

	// When account user be created, insert this into array contains all account users
	userAccount.push_back(newAccount);

	// Write File
	ofstream fileOutput("account.txt", ios::app);
	if (fileOutput.fail()) {
		printf("Failed to open this file!\n");
		return;
	}
	fileOutput << user << " " << password << endl;
	fileOutput.close();

	// Send message to client 10 if register succesfully
	sendMessage(userSession->sock, "10#");
	return;
}


void login(string data, session *userSession) {
	// var result will be sent to client
	char *result = "";

	// Index of character spacing 
	int temp = data.find(' ');

	// Slice data to get user's account
	string user = data.substr(0, temp);

	// Slice data to get user's password
	string password = data.substr(temp + 1);

	// Send message to client 24 if account logged
	if (userSession->account != "") {
		result = "24#";
	}
	if (result != "") {
		sendMessage(userSession->sock, result);
		return;
	}

	// check accounts that are already session in elsewhere
	for (int i = 0; i < 1000; i++) {
		if (clientSession[i].account == user) {
			result = "22#";
			break;
		}
	}
	if (result != "") {
		sendMessage(userSession->sock, result);
		return;
	}

	// Validate loggin
	for (iterAccount = userAccount.begin(); iterAccount != userAccount.end(); iterAccount++) {
		if (iterAccount->first == user) {
			// active account
			if (iterAccount->second == password) {// correct password
				userSession->account = iterAccount->first;
				userSession->status = 1;
				sendMessage(userSession->sock, "11#");
				return;
			}
			// Wrong password
			if (iterAccount->second != password) { 
				sendMessage(userSession->sock, "21#");
				return;
			}
		}
	}
	sendMessage(userSession->sock, "23#");
	return;
}

void practice(session *userSession) {

	// Get random 10 questions from the bank question
	vector<questionInfo> questions = examPractice->questions;
	// Handle insert question into message to send to client
	string message = "19 ";
	for (int i = 0; i < questions.size(); i++) {
		message += questions[i].question;
	}
	// Finished handle data of message
	message += "#";

	// Handle streaming message to send client
	int length = message.length(), sentByte = 0;
	char *messBuff = convertStringToCharArray(message);
	int indexMessBuff = 0, indexSBuff = 0;
	char sBuff[2048];
	while (messBuff[indexMessBuff]) {
		// if the character is # then finish processing
		if (messBuff[indexMessBuff] == '#') {
			sBuff[indexSBuff] = messBuff[indexMessBuff];
			sBuff[++indexSBuff] = 0;
			sendMessage(userSession->sock, sBuff);
			break;
		}
		// slicing the message into each buffer
		if (indexMessBuff % 2046 == 0 && indexMessBuff != 0) {
			sBuff[indexSBuff] = messBuff[indexMessBuff];
			sBuff[indexSBuff + 1] = 0;
			sendMessage(userSession->sock, sBuff);
			indexSBuff = 0;
			indexMessBuff++;
			continue;
		}
		// copy message to buffer
		sBuff[indexSBuff++] = messBuff[indexMessBuff++];
	}
	// Finished handle streaming 

}

exam *randomQuestion(int numberOfQuestion) {
	exam *tempExam = new exam;
	set<int> idOfQuestions;
	int maxQuestion = questions.size() - 1;
	// While loop used to get random question from the bank question
	while (idOfQuestions.size() < numberOfQuestion) {
		int idOfQuestion = rand() % maxQuestion;
		idOfQuestions.insert(idOfQuestion);
	}
	// After get random question from the bank question, push into questions of the temp exam 
	for (int i : idOfQuestions) {
		tempExam->questions.push_back(questions[i]);
	}
	return tempExam;
}

void createRoom(string data, session *userSession) {
	// var result will be sent to client
	char *result = "";

	// Index of character spacing 
	int temp = data.find(' ');

	// Slice data to get number of question of the exam room 
	string numberOfQuestion = data.substr(0, temp);

	// Slice data to get time examination of the exam room 
	string time = data.substr(temp + 1);

	// Settings infomation of the exam room and insert this into array rooms
	room *newRoom = new room;
	newRoom->numberOfQuestion = numberOfQuestion;
	newRoom->status = "1";
	newRoom->time = time;
	newRoom->admin = userSession;

	// Randomize questions from the question bank for the exam room
	exam *tempExam = randomQuestion(stoi(numberOfQuestion));

	// Push this temp exam into array exams
	exams.push_back(*tempExam);

	// Set the property ifOfExam newRoom is the index of the newly added element
	newRoom->idOfExam = to_string(exams.size() - 1);

	// Push into array rooms after newRoom being created
	rooms.push_back(*newRoom);

	// Handle the message to the correct protocol format to send to the client
	string message = "15 " + to_string(rooms.size() - 1)+ "#";
	char* sendBuff= convertStringToCharArray(message);
	sendMessage(userSession->sock, sendBuff);
	return;
}

void getListRoom(session *userSession){
	// var result will be sent to client
	string result = "13 "; 

	// handle message to send to client. Client will be recived 1 string
	for (int i = 0; i < rooms.size(); i++) {
		result = result + to_string(i) + " " + rooms[i].status + "/";
	}
	int length = result.length();
	char sendBuff[2048];

	// send message to client
	strncpy_s(sendBuff, result.c_str(), length);
	strcat_s(sendBuff, "#");
	sendMessage(userSession->sock, sendBuff);
}

void submit(string data, session * userSession) {

	// Index of character spacing 
	int temp = data.find(' ');

	// Slice data to get idOfRoom
	int idOfRoom = stoi(data.substr(0, temp));

	// Slice data to get result questions of the client
	string result = data.substr(temp + 1);
	cout << idOfRoom << " " << result << endl;

	// if practice mode
	if (idOfRoom == -1) {
		int indexOfResult = 0;
		int correct = 0;
		// Handle count the number of correct sentences
		while (result[indexOfResult]) {
			if (result[indexOfResult] == examPractice->questions[indexOfResult].result) {
				correct++;
			}
			indexOfResult++;
		}
		// Send points to client and store into the exam room
		string message = "17 ";
		message += to_string(correct) + "#";
		char *sendBuff = convertStringToCharArray(message);
		sendMessage(userSession->sock, sendBuff);
	}
	else {
		// exam mode
		int idOfExam = stoi(rooms[idOfRoom].idOfExam);
		int indexOfResult = 0;
		int correct = 0;
		// Handle count the number of correct sentences
		while (result[indexOfResult]) {
			if (result[indexOfResult] == exams[idOfExam].questions[indexOfResult].result) {
				correct++;
			}
			indexOfResult++;
		}
		int numberOfQuestion = stoi(rooms[idOfRoom].numberOfQuestion);
		float point = (float)correct * 10 / numberOfQuestion;

		// Finished handle

		// Handle scoring for user
		vector<pair<session*, float>> *player = &rooms[idOfRoom].resultOfExam;
		for (int i = 0; i < (*player).size(); i++) {
			if (!(*player)[i].first->account.compare(userSession->account)) {
				(*player)[i].second = point;
				break;
			}
		}
		// Send points to client and store into the exam room
		string message = "17 ";
		message += to_string(point) + "#";
		char *sendBuff = convertStringToCharArray(message);
		sendMessage(userSession->sock, sendBuff);
	}
}

void join(string data, session *userSession) {
	int idOfRoom = stoi(data);

	// Handle message if status room not yet started
	if (rooms[idOfRoom].status == "1") {
		if (rooms[idOfRoom].admin->account.compare(userSession->account)) {
			rooms[idOfRoom].resultOfExam.push_back(make_pair(userSession, -1));
		}
		string message = "14 " + rooms[idOfRoom].numberOfQuestion + " " + rooms[idOfRoom].time +"#";
		char* sendBuff = convertStringToCharArray(message);
		sendMessage(userSession->sock, sendBuff);
	}

	// Handle message if status room in process
	else if (rooms[idOfRoom].status == "2") {
		sendMessage(userSession->sock, "25#");
	}

	// Handle message if status room finished
	else if (rooms[idOfRoom].status == "3") {
		sendMessage(userSession->sock, "26#");
	}
}
void start(string data, session *userSession) {
	int idOfRoom = stoi(data);
	if (!rooms[idOfRoom].admin->account.compare(userSession->account) && rooms[idOfRoom].status == "1") {
		_beginthreadex(0, 0, clockThread, (void *)&rooms[idOfRoom], 0, 0);
		rooms[idOfRoom].status = "2";
		// Send exam questions to room members
		vector <pair<session *, float>> player = rooms[idOfRoom].resultOfExam;
		string message = "16 ";
		int idOfExam = stoi(rooms[idOfRoom].idOfExam);
		vector<questionInfo> questions = exams[idOfExam].questions;

		// join question of exam into message
		for (int i = 0; i < questions.size(); i++) {
			message += questions[i].question;
		}
		message += "#";
		int length = message.length(), sentByte = 0;
		int numberOfSend = length / 2047, sent = 0;
		// Handle streaming
		while (sent<=numberOfSend) {

			//trim the string to 2048 bytes of buffer
			string submess = message.substr(sent * 2048, 2048);
			char *messBuff = convertStringToCharArray(submess);
			int indexMessBuff = 0, indexSBuff = 0;
			char sBuff[2048];

			//Send the exam questions back to all clients in the exam room
			// Handle streaming message to send to client
			while (messBuff[indexMessBuff]) {
				if (messBuff[indexMessBuff] == '#') {
					sBuff[indexSBuff] = messBuff[indexMessBuff];
					sBuff[++indexSBuff] = 0;
					for (int i = 0; i < player.size(); i++) {
						sendMessage(player[i].first->sock, sBuff);
					}
					break;
				}
				if (indexMessBuff % 2046 == 0 && indexMessBuff != 0) {
					sBuff[indexSBuff] = messBuff[indexMessBuff];
					sBuff[indexSBuff + 1] = 0;
					for (int i = 0; i < player.size(); i++) {
						sendMessage(player[i].first->sock, sBuff);
					}
					indexSBuff = 0;
					indexMessBuff++;
					continue;
				}
				sBuff[indexSBuff++] = messBuff[indexMessBuff++];
			}
			// Finished handle streaming
			sent++;
		}

	}
	// Send to message 25 if userSesson's accout not admin room, this user can't start the exam room 
	else if (!rooms[idOfRoom].admin->account.compare(userSession->account) && rooms[idOfRoom].status == "2") {
		sendMessage(userSession->sock, "25#");
	}
	// Send to message 26 if userSesson's accout not admin room, this user can't start the exam room 
	else if (!rooms[idOfRoom].admin->account.compare(userSession->account) && rooms[idOfRoom].status == "3") {
		sendMessage(userSession->sock, "26#");
	}
	else {		
		// Notify non-admin sender
		sendMessage(userSession->sock, "27#");
	}
}

void result(string data, session* userSession) {
	int idOfRoom = stoi(data);

	// if the exam room hasn't started yet
	if (rooms[idOfRoom].status == "1") {
		sendMessage(userSession->sock, "28#");
	}

	//if the exam room is in process
	else if (rooms[idOfRoom].status == "2") {
		sendMessage(userSession->sock, "25#");
	}

	// if the exam room is finish
	else if (rooms[idOfRoom].status == "3") {
		string message = "18 ";

		// Get all members in the exam room
		vector <pair<session *, float>> player = rooms[idOfRoom].resultOfExam;
		for (int i = 0; i < player.size(); i++) {
			message += player[i].first->account + " " + to_string(player[i].second) + "/";
		}
		message += "#";
		// Finished handle

		// Handle their user information and points into the message
		int length = message.length(), sentByte = 0;
		char *messBuff = convertStringToCharArray(message);
		int indexMessBuff = 0, indexSBuff = 0;
		char sBuff[2048];

		// Handle streaming
		while (messBuff[indexMessBuff]) {
			if (messBuff[indexMessBuff] == '#') {
				sBuff[indexSBuff] = messBuff[indexMessBuff];
				sBuff[++indexSBuff] = 0;
				sendMessage(userSession->sock, sBuff);
				break;
			}
			if (indexMessBuff % 2046 == 0 && indexMessBuff != 0) {
				sBuff[indexSBuff] = messBuff[indexMessBuff];
				sBuff[indexSBuff + 1] = 0;
				sendMessage(userSession->sock, sBuff);
				indexSBuff = 0;
				indexMessBuff++;
				continue;
			}
			sBuff[indexSBuff++] = messBuff[indexMessBuff++];
		}
		// Finished streaming
	}

}

// handle when user logs out
void logout(session *userSession) {
	// var result will be sent to client
	char *result = "";
	// The current session is not session in
	if (userSession->account == "") {
		result = "21#";
	}
	else {
		// Log out of the current session
		userSession->account = "";
		userSession->status = 0;
		result = "12#";
	}
	if (result != "") {
		sendMessage(userSession->sock, result);
		return;
	}

	sendMessage(userSession->sock, "99#");
	return;
}

// defines how the server should process when a user makes a message
void handle(char* sBuff, session *userSession) {
	int temp = string(sBuff).find(' ');
	string requestMessageType = string(sBuff).substr(0, temp);
	string data = string(sBuff).substr(temp + 1);
	if (requestMessageType == "LOGIN") {
		login(data, userSession);
	} else
	if (requestMessageType == "REGISTER") {
		registerAccount(data, userSession);
	} else
	if (requestMessageType == "PRACTICE") {
		practice(userSession);
	} else
	if (requestMessageType == "CREATEROOM") {
		createRoom(data, userSession);
	} else
	if (requestMessageType == "LISTROOM") {
		getListRoom(userSession);
	} else
	if (requestMessageType == "JOIN") {
		join(data, userSession);
	} else
	if (requestMessageType == "RESULT") {
		result(data, userSession);
	} else
	if (requestMessageType == "SUBMIT") {
		submit(data, userSession);
	} else
	if (requestMessageType == "START") {
		start(data, userSession);
	} else 
	if(requestMessageType == "LOGOUT"){
		logout(userSession);
	}
	else {
		sendMessage(userSession->sock, "99#");
	}
	return;
}

// read file into userAccount
int readFileAccount() {
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

int readFileQuestion() {
	ifstream fileInput("Question.txt");
	if (fileInput.fail()) {
		printf("Failed to open this file!\n");
		return 0;
	}
	questionInfo * tempQuestion = new questionInfo();
	int i = 0;
	// handle file question to format question to send to client
	while (!fileInput.eof())
	{
		char temp[2048];
		fileInput.getline(temp, 2048);
		if (i == 5) {
			i = 0;
			continue;
		} else
		if (i == 4) {
			char result = temp[0];
			tempQuestion->result = (char)result;
			questions.push_back(*tempQuestion);
			tempQuestion = new questionInfo();
			i++;
		} else
		if (i == 3) {
			string line = temp;
			tempQuestion->question += line;
			tempQuestion->question += "/";
			i++;
			continue;
		}
		else {
			string line = temp;
			tempQuestion->question += line;
			tempQuestion->question += "/";
			i++;
		}
	}
	// Finished handle

	fileInput.close();
}

int readFileResult() {
	string account;
	float resultOfExam;
	room *tempRoom;
	ifstream fileInput("Rooms.txt");
	if (fileInput.fail()) {
		printf("Failed to open this file!\n");
		return 0;
	}
	// handle file result to format result to send to client
	while (!fileInput.eof())
	{
		char temp[255];
		fileInput.getline(temp, 255);
		string line = temp;
		if (line == "{") {
			tempRoom = new room;
			continue;
		}
		if (line == "}") {
			rooms.push_back(*tempRoom);
			continue;
		}
		account = line.substr(0, line.find(" "));
		resultOfExam = stof(line.substr(line.find(" ") + 1, line.length()));
		session * tempSession = new session;
		tempSession->account = account;
		tempRoom->resultOfExam.push_back(make_pair(tempSession, resultOfExam));
		tempRoom->status = "3";
	}

	fileInput.close();
	return 0;
}

unsigned __stdcall procThread(void *param) {
	DWORD		nEvents = 0;
	DWORD		index;
	SOCKET		socks[WSA_MAXIMUM_WAIT_EVENTS] = { 0 };
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
			// Remove session client if server dont receive message from client
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
				//echo to client
				rcvBuff[ret] = 0;
				int indexBuff = 0;
				// Handle process stream data receive from client
				while (rcvBuff[indexBuff] != '\0') {
					// ENDING_DELIMITER, handle the message
					if (rcvBuff[indexBuff] == '#') {
						message[indexMess] = 0;
						// handle and send message to client
						handle(message, &clientSession[index]);
						indexMess = 0;
						indexBuff++;
						continue;
					}
					// not ENDING_DELIMITER, copy from rBuff to message
					message[indexMess] = rcvBuff[indexBuff];
					indexMess++;
					indexBuff++;
				}
				// Finished handle streaming message
				//reset event
				WSAResetEvent(events[index]);
			}
		}

		if (sockEvent.lNetworkEvents & FD_CLOSE) {
			int indexClientClose = numThread * 64 + index; // close client position in clientSession array
			int indexLastClient = numThread * 64 + nEvents - 1; // last client of thread in clientSession array
			int i = 0;
			// is the last element, then remove from the array
			if (index == nEvents - 1) {
				// close socket
				closesocket(socks[index]);
				WSACloseEvent(events[index]);
				socks[index] = 0;
				// reset session of client
				clientSession[indexClientClose].account = "";
				clientSession[indexClientClose].sock = 0;
				clientSession[indexClientClose].clientAddr = {};
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
				clientSession[indexClientClose] = clientSession[indexLastClient];
				clientSession[indexLastClient].account = "";
				clientSession[indexLastClient].sock = 0;
				clientSession[indexLastClient].clientAddr = {};

				nEvents--;
			}
		}
	}
	return 0;
}

unsigned __stdcall clockThread(void *examRoom) {
	room *presentRoom = (room *)examRoom;
	int time = stoi(presentRoom->time);
	int second = time * 60;

	// Handle case of time doing lessons
	Sleep(second * 1000);
	vector<pair<session *, float>> player = presentRoom->resultOfExam;

	// required to submit assignments
	for (int i = 0; i < player.size(); i++) {
		// If client not have points, send a request
		if (player[i].second == -1) {
			sendMessage(player[i].first->sock, "SUBMITNOTIFICATION#");
		}
	}

	// Set exam room status as finished
	presentRoom->status = "3";
	Sleep(10000);

	// Record the exam results of the room in file
	ofstream fileOutput("Rooms.txt", ios::app);
	if (fileOutput.fail()) {
		printf("Failed to open this file!\n");
		return 1;
	}
	fileOutput << endl;
	fileOutput << "{" << endl;
	for (int i = 0; i < player.size(); i++) {
		fileOutput << player[i].first->account << " " << player[i].second << endl;
	}
	fileOutput << "}";
	return 0;
};

