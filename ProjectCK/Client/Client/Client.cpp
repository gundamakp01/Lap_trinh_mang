// Win32Project2.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "client.h"
#include "windows.h"
#include "string"
#include "ws2tcpip.h"
#include "winsock2.h"
#include "string.h"
#include <vector>
#include "process.h"
using namespace std;

#define WM_SOCKET WM_USER + 1
#define SERVER_PORT 5500
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2047
#define ENDING_DELEMETER "#"
#pragma comment(lib, "Ws2_32.lib")

#define BACK 100
#define ANSWER_A 'A'
#define ANSWER_B 'B'
#define ANSWER_C 'C'
#define VIEW_LOGIN 1
#define VIEW_REGISTER 2
#define VIEW_LIST_ROOM 3
#define LOGIN 4
#define REGISTER 5
#define LOGOUT 6
#define VIEW_QUESTION 7
#define SUBMIT 8
#define GET_ID 9
#define JOIN 10
#define VIEW_CREATE_ROOM 11
#define CREATE_ROOM 12
#define START 13
#define RESULT 14
#define PRACTICE 15
#define REFRESH 16


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK View1Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LoginProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RegisterProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK View2Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ListRoomProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CreateRoomProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RoomProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QuestionProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ResultProc(HWND, UINT, WPARAM, LPARAM);

void CreateView1(HWND);
void CreateViewLogin(HWND);
void CreateViewRegister(HWND);
void CreateView2(HWND);
void CreateViewListRoom(HWND);
void ViewCreateRoom(HWND);
void CreateViewRoom(HWND);
void CreateViewQuestion(HWND);
void CreateViewResult(HWND, vector<pair<string, string>> players);

void Send(SOCKET s, char *sBuff, int size, int flags);
void Receive(SOCKET s, string &rBuffStr);
void login(char *username, char *password);
void Register(char *username, char *password);
void logout();
void listRoom();
void join(string roomID);
void createRoom(char *numOfQuestions, char *time);
void start(string roomID);
void submit();
void result(string roomID);
void practice();
void processData(string message);

HWND hWnd;
HWND hWndNow;
HWND hUsername;
HWND hPassword;
HWND hConfirmPass;
HWND hListRoom;
HWND hRoomID;
HWND hJoin;
HWND hNumOfQuestions;
HWND hTime;
HWND hResult;
HWND hQuestion;
HWND hAnswerA;
HWND hAnswerB;
HWND hAnswerC;
HWND hSubmit;

string questions;// series of questions and answers separated by "/" which received from server. For example: Question 1:.../A:.../B:.../C:.../Question 2:.../
int indexQuestions = 0;
char answer[BUFF_SIZE];// Answer of the user which would be sent to server
string idOfRoom;
SOCKET client;

// Exam room include id, status, number of question and exam time
typedef struct Room
{
	string id;
	string status;
	string numOfQuestions;
	string time;
} Room;
Room rooms[1000];

// Save data which received from server
string rBuffStr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//Registering the Window Class
	WNDCLASSW wc = { 0 };
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hInstance = hInstance;
	wc.lpszClassName = L"myWindowClass";
	wc.lpfnWndProc = WndProc;
	if (!RegisterClassW(&wc)) {
		return -1;
	}
	//Create the window
	hWnd = CreateWindowW(L"myWindowClass", L"My Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 800, 400, NULL, NULL, NULL, NULL);

	//Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		MessageBox(hWnd, L"Winsock 2.2 is not supported.", L"Error!", MB_OK);
		return 0;
	}

	//Construct socket
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		MessageBox(hWnd, L"Error: Cannot construct client socket.", L"Error!", MB_OK);
		return 0;
	}

	//Specify server socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	//Request to connect server
	if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		MessageBox(hWnd, L"Error: Cannot connect server.", L"Error!", MB_OK);
		return 0;
	}
	WSAAsyncSelect(client, hWnd, WM_SOCKET, FD_READ | FD_CLOSE);
	MessageBox(hWnd, L"Connected server!", L"Information", MB_ICONINFORMATION);

	// Main message loop:
	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SOCKET:
	{
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
		{
			Receive(client, rBuffStr); //Receive data from server and append to string "rBuffStr"
			// Process streaming
			int temp = rBuffStr.find(ENDING_DELEMETER);
			if (temp != -1) { // If ENDING_DELEMETER is found, split string "rBuffStr" to get the string before ENDING_DELEMETER then pass to processData(), the remaining is assgined to rBuffStr  
				string message = rBuffStr.substr(0, temp);
				processData(message);
				rBuffStr = rBuffStr.substr(temp + 1);
			}
			break;
		}

		case FD_CLOSE:
		{
			//Close socket
			closesocket(client);
			break;
		}

		}
		break;
	}

	case WM_CREATE:
		CreateView1(hwnd);
		break;
	case WM_DESTROY:
		WSACleanup();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// Create window include login and register button
void CreateView1(HWND hwnd) {
	WNDCLASSW swc = { 0 };
	swc.lpszClassName = L"mySubWindow";
	swc.lpfnWndProc = View1Proc;
	RegisterClassW(&swc);
	hWndNow = CreateWindowW(L"mySubWindow", L"", WS_VISIBLE | WS_CHILD, 0, 0, 800, 800, hwnd, NULL, NULL, NULL);
	CreateWindowW(L"button", L"Login", WS_VISIBLE | WS_CHILD, 0, 0, 100, 50, hWndNow, (HMENU)VIEW_LOGIN, NULL, NULL);
	CreateWindowW(L"button", L"Register", WS_VISIBLE | WS_CHILD, 0, 50, 100, 50, hWndNow, (HMENU)VIEW_REGISTER, NULL, NULL);
}

// Create new window when user click to button "Login" and "Register" 
LRESULT CALLBACK View1Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case VIEW_LOGIN:
			DestroyWindow(hwnd);
			CreateViewLogin(hWnd);
			break;
		case VIEW_REGISTER:
			DestroyWindow(hwnd);
			CreateViewRegister(hWnd);
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// Create login window for the user to enter username and password 
void CreateViewLogin(HWND hwnd) {
	WNDCLASSW swc = { 0 };
	swc.lpszClassName = L"ViewLogin";
	swc.lpfnWndProc = LoginProc;
	RegisterClassW(&swc);
	hWndNow = CreateWindowW(L"ViewLogin", L"", WS_VISIBLE | WS_CHILD, 0, 0, 800, 800, hwnd, NULL, NULL, NULL);
	CreateWindowW(L"static", L"Username", WS_VISIBLE | WS_CHILD, 0, 0, 80, 50, hWndNow, NULL, NULL, NULL);
	hUsername = CreateWindowW(L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE, 100, 0, 100, 50, hWndNow, NULL, NULL, NULL);
	CreateWindowW(L"static", L"Password", WS_VISIBLE | WS_CHILD, 0, 60, 80, 50, hWndNow, NULL, NULL, NULL);
	hPassword = CreateWindowW(L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 100, 60, 100, 50, hWndNow, NULL, NULL, NULL);
	CreateWindowW(L"button", L"Login", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 120, 50, 50, hWndNow, (HMENU)LOGIN, NULL, NULL);
	CreateWindowW(L"button", L"Back", WS_VISIBLE | WS_CHILD | WS_BORDER, 152, 120, 50, 50, hWndNow, (HMENU)BACK, NULL, NULL);
}

// Process when user click to button "Login"
LRESULT CALLBACK LoginProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case LOGIN: // Get username and password from input of user and call function login()
		{
			wchar_t usernameW[100];
			GetWindowTextW(hUsername, usernameW, 100);// Get username from input
			char username[100];
			wcstombs(username, usernameW, 100); // Cast wchar_t to char
												// Error message if user leaves username blank
			if (strcmp(username, "") == 0) {
				MessageBox(hWnd, L"Please enter your username.", L"Error!", MB_ICONERROR);
				break;
			}
			wchar_t passwordW[100];
			GetWindowTextW(hPassword, passwordW, 100);// Get password from input
			char password[100];
			wcstombs(password, passwordW, 100);
			// Error message if user leaves password blank
			if (strcmp(password, "") == 0) {
				MessageBox(hWnd, L"Please enter your password.", L"Error!", MB_ICONERROR);
				break;
			}
			else {
				login(username, password);
			}
			break;
		}

		case BACK:
			DestroyWindow(hwnd);
			CreateView1(hWnd);
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// Create register window for the user to register account, include username, password and confirm password
void CreateViewRegister(HWND hwnd) {
	WNDCLASSW swc = { 0 };
	swc.lpszClassName = L"ViewRegister";
	swc.lpfnWndProc = RegisterProc;
	RegisterClassW(&swc);
	hWndNow = CreateWindowW(L"ViewRegister", L"", WS_VISIBLE | WS_CHILD, 0, 0, 800, 800, hwnd, NULL, NULL, NULL);
	hUsername = CreateWindowW(L"edit", L"username", WS_VISIBLE | WS_CHILD | WS_BORDER, 0, 0, 100, 50, hWndNow, NULL, NULL, NULL);
	hPassword = CreateWindowW(L"edit", L"password", WS_VISIBLE | WS_CHILD | WS_BORDER, 0, 50, 100, 50, hWndNow, NULL, NULL, NULL);
	hConfirmPass = CreateWindowW(L"edit", L"confirmPassword", WS_VISIBLE | WS_CHILD | WS_BORDER, 0, 100, 100, 50, hWndNow, NULL, NULL, NULL);
	CreateWindowW(L"button", L"Register", WS_VISIBLE | WS_CHILD | WS_BORDER, 0, 150, 50, 50, hWndNow, (HMENU)REGISTER, NULL, NULL);
	CreateWindowW(L"button", L"Back", WS_VISIBLE | WS_CHILD | WS_BORDER, 52, 150, 50, 50, hWndNow, (HMENU)BACK, NULL, NULL);
}

// Process when user click to button "Register"
LRESULT CALLBACK RegisterProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case REGISTER: // Get username, password and confirm password from input of user and call function login()
		{
			wchar_t usernameW[100];
			GetWindowTextW(hUsername, usernameW, 100);// Get username from input
			char username[100];
			wcstombs(username, usernameW, 100);
			// Error message if user leaves username blank
			if (strcmp(username, "") == 0) {
				MessageBox(hWnd, L"Please enter your username.", L"Error!", MB_OK);
				break;
			}
			wchar_t passwordW[100];
			GetWindowTextW(hPassword, passwordW, 100);// Get password from input
			char password[100];
			// Error message if user leaves password blank
			wcstombs(password, passwordW, 100);
			if (strcmp(password, "") == 0) {
				MessageBox(hWnd, L"Please enter your password.", L"Error!", MB_OK);
				break;
			}
			wchar_t confirmPassW[100];
			GetWindowTextW(hConfirmPass, confirmPassW, 100);// Get confirm password from input
			char confirmPass[100];
			wcstombs(confirmPass, confirmPassW, 100);
			// Error message if user leaves confirm password blank
			if (strcmp(confirmPass, "") == 0) {
				MessageBox(hWnd, L"Please confirm your password.", L"Error!", MB_OK);
				break;
			}
			// Error message if password and confirm password do not match.
			if (strcmp(password, confirmPass) != 0) {
				int result = MessageBoxW(NULL, L"Password and confirm password does not match", L"Error!", MB_OK | MB_ICONERROR);
				SetWindowText(hConfirmPass, L"");
			}
			else
			{
				Register(username, password);
			}
			break;
		}

		case BACK:
			DestroyWindow(hwnd);
			CreateView1(hWnd);
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// Create window include button "List Room", "Practice" and "Logout"
void CreateView2(HWND hwnd) {
	WNDCLASSW swc = { 0 };
	swc.lpszClassName = L"View2";
	swc.lpfnWndProc = View2Proc;
	RegisterClassW(&swc);
	hWndNow = CreateWindowW(L"View2", L"", WS_VISIBLE | WS_CHILD, 0, 0, 800, 800, hwnd, NULL, NULL, NULL);
	CreateWindowW(L"button", L"List Room", WS_VISIBLE | WS_CHILD, 0, 0, 100, 50, hWndNow, (HMENU)VIEW_LIST_ROOM, NULL, NULL);
	CreateWindowW(L"button", L"Practice", WS_VISIBLE | WS_CHILD, 0, 50, 100, 50, hWndNow, (HMENU)PRACTICE, NULL, NULL);
	CreateWindowW(L"button", L"Logout", WS_VISIBLE | WS_CHILD, 0, 100, 100, 50, hWndNow, (HMENU)LOGOUT, NULL, NULL);
}

// Process when the user click to button "List room", "Practice" and "Logout"
LRESULT CALLBACK View2Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case VIEW_LIST_ROOM:
			listRoom();
			break;

		case PRACTICE:
			practice();

			break;
		case LOGOUT:
			logout();
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// Create window which show list of exam rooms, button "Refresh", "Join", "Result", "Create new room"
void CreateViewListRoom(HWND hwnd) {
	WNDCLASSW swc = { 0 };
	swc.lpszClassName = L"ViewListRoom";
	swc.lpfnWndProc = ListRoomProc;
	RegisterClassW(&swc);
	hWndNow = CreateWindowW(L"ViewListRoom", L"", WS_VISIBLE | WS_CHILD, 0, 0, 800, 800, hwnd, NULL, NULL, NULL);
}

// Process when user click to button "Refresh", "Join", "Result", "Create new room"
LRESULT CALLBACK ListRoomProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		hListRoom = CreateWindowW(L"listbox", NULL, WS_CHILD | WS_VISIBLE | LBS_NOTIFY, 10, 10, 150, 200, hwnd, (HMENU)GET_ID, NULL, NULL);
		hRoomID = CreateWindowW(L"static", L"", WS_CHILD, 500, 300, 100, 45, hwnd, NULL, NULL, NULL);
		CreateWindowW(L"button", L"Refresh", WS_CHILD | WS_VISIBLE, 170, 10, 100, 45, hwnd, (HMENU)REFRESH, NULL, NULL);
		hJoin = CreateWindowW(L"Button", L"Join", WS_CHILD | WS_VISIBLE, 170, 55, 100, 45, hwnd, (HMENU)JOIN, NULL, NULL);
		hResult = CreateWindowW(L"Button", L"Result", WS_CHILD | WS_VISIBLE, 170, 100, 100, 45, hwnd, (HMENU)RESULT, NULL, NULL);
		CreateWindowW(L"Button", L"Back", WS_CHILD | WS_VISIBLE, 170, 145, 100, 45, hwnd, (HMENU)BACK, NULL, NULL);

		CreateWindowW(L"Button", L"Create new room", WS_CHILD | WS_VISIBLE, 10, 220, 150, 45, hwnd, (HMENU)VIEW_CREATE_ROOM, NULL, NULL);

		for (int i = 0; i < ARRAYSIZE(rooms); i++)
		{
			if (rooms[i].id == "") {
				break;
			}
			string status;
			if (rooms[i].status == "1") {
				status = "Not yet started";
			}
			else if (rooms[i].status == "2") {
				status = "In process";
			}
			else {
				status = "Finished";
			}

			string content = "Room" + rooms[i].id + ": " + status;
			wstring contentW = wstring(content.begin(), content.end());
			const wchar_t* roomW = contentW.c_str();
			SendMessage(hListRoom, LB_ADDSTRING, 0, (LPARAM)roomW);
		}
	case WM_COMMAND:
		if (LOWORD(wParam) == GET_ID)
		{
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				int i = (int)SendMessage(hListRoom, LB_GETCURSEL, 0, 0);
				string content = rooms[i].id;
				wstring contentW = wstring(content.begin(), content.end());
				const wchar_t* roomW = contentW.c_str();
				SetWindowText(hRoomID, roomW);
			}
		}
		switch (wParam)
		{
		case JOIN: // Get roomID and call function join(roomID)
		{
			wchar_t roomIDW[10];
			GetWindowTextW(hRoomID, roomIDW, 10);
			char roomID[10];
			wcstombs(roomID, roomIDW, 10);
			// Notify if the user haven't choose the room yet.
			if (strcmp(roomID, "") == 0) {
				MessageBox(hWnd, L"Please choose the room", L"Result", MB_OK);
			}
			else {
				int id = atoi(roomID);
				string status = rooms[id].status;
				join(roomID);
			}
			break;
		}
		case RESULT: // Get roomID and call function result(roomID)
		{
			wchar_t roomIDW[10];
			GetWindowTextW(hRoomID, roomIDW, 10);
			char roomID[10];
			wcstombs(roomID, roomIDW, 10);

			if (strcmp(roomID, "") == 0) {
				MessageBox(hWnd, L"Chua chon phong", L"Result", MB_OK);
			}
			else {
				int id = atoi(roomID);
				string status = rooms[id].status;
				result(roomID);
			}
			break;
		}
		case VIEW_CREATE_ROOM:
			DestroyWindow(hwnd);
			ViewCreateRoom(hWnd);
			break;
		case REFRESH: // Update list of exam rooms
			listRoom();
		case BACK:
			DestroyWindow(hwnd);
			CreateView2(hWnd);
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// Create window for the user input number of questions and time to create new exam room
void ViewCreateRoom(HWND hwnd) {
	WNDCLASSW swc = { 0 };
	swc.lpszClassName = L"ViewCreateRoom";
	swc.lpfnWndProc = CreateRoomProc;
	RegisterClassW(&swc);
	hWndNow = CreateWindowW(L"ViewCreateRoom", L"", WS_VISIBLE | WS_CHILD, 0, 0, 800, 800, hwnd, NULL, NULL, NULL);
}

// Process when user click to button "Create"
LRESULT CALLBACK CreateRoomProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		CreateWindowW(L"static", L"Number of questions: ", WS_VISIBLE | WS_CHILD, 0, 0, 80, 50, hwnd, NULL, NULL, NULL);
		hNumOfQuestions = CreateWindowW(L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 0, 100, 50, hwnd, NULL, NULL, NULL);
		CreateWindowW(L"static", L"Time: ", WS_VISIBLE | WS_CHILD, 0, 60, 80, 50, hwnd, NULL, NULL, NULL);
		hTime = CreateWindowW(L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 60, 100, 50, hwnd, NULL, NULL, NULL);
		CreateWindowW(L"button", L"Create", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 120, 50, 50, hwnd, (HMENU)CREATE_ROOM, NULL, NULL);
		CreateWindowW(L"button", L"Back", WS_VISIBLE | WS_CHILD | WS_BORDER, 152, 120, 50, 50, hwnd, (HMENU)BACK, NULL, NULL);
	case WM_COMMAND:
		switch (wParam)
		{
		case CREATE_ROOM: // Get number of questions and time from input of user and call function createRoom()
		{
			wchar_t numOfQuestionsW[10];
			GetWindowTextW(hNumOfQuestions, numOfQuestionsW, 10);
			char numOfQuestions[10];
			wcstombs(numOfQuestions, numOfQuestionsW, 10);
			// Error message if user leaves number of questions blank
			if (strcmp(numOfQuestions, "") == 0) {
				MessageBox(hWnd, L"Please enter number of questions.", L"Error!", MB_ICONERROR);
				break;
			}
			wchar_t timeW[10];
			GetWindowTextW(hTime, timeW, 10);
			char time[10];
			wcstombs(time, timeW, 10);
			// Error message if user leaves time blank
			if (strcmp(time, "") == 0) {
				MessageBox(hWnd, L"Please enter time.", L"Error!", MB_ICONERROR);
				break;
			}

			createRoom(numOfQuestions, time);
			break;
		}

		case BACK:
			listRoom();
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// Create window show the exam room, include number of question, time and button "Start"
void CreateViewRoom(HWND hwnd) {
	WNDCLASSW swc = { 0 };
	swc.lpszClassName = L"ViewRoom";
	swc.lpfnWndProc = RoomProc;
	RegisterClassW(&swc);
	hWndNow = CreateWindowW(L"ViewRoom", L"", WS_VISIBLE | WS_CHILD, 0, 0, 800, 800, hwnd, NULL, NULL, NULL);

	int id = stoi(idOfRoom);
	string numOfQuestions = "Number of Questions: " + rooms[id].numOfQuestions;
	string time = "Time: " + rooms[id].time;
	wstring numOfQuestionsStrW = wstring(numOfQuestions.begin(), numOfQuestions.end());
	const wchar_t* numOfQuestionsW = numOfQuestionsStrW.c_str();
	wstring timeStrW = wstring(time.begin(), time.end());
	const wchar_t* timeW = timeStrW.c_str();
	wstring roomIDStrW = wstring(idOfRoom.begin(), idOfRoom.end());
	const wchar_t* roomIDW = roomIDStrW.c_str();

	hRoomID = CreateWindowW(L"static", roomIDW, WS_VISIBLE | WS_CHILD, 0, 0, 200, 50, hWndNow, NULL, NULL, NULL);
	hNumOfQuestions = CreateWindowW(L"static", numOfQuestionsW, WS_VISIBLE | WS_CHILD, 0, 30, 200, 50, hWndNow, NULL, NULL, NULL);
	hTime = CreateWindowW(L"static", timeW, WS_VISIBLE | WS_CHILD, 0, 60, 200, 50, hWndNow, NULL, NULL, NULL);

	CreateWindowW(L"button", L"Start", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 120, 50, 50, hWndNow, (HMENU)START, NULL, NULL);
	CreateWindowW(L"button", L"Back", WS_VISIBLE | WS_CHILD | WS_BORDER, 152, 120, 50, 50, hWndNow, (HMENU)BACK, NULL, NULL);
}

// Process when user click to button "Start"
LRESULT CALLBACK RoomProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case START: // Get id of the room and call function start()
		{
			wchar_t roomIDW[10];
			GetWindowTextW(hRoomID, roomIDW, 100);
			char roomID[10];
			wcstombs(roomID, roomIDW, 100);
			start(roomID);
			break;
		}
		case BACK:
			listRoom();
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// Create window for each question, include question, button "A", "B", "C" and "Submit"
void CreateViewQuestion(HWND hwnd) {
	WNDCLASSW swc = { 0 };
	swc.lpszClassName = L"ViewQuestion";
	swc.lpfnWndProc = QuestionProc;
	RegisterClassW(&swc);
	hWndNow = CreateWindowW(L"ViewQuestion", L"", WS_VISIBLE | WS_CHILD, 0, 0, 800, 800, hwnd, NULL, NULL, NULL);

	char question[BUFF_SIZE];
	int indexQuestion = 0;
	int flag = 0;

	while (true) {
		// When go to end of the string "questions", submit answer to server
		if (questions[indexQuestions] == '\0') {
			submit();
			break;
		}

		// Separate the question and the answer and then display it in the window
		if (questions[indexQuestions] == '/') {
			question[indexQuestion] = '\0';
			wchar_t questionW[BUFF_SIZE];
			mbstowcs(questionW, question, BUFF_SIZE);

			switch (flag++)
			{
			case 0: {
				hQuestion = CreateWindowW(L"static", questionW, WS_VISIBLE | WS_CHILD, 0, 0, 100, 50, hWndNow, NULL, NULL, NULL);
				break;
			}
			case 1: {
				hAnswerA = CreateWindowW(L"button", questionW, WS_VISIBLE | WS_CHILD, 0, 50, 150, 50, hWndNow, (HMENU)ANSWER_A, NULL, NULL);
				break;
			}
			case 2: {
				hAnswerB = CreateWindowW(L"button", questionW, WS_VISIBLE | WS_CHILD, 0, 100, 150, 50, hWndNow, (HMENU)ANSWER_B, NULL, NULL);
				break;
			}
			case 3: {
				hAnswerC = CreateWindowW(L"button", questionW, WS_VISIBLE | WS_CHILD, 0, 150, 150, 50, hWndNow, (HMENU)ANSWER_C, NULL, NULL);
				break;
			}
			default:
				break;
			}
			indexQuestions++;
			indexQuestion = 0;
			question[0] = '\0';
		}
		else
		{
			question[indexQuestion++] = questions[indexQuestions++];
		}
		// Stop loop when get one question and three answers
		if (flag == 4) {
			break;
		}
		hSubmit = CreateWindowW(L"button", L"Submit", WS_VISIBLE | WS_CHILD, 0, 200, 150, 50, hWndNow, (HMENU)SUBMIT, NULL, NULL);
	}
}

// Process when the user click to button "A", "B", "C" and "Submit"
LRESULT CALLBACK QuestionProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case ANSWER_A: // Append "A" to the string "answer" and create window for the next question
		{
			strcat(answer, "A");
			DestroyWindow(hWndNow);
			CreateViewQuestion(hWnd);
			break;
		}
		case ANSWER_B: // Append "B" to the string "answer" and create window for the next question
		{
			strcat(answer, "B");
			DestroyWindow(hWndNow);
			CreateViewQuestion(hWnd);
			break;
		}
		case ANSWER_C: // Append "C" to the string "answer" and create window for the next question
		{
			strcat(answer, "C");
			DestroyWindow(hWndNow);
			CreateViewQuestion(hWnd);
			break;
		}
		case SUBMIT:
		{
			submit();
			break;
		}
		case BACK:
			DestroyWindow(hwnd);
			CreateView2(hWnd);
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// Create window which show result of one room
void CreateViewResult(HWND hwnd, vector<pair<string, string>> players) {
	WNDCLASSW swc = { 0 };
	swc.lpszClassName = L"ViewResult";
	swc.lpfnWndProc = ResultProc;
	RegisterClassW(&swc);
	hWndNow = CreateWindowW(L"ViewResult", L"", WS_VISIBLE | WS_CHILD, 0, 0, 800, 800, hwnd, NULL, NULL, NULL);
	int x = -20;
	for (auto player : players) {
		string content = player.first + ": " + player.second;
		wstring contentStrW = wstring(content.begin(), content.end());
		const wchar_t* contentW = contentStrW.c_str();
		CreateWindowW(L"static", contentW, WS_VISIBLE | WS_CHILD, 10, x += 30, 200, 30, hWndNow, NULL, NULL, NULL);
	}
	CreateWindowW(L"button", L"Back", WS_VISIBLE | WS_CHILD, 10, x += 50, 100, 30, hWndNow, (HMENU)BACK, NULL, NULL);

}

LRESULT CALLBACK ResultProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case BACK:
			listRoom();
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

/* The send() wrapper function*/
void Send(SOCKET s, char *in, int size, int flags) {
	strcat(in, ENDING_DELEMETER);
	int n = send(s, in, size, flags);
	if (n == SOCKET_ERROR) {
		MessageBox(NULL, L"Error: Cannot send data.", L"Information", MB_ICONINFORMATION);
	}
}

/* The recv() wrapper function */
void Receive(SOCKET s, string &rBuffStr) {
	char rBuff[2048];
	int n = recv(s, rBuff, 2048, 0);
	if (n == SOCKET_ERROR) {
		MessageBox(hWnd, L"Error: Cannot receive data.", L"Error", MB_ICONERROR);
	}
	else if (n > 0) {
		rBuff[n] = '\0';
		rBuffStr += string(rBuff);
	}
}

// Send username and password to login
void login(char *username, char *password) {
	char sBuff[BUFF_SIZE] = "LOGIN";
	strcat(sBuff, " ");
	strcat(sBuff, username);
	strcat(sBuff, " ");
	strcat(sBuff, password);
	Send(client, sBuff, BUFF_SIZE, 0);
}

// Send username and password to register
void Register(char *username, char *password) {
	char sBuff[BUFF_SIZE] = "REGISTER";
	strcat(sBuff, " ");
	strcat(sBuff, username);
	strcat(sBuff, " ");
	strcat(sBuff, password);
	Send(client, sBuff, BUFF_SIZE, 0);
}

// Send message LOGOUT
void logout() {
	char sBuff[BUFF_SIZE] = "LOGOUT";
	Send(client, sBuff, BUFF_SIZE, 0);
}

// Send message LISTROOM to server
void listRoom() {
	char sBuff[20] = "LISTROOM";
	Send(client, sBuff, 100, 0);
	return;
}

// Send message JOIN roomID
void join(string roomID) {
	idOfRoom = roomID;
	char sBuff[BUFF_SIZE] = "JOIN ";
	char idC[10];
	strcpy(idC, roomID.c_str());
	strcat(sBuff, idC);
	Send(client, sBuff, BUFF_SIZE, 0);
	return;
}

// Send message CREATEROOM numberOfQuestion time
void createRoom(char *numOfQuestions, char *time) {
	char sBuff[BUFF_SIZE] = "CREATEROOM";
	strcat(sBuff, " ");
	strcat(sBuff, numOfQuestions);
	strcat(sBuff, " ");
	strcat(sBuff, time);
	Send(client, sBuff, BUFF_SIZE, 0);
	return;
}

// Send message START roomID
void start(string roomID) {
	char sBuff[BUFF_SIZE] = "START ";
	char idC[10];
	strcpy(idC, roomID.c_str());
	strcat(sBuff, idC);
	Send(client, sBuff, BUFF_SIZE, 0);
	return;
}

// Send message SUBMIT roomID
void submit() {
	char sBuff[BUFF_SIZE] = "SUBMIT ";
	char idC[10];
	strcpy(idC, idOfRoom.c_str());
	strcat(sBuff, idC);
	strcat(sBuff, " ");
	strcat(sBuff, answer);
	Send(client, sBuff, BUFF_SIZE, 0);
	return;
}

// Send message RESULT roomID
void result(string roomID) {
	char sBuff[BUFF_SIZE] = "RESULT ";
	char idC[10];
	strcpy(idC, roomID.c_str());
	strcat(sBuff, idC);
	Send(client, sBuff, BUFF_SIZE, 0);
	return;
}

// Send message PRACTICE
void practice() {
	char sBuff[BUFF_SIZE] = "PRACTICE";
	Send(client, sBuff, BUFF_SIZE, 0);
	return;
}

// Process data receive from server
void processData(string message) {
	int temp = message.find(' ');
	string replyMessageType = message.substr(0, temp);
	string data = message.substr(temp + 1);

	// Process register reply message
	if (replyMessageType == "10") {
		MessageBox(NULL, L"Register successfully.", L"Information", MB_ICONINFORMATION);
		DestroyWindow(hWndNow);
		CreateView1(hWnd);
	}
	else if (replyMessageType == "20") {
		MessageBox(NULL, L"This username already exists.", L"Information", MB_ICONINFORMATION);
	}

	// Process login reply message
	else if (replyMessageType == "11") {
		MessageBox(NULL, L"Login successfully.", L"Information", MB_ICONINFORMATION);
		DestroyWindow(hWndNow);
		CreateView2(hWnd);
	}
	else if (replyMessageType == "21") {
		MessageBox(NULL, L"Password does not correct.", L"Information", MB_ICONINFORMATION);
	}
	else if (replyMessageType == "22") {
		MessageBox(NULL, L"This account is already logged in another client.", L"Information", MB_ICONINFORMATION);
	}
	else if (replyMessageType == "23") {
		MessageBox(NULL, L"This username does not exists.", L"Information", MB_ICONINFORMATION);
	}
	else if (replyMessageType == "24") {
		MessageBox(NULL, L"you are logged in.", L"Information", MB_ICONINFORMATION);
	}

	// Process logout reply message
	else if (replyMessageType == "12") {
		MessageBox(NULL, L"Logout successfully.", L"Information", MB_ICONINFORMATION);
		DestroyWindow(hWndNow);
		CreateView1(hWnd);
	}

	// Process list room reply message
	else if (replyMessageType == "13") {
		int flag = 0;
		string statusTemp = "", idTemp = "";
		int indexRoom = 0, i = 0;
		// Sperate id and status of room
		while (data[i]) {
			if (data[i] == '/') {
				flag = 0;
				rooms[indexRoom].status = statusTemp;
				statusTemp = "";
				indexRoom++;
				i++;
				continue;
			}
			else
				if (data[i] == ' ') {
					flag = 1;
					rooms[indexRoom].id = idTemp;
					idTemp = "";
					i++;
					continue;
				}
			if (flag == 1) {
				statusTemp += data[i];

			}
			else if (flag == 0) {
				idTemp += data[i];
			}
			i++;
		}
		DestroyWindow(hWndNow);
		CreateViewListRoom(hWnd);
	}

	// Process join reply message: 
	else if (replyMessageType == "14") {
		int id = stoi(idOfRoom);
		int temp = data.find(' ');
		rooms[id].numOfQuestions = data.substr(0, temp);
		rooms[id].time = data.substr(temp + 1);
		DestroyWindow(hWndNow);
		CreateViewRoom(hWnd);
	}
	else if (replyMessageType == "25") {
		MessageBox(hWnd, L"This room is in progress.", L"Information", MB_ICONINFORMATION);
	}
	else if (replyMessageType == "26") {
		MessageBox(hWnd, L"This room has finished.", L"Information", MB_ICONINFORMATION);
	}

	// Process create room reply message
	else if (replyMessageType == "15") {
		string roomID = data;
		MessageBox(NULL, L"Create room successfully.", L"Information", MB_ICONINFORMATION);
		join(roomID);
	}

	// Process start reply message
	else if (replyMessageType == "16") {
		questions = data;
		DestroyWindow(hWndNow);
		CreateViewQuestion(hWnd);
		return;
	}
	else if (replyMessageType == "27") {
		MessageBox(NULL, L"You are not the owner of the room", L"Information", MB_ICONINFORMATION);
	}

	// Process submit reply message
	else if (replyMessageType == "17") {
		string points = "Your points: " + data;
		wstring pointsStrW = wstring(points.begin(), points.end());
		const wchar_t* pointsW = pointsStrW.c_str();
		MessageBox(NULL, pointsW, L"Information", MB_ICONINFORMATION);

		// Reset variable "indexQuestions", "questions" and "answer"
		indexQuestions = 0;
		questions = "";
		answer[0] = '\0';
		listRoom();
	}
	else if (replyMessageType == "28") {
		MessageBox(NULL, L"This room has not begin yet", L"Information", MB_ICONINFORMATION);
	}
	else if (replyMessageType == "SUBMITNOTIFICATION") {
		submit();
	}

	// Process result reply message
	else if (replyMessageType == "18") {
		vector <pair<string, string>> players;
		int i = 0;
		int flag = 0;
		string username, point;
		while (data[i])
		{
			if (data[i] == '/') {
				flag = 0;
				players.push_back(make_pair(username, point));
				username = "";
				point = "";
				i++;
				continue;
			}
			if (data[i] == ' ') {
				flag = 1;
				i++;
				continue;
			}
			if (flag == 0) {
				username += data[i];
			}
			else {
				point += data[i];
			}
			i++;
		}
		DestroyWindow(hWndNow);
		CreateViewResult(hWnd, players);
	}

	// Process practice reply message
	else if (replyMessageType == "19") {
		idOfRoom = "-1";
		questions = data;
		DestroyWindow(hWndNow);
		CreateViewQuestion(hWnd);
	}

	else if (replyMessageType == "99") {
		MessageBox(NULL, L"Try again later", L"Information", MB_ICONINFORMATION);
	}

	return;
}
